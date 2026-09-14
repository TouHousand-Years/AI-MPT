#include "stdafx.h"
#include "AIService.h"
#include "resource.h"
#include "Settings.h"
#include <windowsx.h>
#include "AIPattern.h"
#include "Moddoc.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "View_pat.h"
#include "Childfrm.h"
#include "UpdateHints.h"
#include "Globals.h"
#include "WindowMessages.h"
#include <sddl.h>
#include <algorithm>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>

OPENMPT_NAMESPACE_BEGIN
namespace AI
{
void TestTrace(const std::string &text)
{
	if(const wchar_t *report = _wgetenv(L"OPENMPT_AI_ENDPOINT_REPORT"))
	{
		// Called from the UI thread, the broker thread and the realtime audio
		// callback (first instrumented line only); keep each line intact.
		static std::mutex traceMutex;
		std::lock_guard lock(traceMutex);
		std::ofstream(std::wstring(report) + L".trace", std::ios::app)
			<< GetTickCount64() << " tid=" << GetCurrentThreadId() << " " << text << "\n";
	}
}
namespace
{
constexpr DWORD MaxFrame = 4 * 1024 * 1024;
std::string UTF8(const CString &s) { return mpt::ToCharset(mpt::Charset::UTF8, mpt::ToUnicode(s)); }
CString Text(const std::string &s) { return mpt::ToCString(mpt::ToUnicode(mpt::Charset::UTF8, s)); }

std::wstring CodexTargetPath()
{
	const wchar_t *local = _wgetenv(L"LOCALAPPDATA");
	if(!local || !*local) return {};
	return (std::filesystem::path(local) / L"OpenMPT" / L"AI" / L"codex-target.json").wstring();
}

// Ticket 30: every AI IPC read, write, and wait registers its thread here for
// the duration of the operation. The realtime audio callback scans this set to
// prove no IPC path ever runs on its thread (AI::AudioCallbackIpcCheck).
constexpr size_t IpcThreadSlots = 8;
std::atomic<DWORD> g_ipcThreads[IpcThreadSlots] = {};
struct IpcThreadScope
{
	const DWORD m_thread = GetCurrentThreadId();
	IpcThreadScope()
	{
		for(auto &slot : g_ipcThreads)
		{
			if(slot.load(std::memory_order_relaxed) == m_thread) return;
			DWORD expected = 0;
			if(slot.compare_exchange_strong(expected, m_thread, std::memory_order_relaxed)) return;
		}
	}
	~IpcThreadScope()
	{
		for(auto &slot : g_ipcThreads)
		{
			DWORD expected = m_thread;
			if(slot.compare_exchange_strong(expected, 0, std::memory_order_relaxed)) return;
		}
	}
};

struct Request
{
	Json envelope, response;
	std::mutex mutex;
	std::condition_variable ready;
	bool done = false;
	// Identity of the broker connection the request arrived on; the UI thread
	// uses it to attribute attachment state to this connection (ticket 31).
	uint64 connection = 0;
	void Complete(Json result)
	{
		std::lock_guard lock(mutex);
		response = std::move(result);
		done = true;
		ready.notify_one();
	}
};

// A client connection that had explicitly attached was disconnected. The
// broker never looks at documents or capabilities; it only reports the
// connection identity, and the owning/UI thread decides what to release.
struct DisconnectNotice
{
	uint64 connection = 0;
};

// The broker only handles bytes and envelopes. It has no document pointers.
class Broker
{
	HANDLE m_stop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	std::thread m_worker;
	std::mutex m_clientsMutex;
	std::vector<std::thread> m_clients;
	std::mutex m_mutex;
	std::deque<std::shared_ptr<Request>> m_queue;
	std::mutex m_disconnectMutex;
	std::deque<DisconnectNotice> m_disconnects;
	uint64 m_nextConnection = 0;
	std::wstring m_name;
	std::function<Json()> m_directCall;
	std::atomic<bool> m_running = false;
	void ReapClients(bool all = false)
	{
		std::lock_guard lock(m_clientsMutex);
		for(auto client = m_clients.begin(); client != m_clients.end();)
		{
			if(!all && WaitForSingleObject(client->native_handle(), 0) != WAIT_OBJECT_0)
			{
				++client;
				continue;
			}
			if(client->joinable()) client->join();
			client = m_clients.erase(client);
		}
	}
	bool IO(HANDLE pipe, void *buffer, DWORD size, bool write)
	{
		IpcThreadScope ipc;
		DWORD offset = 0;
		while(offset < size)
		{
			OVERLAPPED ov{};
			ov.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
			DWORD count = 0;
			BOOL ok = write ? WriteFile(pipe, static_cast<char *>(buffer) + offset, size - offset, &count, &ov)
				: ReadFile(pipe, static_cast<char *>(buffer) + offset, size - offset, &count, &ov);
			if(!ok && GetLastError() == ERROR_IO_PENDING)
			{
				HANDLE waits[]{m_stop, ov.hEvent};
				if(WaitForMultipleObjects(2, waits, FALSE, INFINITE) != WAIT_OBJECT_0 + 1)
				{
					CancelIoEx(pipe, &ov);
					GetOverlappedResult(pipe, &ov, &count, TRUE);
					CloseHandle(ov.hEvent);
					return false;
				}
				ok = GetOverlappedResult(pipe, &ov, &count, FALSE);
			}
			CloseHandle(ov.hEvent);
			if(!ok || !count) return false;
			offset += count;
		}
		return true;
	}
	bool Connect(HANDLE pipe)
	{
		OVERLAPPED connect{};
		connect.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		BOOL connected = ConnectNamedPipe(pipe, &connect);
		DWORD error = connected ? ERROR_SUCCESS : GetLastError();
		{
			IpcThreadScope ipc;
			if(error == ERROR_IO_PENDING)
			{
				HANDLE waits[]{m_stop, connect.hEvent};
				if(WaitForMultipleObjects(2, waits, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
				{
					DWORD unused = 0;
					connected = GetOverlappedResult(pipe, &connect, &unused, FALSE);
				} else
				{
					CancelIoEx(pipe, &connect);
					DWORD unused = 0;
					GetOverlappedResult(pipe, &connect, &unused, TRUE);
				}
			} else connected = connected || error == ERROR_PIPE_CONNECTED;
		}
		CloseHandle(connect.hEvent);
		return connected != FALSE;
	}
	void Serve(HANDLE pipe, uint64 connection)
	{
		std::string attachedInstance, attachedDocument;
		while(WaitForSingleObject(m_stop, 0) != WAIT_OBJECT_0)
		{
			DWORD length = 0;
			if(!IO(pipe, &length, 4, false) || length == 0 || length > MaxFrame) break;
			std::string bytes(length, '\0');
			if(!IO(pipe, bytes.data(), length, false)) break;
			Json result;
			try
			{
				auto envelope = Json::parse(bytes);
				if(!envelope.is_object() || envelope.value("version", 0) != 1 || !envelope.at("operation").is_string()
					|| !envelope.at("instance").is_string() || !envelope.at("document").is_string()) throw std::invalid_argument("Envelope");
				const bool attach = envelope["operation"] == "attach";
				if(!attach && (attachedInstance.empty() || envelope["instance"] != attachedInstance || envelope["document"] != attachedDocument))
					result = Failure("notAttached", "Explicit attachment required", "attachment");
				else if(!attach && envelope.value("direct", false))
					result = m_directCall();
				else
				{
					auto request = std::make_shared<Request>();
					request->envelope = envelope;
					request->connection = connection;
					{ std::lock_guard lock(m_mutex); m_queue.push_back(request); }
					std::unique_lock lock(request->mutex);
					IpcThreadScope ipc;
					while(!request->done && WaitForSingleObject(m_stop, 0) != WAIT_OBJECT_0)
					{
						request->ready.wait_for(lock, std::chrono::milliseconds(100));
						if(request->done) break;
						// A request may wait indefinitely for human approval; notice a
						// vanished client promptly instead of only when the eventual
						// reply is written, so its reservation can be released in time.
						DWORD available = 0;
						if(PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr) == FALSE) break;
					}
					if(!request->done) break;
					result = request->response;
					if(attach && result.value("ok", false))
					{
						attachedInstance = envelope["instance"].get<std::string>();
						attachedDocument = envelope["document"].get<std::string>();
					}
				}
			} catch(const Json::exception &) { result = Failure("schemaFailure", "Invalid app envelope", "transport"); }
			catch(const std::invalid_argument &) { result = Failure("schemaFailure", "Invalid app envelope", "transport"); }
			catch(const std::exception &) { result = Failure("internalError", "App service failed", "transport"); }
			bytes = result.dump();
			if(bytes.size() > MaxFrame) bytes = Failure("schemaFailure", "Response exceeds frame limit", "transport").dump();
			length = static_cast<DWORD>(bytes.size());
			if(!IO(pipe, &length, 4, true) || !IO(pipe, bytes.data(), length, true)) break;
		}
		// Only the owning/UI thread may decide whether this connection owns
		// retained capability state. Other connected readers are unaffected.
		if(!attachedInstance.empty())
		{
			std::lock_guard lock(m_disconnectMutex);
			m_disconnects.push_back({connection});
		}
		DisconnectNamedPipe(pipe);
		CloseHandle(pipe);
	}
	void Run()
	{
		HANDLE token = nullptr;
		PSECURITY_DESCRIPTOR descriptor = nullptr;
		if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
		{
			TestTrace("broker: OpenProcessToken failed err=" + std::to_string(GetLastError()));
			return;
		}
		DWORD size = 0;
		GetTokenInformation(token, TokenUser, nullptr, 0, &size);
		std::vector<char> info(size);
		bool valid = GetTokenInformation(token, TokenUser, info.data(), size, &size) != FALSE;
		CloseHandle(token);
		LPWSTR sid = nullptr;
		if(!valid || !ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER *>(info.data())->User.Sid, &sid))
		{
			TestTrace("broker: token user / SID conversion failed");
			return;
		}
		const std::wstring acl = L"D:P(A;;GA;;;" + std::wstring(sid) + L")";
		LocalFree(sid);
		if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(acl.c_str(), SDDL_REVISION_1, &descriptor, nullptr))
		{
			TestTrace("broker: SDDL conversion failed err=" + std::to_string(GetLastError()));
			return;
		}
		SECURITY_ATTRIBUTES security{sizeof(security), descriptor, FALSE};
		bool first = true;
		while(WaitForSingleObject(m_stop, 0) != WAIT_OBJECT_0)
		{
			const DWORD access = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | (first ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0);
			HANDLE pipe = CreateNamedPipeW(m_name.c_str(), access,
				PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
				PIPE_UNLIMITED_INSTANCES, 65536, 65536, 0, &security);
			if(pipe == INVALID_HANDLE_VALUE)
			{
				TestTrace("broker: CreateNamedPipe failed err=" + std::to_string(GetLastError()));
				break;
			}
			if(first)
			{
				first = false;
				m_running = true;
				TestTrace("broker: multi-instance pipe created");
			}
			const uint64 connection = ++m_nextConnection;
			if(!Connect(pipe))
			{
				CloseHandle(pipe);
				break;
			}
			ReapClients();
			std::lock_guard lock(m_clientsMutex);
			m_clients.emplace_back([this, pipe, connection] { Serve(pipe, connection); });
		}
		m_running = false;
		LocalFree(descriptor);
	}
public:
	Broker(std::wstring name, std::function<Json()> directCall)
		: m_name(std::move(name)), m_directCall(std::move(directCall))
	{
		m_worker = std::thread([this] { try { Run(); } catch(...) { m_running = false; } });
	}
	~Broker()
	{
		SetEvent(m_stop);
		if(m_worker.joinable()) m_worker.join();
		ReapClients(true);
		CloseHandle(m_stop);
	}
	bool Running() const { return m_running; }
	std::shared_ptr<Request> Pop()
	{
		std::lock_guard lock(m_mutex);
		if(m_queue.empty()) return {};
		auto result = m_queue.front(); m_queue.pop_front(); return result;
	}
	std::optional<DisconnectNotice> PopDisconnect()
	{
		std::lock_guard lock(m_disconnectMutex);
		if(m_disconnects.empty()) return {};
		auto result = m_disconnects.front(); m_disconnects.pop_front(); return result;
	}
};

CViewPattern *PatternView(CModDoc &doc)
{
	POSITION pos = doc.GetFirstViewPosition();
	while(pos) if(auto *view = dynamic_cast<CViewPattern *>(doc.GetNextView(pos))) return view;
	return nullptr;
}

// Approved Pattern switches must be visible on the Patterns page without
// touching the Sequence, the Order selection or playback. The dedicated AI
// page replaces the lower Patterns view, so when no live CViewPattern exists the
// per-document saved state is updated instead: that is what the next Patterns
// activation restores through the dedicated switchRestore path in
// CCtrlPatterns::OnActivatePage. SetCurrentPattern() already clamps the cursor
// row and collapses any previous selection onto the cursor.
void SynchronizePatternDisplay(CModDoc &doc, PATTERNINDEX pattern)
{
	const auto &sf = doc.GetSoundFile();
	if(!sf.Patterns.IsValidPat(pattern)) return;
	if(auto *view = PatternView(doc))
	{
		view->SetCurrentPattern(pattern);
		view->InvalidatePattern();
		return;
	}
	POSITION pos = doc.GetFirstViewPosition();
	while(pos)
	{
		auto *view = doc.GetNextView(pos);
		auto *frame = view ? dynamic_cast<CChildFrame *>(view->GetParentFrame()) : nullptr;
		if(!frame) continue;
		auto &state = frame->GetPatternViewState();
		state.nPattern = pattern;
		// A freshly created Patterns view always starts at Pattern 0 and asks the
		// Order selection for its Pattern, so the saved state alone is not enough.
		// Mark a dedicated one-shot restore that outranks that initialization.
		state.switchRestore = pattern;
		state.cursor.Sanitize(sf.Patterns[pattern].GetNumRows(), sf.GetNumChannels());
		state.selection = PatternRect(state.cursor, state.cursor);
		state.initialized = true;
		break;
	}
}

// The saved display state is the only Pattern the view-less AI page can name:
// a session-less Pattern switch must report it as its source instead of an
// unbound facade. This is a read-only lookup.
PATTERNINDEX SavedPatternDisplay(CModDoc &doc)
{
	const auto &patterns = doc.GetSoundFile().Patterns;
	POSITION pos = doc.GetFirstViewPosition();
	while(pos)
	{
		auto *view = doc.GetNextView(pos);
		auto *frame = view ? dynamic_cast<CChildFrame *>(view->GetParentFrame()) : nullptr;
		if(!frame) continue;
		const auto &state = frame->GetPatternViewState();
		if(state.switchRestore != PATTERNINDEX_INVALID && patterns.IsValidPat(state.switchRestore)) return state.switchRestore;
		if(state.initialized && patterns.IsValidPat(state.nPattern)) return state.nPattern;
		break;
	}
	return PATTERNINDEX_INVALID;
}

enum Control : UINT { Enable = 1, AlwaysApprove, AlwaysSwitch, AlwaysAccept, Timeout, Apply, Reject, Release, Approve, Decline, Publish, Evidence, SettingsSave };

class Panel;
class ReviewPanel;

// Issue 36: the review half of the AI page is its own child window so the
// dedicated lower splitter view (CViewAI) can host it. Keeping it separate from
// the connection panel lets the service state (broker, capability, review
// data) survive page switches while the UI follows the splitter view.
class ReviewPanel final : public CWnd
{
public:
	Panel *service = nullptr;
	CButton release, approve, decline, apply, reject;
	CListBox evidence;
	CRect statusRect, listRect, evidenceRect;

	ReviewPanel(CWnd &owner, Panel &servicePanel);
	void LayoutChildren();
	void Refresh();
	void UpdateControls();
	void RefreshStatus();
	void DrawEvidence(CDC &dc);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	BOOL OnCommand(WPARAM wParam, LPARAM lParam) override;
	DECLARE_MESSAGE_MAP()
};

class Panel final : public CWnd
{
public:
	struct ThreadProbe
	{
		std::unique_ptr<PatternCapability> capability;
		CModDoc *document = nullptr;
		// Broker connection id of the attachment that created the capability.
		uint64 connection = 0;
		std::mutex mutex;
	};
	std::unique_ptr<Broker> broker;
	std::unique_ptr<PatternCapability> capability;
	ThreadProbe threadProbe;
	CModDoc *document = nullptr;
	std::shared_ptr<Request> pending;
	CButton enable, always, alwaysSwitch, alwaysAccept;
	CEdit timeout, identity;
	std::vector<std::unique_ptr<CButton>> buttons;
	std::string instance;
	// The connection that owns retained (potentially mutating) capability state.
	// Plain attachments and one-shot reads never claim this slot like a switch
	// reservation does, so it is also set while a switch waits for approval.
	uint64 capabilityConnection = 0;
	// Connection ids whose disconnect notice already arrived. A request from such
	// a connection must never create a new reservation or session.
	std::deque<uint64> disconnected;
	std::wstring pipe;
	Json review;
	CString identityText;
	unsigned seconds = 300;
	bool selecting = false, keyboardSelecting = false;
	PatternCursor selectionAnchor;
	uint64 displayedRevision = 0;
	CString lastMessage;
	CRect identityRect;
	// Main frame is the safe parking parent while no control view shows the panel.
	CWnd *parking = nullptr;
#ifdef ENABLE_TESTS
	std::wstring testReport;
	ULONGLONG testDeadline = 0;
	ULONGLONG tickTrace = 0;
	PATTERNINDEX patternSettle = PATTERNINDEX_INVALID;
#endif
	std::unique_ptr<Broker> CreateBroker()
	{
		return std::make_unique<Broker>(pipe, [this]
		{
			std::lock_guard lock(threadProbe.mutex);
			if(!threadProbe.capability)
				return Failure("internalError", "No attached capability is available for the direct-call probe", "dispatch");
			// This is the real issue-29 facade call. It must reject on its first
			// owning-thread guard, before reading any document state.
			auto result = threadProbe.capability->Call("get_pattern_context", Json::object());
			if(!result.value("ok", false) && result["error"].value("code", "") == "owningThreadRequired")
				result["error"]["layer"] = "dispatch";
			return result;
		});
	}
	Panel(CWnd &owner)
	{
		GUID guid{}; CoCreateGuid(&guid); wchar_t id[40]{}; StringFromGUID2(guid, id, 40);
		instance = UTF8(id);
		pipe = L"\\\\.\\pipe\\OpenMPT-AI-" + std::to_wstring(GetCurrentProcessId()) + L"-" + id;
		parking = &owner;
		// Issue 35: always a hidden child window. It is never a floating top-level
		// window; CModControlView attaches it inside its tab client area.
		CreateEx(0, AfxRegisterWndClass(0, LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1)),
			_T("AI / MCP - Pattern collaboration"), WS_CHILD, CRect(0, 0, 1040, 692), &owner, 0);
		enable.Create(_T("Enable MCP"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, CRect(12, 12, 150, 38), this, Enable);
		always.Create(_T("Always approve range expansion"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, CRect(160, 12, 460, 38), this, AlwaysApprove);
		alwaysSwitch.Create(_T("Always allow Pattern switching"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, CRect(12, 44, 330, 70), this, AlwaysSwitch);
		alwaysAccept.Create(_T("Always accept submissions"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, CRect(340, 44, 640, 70), this, AlwaysAccept);
		timeout.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, CRect(470, 12, 530, 38), this, Timeout);
		seconds = std::clamp(theApp.GetSettings().Read<unsigned>(U_("AI/MCP"), U_("TimeoutSeconds"), 300), 1u, 3600u);
		CString value; value.Format(_T("%u"), seconds); timeout.SetWindowText(value);
		always.SetCheck(theApp.GetSettings().Read<bool>(U_("AI/MCP"), U_("AlwaysApprove"), false));
		// Pattern-switch and automatic-submission preferences persist independently
		// of the range-expansion preference and default to off.
		alwaysSwitch.SetCheck(theApp.GetSettings().Read<bool>(U_("AI/MCP"), U_("AlwaysSwitch"), false));
		alwaysAccept.SetCheck(theApp.GetSettings().Read<bool>(U_("AI/MCP"), U_("AlwaysAccept"), false));
		enable.SetCheck(theApp.GetSettings().Read<bool>(U_("AI/MCP"), U_("Enabled"), true));
		auto button = [&](UINT id, LPCTSTR text, CRect rect)
		{
			auto b = std::make_unique<CButton>(); b->Create(text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, rect, this, id); buttons.push_back(std::move(b));
		};
		button(SettingsSave, _T("Save settings (seconds)"), CRect(540, 12, 750, 38));
		button(Publish, _T("Connect active doc to Codex"), CRect(770, 12, 990, 38));
		identity.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOHSCROLL, CRect(12, 48, 1040, 155), this, 20);
		LayoutChildren();
		if(enable.GetCheck()) broker = CreateBroker();
		SetTimer(1, 100, nullptr);
		RefreshIdentity();
		TestTrace("panel: ctor done");
	}
	~Panel()
	{
		if(pending) pending->Complete(Failure("instanceGone", "Application stopping", "attachment"));
		broker.reset();
		std::lock_guard lock(threadProbe.mutex);
		threadProbe.capability.reset();
		capability.reset();
	}
	void RefreshIdentity()
	{
		auto *active = CMainFrame::GetMainFrame()->GetActiveDoc();
		CString value = (broker && broker->Running()) ? _T("Status: MCP ready") : _T("Status: MCP stopped / starting");
		value += _T("\r\nPipe: ") + CString(pipe.c_str()) + _T("\r\nInstance: ") + Text(instance);
		if(const auto target = CodexTargetPath(); !target.empty())
			value += _T("\r\nCodex target: ") + CString(target.c_str());
		value += _T("\r\nOpen documents (probe clients must be given one explicit ID):");
		for(auto *doc : theApp.GetOpenDocuments())
		{
			value += _T("\r\n  ") + Text(doc->AIIdentity()) + _T(" | ") + doc->GetTitle();
			if(doc == active) value += _T("  (active; session binds its Patterns-tab Pattern and selection)");
		}
		if(theApp.GetOpenDocuments().empty()) value += _T("\r\n  (none) Open a document and its Patterns tab.");
		// Called from the timer pump: only rewrite the control when something changed.
		if(value != identityText)
		{
			identityText = value;
			identity.SetWindowText(value);
		}
	}
	// Issue 36: the upper row only holds the MCP connection and configuration
	// controls; the review controls live in the dedicated lower view. Controls
	// wrap onto as many rows as the current width needs instead of being squeezed
	// into unreadable fixed columns; the identity list takes the remaining space.
	void LayoutChildren()
	{
		if(!GetSafeHwnd() || !enable.GetSafeHwnd() || !identity.GetSafeHwnd() || !alwaysSwitch.GetSafeHwnd() || !alwaysAccept.GetSafeHwnd()) return;
		CRect client;
		GetClientRect(&client);
		const int cx = std::max<int>(client.Width(), 320);
		const UINT flags = SWP_NOZORDER | SWP_NOACTIVATE;
		const int margin = 12, gap = 8, height = 26, lineGap = 6;
		int x = margin, y = margin;
		const auto measure = [this](CWnd &control, LPCTSTR fallback, int minimum, int maximum)
		{
			CString caption;
			control.GetWindowText(caption);
			if(caption.IsEmpty()) caption = fallback;
			CClientDC dc(this);
			CFont *font = control.GetFont();
			CFont *previous = font ? dc.SelectObject(font) : nullptr;
			const int width = dc.GetTextExtent(caption).cx + 14;
			if(previous) dc.SelectObject(previous);
			return std::clamp(width, minimum, maximum);
		};
		const auto place = [&](CWnd &control, int preferred)
		{
			const int width = std::max(40, std::min(preferred, cx - 2 * margin));
			if(x > margin && x + width > cx - margin)
			{
				x = margin;
				y += height + lineGap;
			}
			control.SetWindowPos(nullptr, x, y, width, height, flags);
			x += width + gap;
		};
		place(enable, measure(enable, _T("Enable MCP"), 90, 220));
		place(always, measure(always, _T("Always approve range expansion"), 150, 320));
		place(timeout, 70);
		if(!buttons.empty()) place(*buttons[0], measure(*buttons[0], _T("Save settings (seconds)"), 120, 240));
		place(alwaysSwitch, measure(alwaysSwitch, _T("Always allow Pattern switching"), 160, 330));
		place(alwaysAccept, measure(alwaysAccept, _T("Always accept submissions"), 140, 310));
		if(buttons.size() > 1) place(*buttons[1], measure(*buttons[1], _T("Connect active doc to Codex"), 130, 260));
		y += height + 10;
		identityRect.SetRect(margin, y, std::max(margin + 18, cx - margin), std::max(y + 18, client.Height() - margin));
		identity.SetWindowPos(nullptr, identityRect.left, identityRect.top, identityRect.Width(), identityRect.Height(), flags);
		Invalidate(FALSE);
	}
	afx_msg void OnSize(UINT nType, int cx, int cy)
	{
		CWnd::OnSize(nType, cx, cy);
		LayoutChildren();
	}
	void ReleaseNow()
	{
		if(capability) capability->ForceRelease();
		capabilityConnection = 0;
		if(pending) { pending->Complete(Failure("occupancyLost", "Human released the session")); pending.reset(); }
	}
	void PublishActiveDocument()
	{
		if(!broker || !broker->Running())
		{
			lastMessage = _T("Enable MCP and wait for the service to start before connecting Codex.");
			return;
		}
		if(capability && (capability->Occupied() || capability->PendingSwitch() || capability->HasProposal()))
		{
			lastMessage = _T("Finish or release current AI work before publishing another target.");
			return;
		}
		auto *target = CMainFrame::GetMainFrame()->GetActiveDoc();
		if(!target)
		{
			lastMessage = _T("Open and activate a document before connecting Codex.");
			return;
		}
		const std::wstring path = CodexTargetPath();
		if(path.empty())
		{
			lastMessage = _T("LOCALAPPDATA is unavailable; the Codex target cannot be published.");
			return;
		}
		std::error_code directoryError;
		std::filesystem::create_directories(std::filesystem::path(path).parent_path(), directoryError);
		if(directoryError)
		{
			lastMessage = _T("Could not create the Codex target directory.");
			return;
		}
		GUID guid{};
		if(CoCreateGuid(&guid) != S_OK)
		{
			lastMessage = _T("Could not create a Codex publication identity.");
			return;
		}
		wchar_t id[40]{};
		StringFromGUID2(guid, id, 40);
		const Json published{{"version", 1}, {"pipe", UTF8(CString(pipe.c_str()))}, {"instance", instance},
			{"document", target->AIIdentity()}, {"generation", UTF8(CString(id))}, {"title", UTF8(target->GetTitle())}};
		const std::wstring temporary = path + L".tmp-" + std::to_wstring(GetCurrentProcessId());
		{
			std::ofstream output(std::filesystem::path(temporary), std::ios::binary | std::ios::trunc);
			const std::string bytes = published.dump();
			output.write(bytes.data(), bytes.size());
			output.close();
			if(!output)
			{
				DeleteFileW(temporary.c_str());
				lastMessage = _T("Could not write the Codex target file.");
				return;
			}
		}
		if(!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
		{
			DeleteFileW(temporary.c_str());
			lastMessage = _T("Could not publish the Codex target file.");
			return;
		}
		lastMessage = _T("Active document published to Codex. No MCP reconfiguration is needed.");
		RefreshIdentity();
	}
	void HandleDisconnect(const DisconnectNotice &notice)
	{
		// Remember the identity so a request from this connection that is still
		// queued behind the disconnect notice can never create a reservation or
		// session; connection ids are unique and never reused.
		disconnected.push_back(notice.connection);
		if(disconnected.size() > 64) disconnected.pop_front();
		{
			std::lock_guard lock(threadProbe.mutex);
			if(threadProbe.connection == notice.connection)
			{
				threadProbe.capability.reset();
				threadProbe.document = nullptr;
				threadProbe.connection = 0;
			}
		}
		// Ticket 31 AC4: process loss releases only state retained by this exact
		// connection. Disconnects from concurrent read-only clients are inert.
		if(notice.connection != capabilityConnection) return;
		capabilityConnection = 0;
		if(pending && pending->connection == notice.connection)
		{
			pending->Complete(Failure("occupancyLost", "Client disconnected"));
			pending.reset();
		}
		// A proposal frozen by handoff_for_review belongs to human review and
		// must survive the client; only a proposal-free capability is released.
		if(capability && !capability->HasProposal()) { capability.reset(); document = nullptr; RefreshReview(); }
	}
	Json Dispatch(const Json &envelope, uint64 connection)
	{
		if(!theApp.InGuiThread()) return Failure("owningThreadRequired", "Use document owning thread", "dispatch");
		if(envelope.at("instance") != instance) return Failure("instanceGone", "Instance identity no longer exists", "attachment");
		CModDoc *target = nullptr;
		for(auto *doc : theApp.GetOpenDocuments()) if(envelope.at("document") == doc->AIIdentity()) { target = doc; break; }
		if(!target) return Failure("documentGone", "Document lifetime ended", "attachment");
		if(envelope.at("operation") == "attach")
		{
			std::lock_guard lock(threadProbe.mutex);
			threadProbe.capability = std::make_unique<PatternCapability>(*target, PATTERNINDEX_INVALID, std::nullopt);
			threadProbe.document = target;
			threadProbe.connection = connection;
			return {{"ok", true}};
		}
		if(envelope.at("operation") != "call") return Failure("schemaFailure", "Unknown operation", "transport");
		const std::string tool = envelope.at("tool").get<std::string>();
		const auto &args = envelope.at("arguments");
		if(!args.is_object()) return Failure("schemaFailure", "Arguments must be an object", "transport");
		// Order inspection is a session-less read: serve it through a temporary
		// read-only capability with no bound Pattern, before the view, session and
		// connection-ownership restrictions. An existing session and its owning
		// connection are neither rebound nor released by this probe.
		if(tool == "get_pattern_order")
		{
			PatternCapability reader(*target, PATTERNINDEX_INVALID, std::nullopt);
			reader.Configure(seconds, always.GetCheck() == BST_CHECKED, alwaysSwitch.GetCheck() == BST_CHECKED, alwaysAccept.GetCheck() == BST_CHECKED);
			return reader.Call(tool, args);
		}
		// The notice is drained before queued requests, so a dead connection can
		// never create a reservation or an occupancy other readers could inherit.
		if(std::find(disconnected.begin(), disconnected.end(), connection) != disconnected.end())
			return Failure("occupancyLost", "Client disconnected before dispatch", "attachment");
		if(capability && (capability->Occupied() || capability->PendingSwitch() || capability->HasProposal()) && document != target)
			return Failure("busy", "Another document has active work");
		if(capability && (capability->Occupied() || capability->PendingSwitch()) && capabilityConnection != connection)
			return Failure("busy", "Another MCP connection owns the active session");
		// A session-less switch does not need a prior context read: bind the
		// facade to the displayed Pattern when the Patterns view exists, or fall
		// back to the saved display state on the view-less AI page so the switch
		// source still names the Pattern the human last displayed. Approval
		// captures the target and creates the session.
		if(tool == "switch_pattern" && (!capability || document != target))
		{
			auto *view = PatternView(*target);
			const PATTERNINDEX source = view ? view->GetCurrentPattern() : SavedPatternDisplay(*target);
			capability = std::make_unique<PatternCapability>(*target, source, view ? view->AISelection() : std::nullopt);
			document = target;
		}
		// A session-less context read binds to the Patterns view. It must never
		// replace a facade that is waiting on a switch: the reservation and the
		// wire request waiting on it have to survive other readers.
		if(tool == "get_pattern_context" && !args.contains("session") && (!capability || (!capability->Occupied() && !capability->PendingSwitch() && !capability->HasProposal())))
		{
			auto *view = PatternView(*target);
			if(!view) return Failure("patternRequired", "Open the document's Patterns tab first");
			capability = std::make_unique<PatternCapability>(*target, view->GetCurrentPattern(), view->AISelection());
			document = target;
		}
		if(!capability || document != target) return Failure("occupancyLost", "Start with an occupied context read");
		// Push every independent preference on each dispatch; a partial Configure
		// call would silently reset the switch/accept preferences.
		ConsumeConfigure(capability->Configure(seconds, always.GetCheck() == BST_CHECKED, alwaysSwitch.GetCheck() == BST_CHECKED, alwaysAccept.GetCheck() == BST_CHECKED));
		auto result = capability->Call(tool, args);
		if(capability->Occupied() && result.value("ok", false) && result.contains("session")) capabilityConnection = connection;
		else if(capability->PendingSwitch()) capabilityConnection = connection;
		else if(!capability->Occupied()) capabilityConnection = 0;
		if(result.value("status", std::string{}) == "switched" && document) SynchronizePatternDisplay(*document, capability->Pattern());
		return result;
	}
	std::optional<ModCommand> CurrentCell(const Json &cell) const
	{
		if(!document || !capability) return {};
		const auto &sf = document->GetSoundFile();
		if(!sf.Patterns.IsValidPat(capability->Pattern())) return {};
		const auto row = cell["row"].get<ROWINDEX>();
		const auto channel = cell["channel"].get<CHANNELINDEX>();
		if(row >= sf.Patterns[capability->Pattern()].GetNumRows() || channel >= sf.GetNumChannels()) return {};
		return *sf.Patterns[capability->Pattern()].GetpModCommand(row, channel);
	}
	void RefreshReview();
	void ConsumeConfigure(Json result);
	BOOL OnCommand(WPARAM wParam, LPARAM lParam) override;
	afx_msg void OnTimer(UINT_PTR);
	DECLARE_MESSAGE_MAP()
};
std::unique_ptr<Panel> panel;
std::unique_ptr<ReviewPanel> reviewPanel;

void Panel::RefreshReview()
{
	// Commands such as EN_CHANGE from the timeout edit can re-enter here while
	// the constructor is still creating child controls; reviewPanel is null then.
	review = capability && capability->HasProposal() ? capability->Review() : Json{};
	if(reviewPanel) reviewPanel->Refresh();
}

void Panel::ConsumeConfigure(Json result)
{
	if(!capability) return;
	// Configure resolves work that was waiting on a preference: an existing
	// Pattern switch when "Always allow Pattern switching" is turned on, or an
	// existing proposal when "Always accept submissions" is turned on. A wire
	// request waiting for that work must receive the resolution, and its
	// connection keeps ownership of the resulting session.
	const bool resolved = result.contains("status") || result.contains("ending_reminder");
	if(!resolved) return;
	if(document && result.value("status", std::string{}) == "switched")
		SynchronizePatternDisplay(*document, capability->Pattern());
	lastMessage = Text(result.dump());
	if(pending)
	{
		if(result.value("ok", false) && capability->Occupied()) capabilityConnection = pending->connection;
		pending->Complete(std::move(result));
		pending.reset();
	}
	RefreshReview();
}

BOOL Panel::OnCommand(WPARAM wParam, LPARAM lParam)
{
	const UINT id = LOWORD(wParam);
	try
	{
		if(id == Publish) PublishActiveDocument();
		if(id == SettingsSave || id == Enable || id == AlwaysApprove || id == AlwaysSwitch || id == AlwaysAccept)
		{
			CString value; timeout.GetWindowText(value); seconds = std::clamp(_ttoi(value), 1, 3600);
			const bool rangeAlways = always.GetCheck() == BST_CHECKED;
			const bool switchAlways = alwaysSwitch.GetCheck() == BST_CHECKED;
			const bool acceptAlways = alwaysAccept.GetCheck() == BST_CHECKED;
			theApp.GetSettings().Write<bool>(U_("AI/MCP"), U_("Enabled"), enable.GetCheck() != 0);
			theApp.GetSettings().Write<bool>(U_("AI/MCP"), U_("AlwaysApprove"), rangeAlways);
			// The switch and submission preferences persist independently of the
			// range-expansion preference.
			theApp.GetSettings().Write<bool>(U_("AI/MCP"), U_("AlwaysSwitch"), switchAlways);
			theApp.GetSettings().Write<bool>(U_("AI/MCP"), U_("AlwaysAccept"), acceptAlways);
			theApp.GetSettings().Write<unsigned>(U_("AI/MCP"), U_("TimeoutSeconds"), seconds);
			if(!enable.GetCheck()) { ReleaseNow(); broker.reset(); }
			else if(!broker) broker = CreateBroker();
			// One Configure call carries every independent preference; a partial
			// call would silently reset the switch/accept preferences.
			if(capability) ConsumeConfigure(capability->Configure(seconds, rangeAlways, switchAlways, acceptAlways));
		}
		RefreshReview();
	} catch(const std::exception &) { lastMessage = _T("Operation failed; document was not changed."); }
	return CWnd::OnCommand(wParam, lParam);
}

void Panel::OnTimer(UINT_PTR)
{
	try
	{
#ifdef ENABLE_TESTS
		if(testDeadline)
		{
			if(!testReport.empty() && GetTickCount64() >= tickTrace)
			{
				tickTrace = GetTickCount64() + 2000;
				TestTrace(std::string("tick broker=") + (broker && broker->Running() ? "1" : "0")
					+ " view=" + (document && PatternView(*document) ? "1" : "0")
					+ " doc=" + (document ? "1" : "0"));
			}
			const wchar_t *stop = _wgetenv(L"OPENMPT_AI_STOP_FILE");
			if(GetTickCount64() >= testDeadline || (stop && GetFileAttributesW(stop) != INVALID_FILE_ATTRIBUTES))
			{
				ReleaseNow(); CMainFrame::GetMainFrame()->PostMessage(WM_CLOSE); return;
			}
			if(!testReport.empty() && document && broker && broker->Running())
				if(auto *view = PatternView(*document))
				{
					const wchar_t *pattern = _wgetenv(L"OPENMPT_AI_PATTERN");
					const PATTERNINDEX wanted = pattern ? static_cast<PATTERNINDEX>(_wtoi(pattern)) : 0;
					view->SetCurrentPattern(wanted);
					// The harness plays audio while binding patterns; follow-song
					// would drag the view back to the playing pattern and break
					// the explicit binding under test.
					if(document->GetFollowWnd() == view->m_hWnd)
					{
						document->SetFollowWnd(nullptr);
						TestTrace("endpoint: follow-song disabled");
					}
					// Pattern page initialization completes asynchronously after
					// the view appears and resets the current pattern once more;
					// publish the endpoint only when the requested pattern has
					// stayed bound across two consecutive ticks.
					const PATTERNINDEX bound = view->GetCurrentPattern();
					const bool settled = bound == wanted && patternSettle == wanted;
					patternSettle = bound;
					if(settled)
					{
						Json info{{"pipe", UTF8(pipe.c_str())}, {"instance", instance}, {"document", document->AIIdentity()}};
						const std::wstring tmp = testReport + L".tmp";
						std::ofstream(tmp) << info.dump();
						if(_wrename(tmp.c_str(), testReport.c_str()) == 0) TestTrace("endpoint written");
						else TestTrace("endpoint rename failed err=" + std::to_string(GetLastError()));
						testReport.clear();
						patternSettle = PATTERNINDEX_INVALID;
					} else TestTrace("endpoint: pattern not settled yet");
				}
		}
#endif
		RefreshIdentity();
		if(capability) capability->Tick();
		if(document && displayedRevision != document->AIRevision()) { displayedRevision = document->AIRevision(); RefreshReview(); }
		if(pending && capability && !capability->Occupied() && !capability->PendingSwitch()) { pending->Complete(Failure("occupancyLost", "Session expired")); pending.reset(); }
		// Ticket 31 AC4: drain disconnect notices before new work so a
		// vanished client's occupancy is released before its successor's
		// reattach is dispatched.
		if(broker)
			while(auto notice = broker->PopDisconnect())
				HandleDisconnect(*notice);
		// Keep draining other connections while one request waits for human
		// approval. Dispatch rejects their capability calls as busy, but attach
		// and the reply itself no longer queue behind an unrelated writer.
		if(broker)
			if(auto request = broker->Pop())
			{
				Json result;
				try
				{
#ifdef ENABLE_TESTS
					// Deterministically exercise the dispatch-time liveness recheck:
					// the transport request has already been queued and popped before
					// the explicitly addressed document is closed here.
					if(request->envelope.value("test_close_before_dispatch", false))
					{
						for(auto *doc : theApp.GetOpenDocuments())
							if(request->envelope.at("document") == doc->AIIdentity())
							{
								doc->OnCloseDocument();
								break;
							}
					}
#endif
					result = Dispatch(request->envelope, request->connection);
				}
				catch(const Json::exception &) { result = Failure("schemaFailure", "Invalid request", "transport"); }
				catch(const std::exception &) { result = Failure("internalError", "Application capability failed"); }
				if(result.value("pending_approval", false)) pending = request;
				else request->Complete(result);
				RefreshReview();
			}
		if(reviewPanel)
		{
			reviewPanel->UpdateControls();
			reviewPanel->RefreshStatus();
		}
	} catch(...) { lastMessage = _T("AI service error."); }
}

ReviewPanel::ReviewPanel(CWnd &owner, Panel &servicePanel) : service(&servicePanel)
{
	// Issue 36: a plain child window (not the CModScrollView itself, whose
	// WindowProc blocks WM_COMMAND while the document is AI-occupied). It is
	// created directly inside the dedicated lower view.
	CreateEx(0, AfxRegisterWndClass(0, LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1)),
		_T("AI / MCP - Review"), WS_CHILD, CRect(0, 0, 1040, 420), &owner, 0);
	release.Create(_T("RELEASE AI NOW"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(12, 170, 205, 204), this, Release);
	approve.Create(_T("Approve expansion"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(215, 170, 410, 204), this, Approve);
	decline.Create(_T("Decline expansion"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(420, 170, 615, 204), this, Decline);
	apply.Create(_T("Apply whole proposal"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(625, 170, 820, 204), this, Apply);
	reject.Create(_T("Reject whole proposal"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(830, 170, 1035, 204), this, Reject);
	evidence.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, CRect(12, 265, 1040, 460), this, Evidence);
	evidence.SetFont(CFont::FromHandle(static_cast<HFONT>(GetStockObject(ANSI_FIXED_FONT))));
	LayoutChildren();
	UpdateControls();
}

void ReviewPanel::LayoutChildren()
{
	if(!GetSafeHwnd() || !release.GetSafeHwnd() || !evidence.GetSafeHwnd()) return;
	CRect client;
	GetClientRect(&client);
	const int cx = std::max<int>(client.Width(), 320);
	const int cy = std::max<int>(client.Height(), 120);
	const UINT flags = SWP_NOZORDER | SWP_NOACTIVATE;
	const int margin = 12, gap = 8, height = 34, lineGap = 6;
	int y = 8;
	const int statusH = std::min(64, std::max(44, cy / 5));
	statusRect.SetRect(margin, y, cx - margin, y + statusH);
	y += statusH + 6;
	int x = margin;
	// Buttons wrap when their captions would no longer fit side by side; the
	// approval captions change between switch and expansion wording, so widths
	// are measured from the current text instead of fixed columns.
	const auto measure = [this](CButton &control, int minimum, int maximum)
	{
		CString caption;
		control.GetWindowText(caption);
		CClientDC dc(this);
		CFont *font = control.GetFont();
		CFont *previous = font ? dc.SelectObject(font) : nullptr;
		const int width = dc.GetTextExtent(caption).cx + 16;
		if(previous) dc.SelectObject(previous);
		return std::clamp(width, minimum, maximum);
	};
	const auto place = [&](CButton &control, int preferred)
	{
		const int width = std::max(60, std::min(preferred, cx - 2 * margin));
		if(x > margin && x + width > cx - margin)
		{
			x = margin;
			y += height + lineGap;
		}
		control.SetWindowPos(nullptr, x, y, width, height, flags);
		x += width + gap;
	};
	place(release, measure(release, 110, 220));
	place(approve, measure(approve, 130, 260));
	place(decline, measure(decline, 130, 260));
	place(apply, measure(apply, 130, 260));
	place(reject, measure(reject, 130, 260));
	y += height + lineGap;
	const int remaining = std::max(0, cy - y - 8);
	const int listH = std::max(16, remaining * 2 / 5);
	const int drawnH = std::max(16, remaining - listH - 6);
	listRect.SetRect(margin, y, cx - margin, y + listH);
	y += listH + 6;
	evidenceRect.SetRect(margin, y, cx - margin, y + drawnH);
	evidence.SetWindowPos(nullptr, listRect.left, listRect.top, listRect.Width(), listRect.Height(), flags);
	Invalidate(FALSE);
}

void ReviewPanel::Refresh()
{
	if(!service || !evidence.GetSafeHwnd()) return;
	evidence.ResetContent();
	const Json &review = service->review;
	if(review.is_object() && review.value("ok", false) && service->document)
	{
		const auto &sf = service->document->GetSoundFile();
		for(const auto &cell : review.at("diff"))
		{
			const int row = cell.at("row").get<int>(), channel = cell.at("channel").get<int>();
			const auto live = service->CurrentCell(cell);
			const CString liveName = live ? mpt::ToCString(sf.GetNoteName(live->note, live->instr)) : CString(_T("unavailable"));
			CString line; line.Format(_T("Row %03d Ch %02d  |  %s  ->  %s  |  current %s"), row, channel + 1,
				mpt::ToCString(sf.GetNoteName(cell["before"]["note"].get<uint8>(), cell["before"]["instrument"].get<uint8>())).GetString(),
				mpt::ToCString(sf.GetNoteName(cell["after"]["note"].get<uint8>(), cell["after"]["instrument"].get<uint8>())).GetString(), liveName.GetString());
			evidence.AddString(line);
		}
	}
	UpdateControls();
	Invalidate(FALSE);
}

void ReviewPanel::UpdateControls()
{
	if(!service || !GetSafeHwnd()) return;
	// The same two controls resolve both approval kinds; their captions follow
	// the pending request so the owner always sees what is being approved.
	const bool switchPending = service->capability && service->capability->PendingSwitch();
	bool relabel = false;
	CString caption;
	approve.GetWindowText(caption);
	const CString approveText = switchPending ? _T("Approve Pattern switch") : _T("Approve expansion");
	if(caption != approveText) { approve.SetWindowText(approveText); relabel = true; }
	decline.GetWindowText(caption);
	const CString declineText = switchPending ? _T("Reject Pattern switch") : _T("Decline expansion");
	if(caption != declineText) { decline.SetWindowText(declineText); relabel = true; }
	approve.EnableWindow(service->pending != nullptr);
	decline.EnableWindow(service->pending != nullptr);
	apply.EnableWindow(service->capability && service->capability->HasProposal());
	reject.EnableWindow(service->capability && service->capability->HasProposal());
	// The captions determine the measured button widths, so wrapped rows are
	// laid out again when the labels change.
	if(relabel) LayoutChildren();
}

void ReviewPanel::RefreshStatus()
{
	if(!GetSafeHwnd()) return;
	InvalidateRect(statusRect, FALSE);
}

void ReviewPanel::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	LayoutChildren();
}

void ReviewPanel::OnPaint()
{
	CPaintDC dc(this);
	dc.FillSolidRect(statusRect, GetSysColor(COLOR_WINDOW));
	// The upper connection panel is the home of MCP status; this pane only
	// reports review state so connection information is never duplicated here.
	CString state;
	const bool switchPending = service && service->capability && service->capability->PendingSwitch();
	if(switchPending)
		state = service->pending ? _T("AI OCCUPIED - Pattern switch approval waiting (timer paused)") : _T("AI OCCUPIED - Pattern switch approval waiting");
	else if(service && service->capability && service->capability->Occupied())
		state = service->pending ? _T("AI OCCUPIED - range expansion approval waiting (timer paused)") : _T("AI OCCUPIED - navigation and playback available; writes blocked");
	else if(service && service->capability && service->capability->HasProposal())
		state = _T("AI released - proposal retained for review");
	else
		state = _T("Review idle - no AI session or proposal");
	if(service && service->review.is_object() && service->review.contains("status"))
		state += _T(" | Proposal ") + Text(service->review["status"].get<std::string>());
	dc.TextOut(statusRect.left, statusRect.top + 1, state);
	if(service && service->pending && service->capability)
	{
		if(service->capability->PendingSwitch())
		{
			// A switch reservation authorizes the whole target Pattern, so show
			// the exact source and target identity plus the granted scope.
			const auto range = service->capability->SwitchRange();
			const CString sourceName = Text(range.value("source_name", std::string{}));
			const CString targetName = Text(range.value("target_name", std::string{}));
			CString source, target, detail;
			source.Format(_T("%d"), range.value("source_pattern", -1));
			if(!sourceName.IsEmpty()) source += _T(" \"") + sourceName + _T("\"");
			target.Format(_T("%d"), range.value("target_pattern", -1));
			if(!targetName.IsEmpty()) target += _T(" \"") + targetName + _T("\"");
			detail.Format(_T("Switch Pattern %s -> %s. Grant: all %d rows x %d channels (full Pattern, %s session)."),
				source.GetString(), target.GetString(), range.value("target_rows", 0), range.value("target_channels", 0),
				range.value("authenticated", false) ? _T("retained") : _T("new"));
			dc.TextOut(statusRect.left, statusRect.top + 23, detail);
		}
		else
		{
			const auto range = service->capability->ExpansionRange();
			CString grant; grant.Format(_T("Requested: rows %d-%d, channel %d. Current grant: rows %d-%d, channels %d-%d (1-based channels)."),
				range["first_row"].get<int>(), range["last_row"].get<int>(), range["channel"].get<int>() + 1,
				range["grant_first_row"].get<int>(), range["grant_last_row"].get<int>(), range["grant_first_channel"].get<int>() + 1, range["grant_last_channel"].get<int>() + 1);
			dc.TextOut(statusRect.left, statusRect.top + 23, grant);
		}
	} else if(service)
		dc.TextOut(statusRect.left, statusRect.top + 23, service->lastMessage);
	DrawEvidence(dc);
}

void ReviewPanel::DrawEvidence(CDC &dc)
{
	dc.FillSolidRect(evidenceRect, RGB(25, 28, 35));
	if(!service) return;
	const Json &review = service->review;
	if(!review.is_object() || !review.contains("diff") || review["diff"].empty()) return;
	const auto &diff = review["diff"];
	int first = INT_MAX, last = 0, low = 128, high = 1;
	for(const auto &cell : diff)
	{
		first = std::min(first, cell["row"].get<int>()); last = std::max(last, cell["row"].get<int>());
		for(const char *version : {"before", "after"}) { int n = cell[version]["note"].get<int>(); if(n >= 1 && n <= 128) { low = std::min(low, n); high = std::max(high, n); } }
		if(const auto live = service->CurrentCell(cell); live && live->IsNote()) { low = std::min<int>(low, live->note); high = std::max<int>(high, live->note); }
	}
	if(low > high) { low = 48; high = 72; }
	const int column = std::max(1, evidenceRect.Width() / 3);
	const int plotTop = evidenceRect.top + 20, plotBottom = std::max(plotTop + 1, static_cast<int>(evidenceRect.bottom) - 14);
	const int plotHeight = plotBottom - plotTop;
	for(int projection = 0; projection < 3; ++projection)
	{
		const int x = evidenceRect.left + 8 + projection * column;
		dc.SetTextColor(RGB(230, 235, 240)); dc.SetBkMode(TRANSPARENT);
		dc.TextOut(x, evidenceRect.top + 4, projection == 0 ? _T("Baseline") : projection == 1 ? _T("Proposal") : _T("Current document"));
		for(const auto &cell : diff)
		{
			int note = cell[projection == 0 ? "before" : "after"]["note"].get<int>();
			if(projection == 2) { const auto live = service->CurrentCell(cell); note = live ? live->note : 0; }
			int px = x + (cell["row"].get<int>() - first) * std::max(1, column - 30) / std::max(1, last - first + 1);
			if(note >= 1 && note <= 128)
			{
				int py = plotBottom - (note - low) * plotHeight / std::max(1, high - low);
				dc.FillSolidRect(CRect(px, py, px + 7, py + 6), RGB(100, 180, 240));
			} else dc.FillSolidRect(CRect(px, plotBottom - 5, px + 5, plotBottom), RGB(240, 165, 70));
		}
	}
}

BOOL ReviewPanel::OnCommand(WPARAM wParam, LPARAM lParam)
{
	const UINT id = LOWORD(wParam);
	try
	{
		if(!service) return CWnd::OnCommand(wParam, lParam);
		if(id == Release) service->ReleaseNow();
		if((id == Approve || id == Decline) && service->pending && service->capability)
		{
			// The same controls resolve a pending expansion or a pending switch.
			const bool switchPending = service->capability->PendingSwitch();
			Json result = switchPending ? service->capability->ResolveSwitch(id == Approve)
				: service->capability->ResolveExpansion(id == Approve);
			if(result.value("status", std::string{}) == "switched" && service->document)
				SynchronizePatternDisplay(*service->document, service->capability->Pattern());
			if(result.value("ok", false) && service->capability->Occupied())
				service->capabilityConnection = service->pending->connection;
			else if(!service->capability->Occupied())
				service->capabilityConnection = 0;
			service->lastMessage = Text(result.dump());
			service->pending->Complete(std::move(result));
			service->pending.reset();
		}
		if(id == Apply && service->capability) service->lastMessage = Text(service->capability->Apply().dump());
		if(id == Reject && service->capability) { service->capability->Reject(); service->lastMessage = _T("Proposal rejected."); }
		if(id == Evidence && HIWORD(wParam) == LBN_SELCHANGE && service->document && service->capability)
		{
			const int index = evidence.GetCurSel();
			if(index >= 0 && service->review.contains("diff") && size_t(index) < service->review["diff"].size())
			{
				const auto &cell = service->review["diff"][index];
				if(auto *view = PatternView(*service->document))
				{
					view->SetCurrentPattern(service->capability->Pattern());
					PatternCursor cursor(cell["row"].get<ROWINDEX>(), cell["channel"].get<CHANNELINDEX>());
					view->SetCursorPosition(cursor); view->SetCurSel(cursor); view->InvalidatePattern();
				}
			}
			Invalidate(FALSE); return TRUE;
		}
		service->RefreshReview();
	} catch(const std::exception &) { if(service) service->lastMessage = _T("Operation failed; document was not changed."); }
	return CWnd::OnCommand(wParam, lParam);
}

BEGIN_MESSAGE_MAP(Panel, CWnd)
	ON_WM_TIMER()
	ON_WM_SIZE()
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(ReviewPanel, CWnd)
	ON_WM_SIZE()
	ON_WM_PAINT()
END_MESSAGE_MAP()
}

// Issue 36: dedicated lower splitter view for the AI / MCP page. It hosts the
// persistent ReviewPanel; the upper row keeps the connection/config Panel in
// the regular tab client. The view only claims the review window while it is
// the active host, so a stale view from another document can never hide or
// reposition the UI of the document that currently owns the AI page.
class CViewAI final : public CModScrollView
{
public:
	CViewAI() = default;
	DECLARE_SERIAL(CViewAI)

	void OnInitialUpdate() override
	{
		CModScrollView::OnInitialUpdate();
		CRect client;
		GetClientRect(&client);
		AttachReviewPanel(*this, client);
	}
	afx_msg void OnSize(UINT nType, int cx, int cy)
	{
		CWnd::OnSize(nType, cx, cy);
		LayoutReviewPanel(*this, CRect(0, 0, cx, cy));
	}
	afx_msg void OnDestroy()
	{
		DetachReviewPanel(this);
		CModScrollView::OnDestroy();
	}
	afx_msg LRESULT OnModMDIActivate(WPARAM, LPARAM)
	{
		CRect client;
		GetClientRect(&client);
		AttachReviewPanel(*this, client);
		return 0;
	}
	afx_msg LRESULT OnModMDIDeactivate(WPARAM, LPARAM)
	{
		DetachReviewPanel(this);
		return 0;
	}
	DECLARE_MESSAGE_MAP()
};
IMPLEMENT_SERIAL(CViewAI, CModScrollView, 0)

BEGIN_MESSAGE_MAP(CViewAI, CModScrollView)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_MOD_MDIACTIVATE, &CViewAI::OnModMDIActivate)
	ON_MESSAGE(WM_MOD_MDIDEACTIVATE, &CViewAI::OnModMDIDeactivate)
END_MESSAGE_MAP()

CRuntimeClass *LowerViewRuntimeClass() { return RUNTIME_CLASS(CViewAI); }

bool AttachReviewPanel(CWnd &host, const CRect &rect)
{
	if(!panel || !host.GetSafeHwnd()) return false;
	// Only the AI lower view may claim the review window; other hosts are
	// rejected so a stale page cannot hide or reposition it.
	if(!host.IsKindOf(RUNTIME_CLASS(CViewAI))) return false;
	// The review window is a child of the lower view and is therefore destroyed
	// together with it on every page switch. All review state lives in Panel, so
	// a fresh window for the view that now owns the AI page is enough.
	if(!reviewPanel || !reviewPanel->GetSafeHwnd())
		reviewPanel = std::make_unique<ReviewPanel>(host, *panel);
	if(reviewPanel->GetParent() != &host)
		reviewPanel->SetParent(&host);
	reviewPanel->SetWindowPos(nullptr, rect.left, rect.top, std::max(1, rect.Width()), std::max(1, rect.Height()),
		SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
	reviewPanel->LayoutChildren();
	return true;
}
void LayoutReviewPanel(CWnd &host, const CRect &rect)
{
	if(!reviewPanel || !reviewPanel->GetSafeHwnd()) return;
	if(reviewPanel->GetParent() != &host) return;
	reviewPanel->SetWindowPos(nullptr, rect.left, rect.top, std::max(1, rect.Width()), std::max(1, rect.Height()), SWP_NOZORDER | SWP_NOACTIVATE);
	reviewPanel->LayoutChildren();
}
void DetachReviewPanel(CWnd *host)
{
	if(!reviewPanel) return;
	if(!reviewPanel->GetSafeHwnd())
	{
		// The owning view was destroyed together with this child window; drop
		// the detached wrapper so the next AI page recreates it.
		reviewPanel.reset();
		return;
	}
	if(host && reviewPanel->GetParent() != host) return;
	if(host)
	{
		const HWND focus = ::GetFocus();
		if(focus == reviewPanel->m_hWnd || ::IsChild(reviewPanel->m_hWnd, focus))
			::SetFocus(host->GetSafeHwnd());
	}
	// Hide in place: the parent view stays alive while another document or tab
	// is active, and dies with this window on a page switch.
	reviewPanel->SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);
}

// Ticket 30 acceptance: no IPC read/write/wait/query may execute on the
// realtime audio callback. The audio callback calls this every buffer; the
// per-buffer cost is one atomic exchange and a few relaxed loads. A violation
// is traced (never thrown) so the native integration test can assert on it.
void AudioCallbackIpcCheck()
{
	static std::atomic<bool> announced = false;
	if(!announced.load(std::memory_order_relaxed) && !announced.exchange(true))
		TestTrace("audio-callback: IPC check instrumented");
	const DWORD thread = GetCurrentThreadId();
	for(const auto &slot : g_ipcThreads)
		if(slot.load(std::memory_order_relaxed) == thread)
			TestTrace("VIOLATION: realtime audio callback executed an AI IPC path");
}

void Start(CWnd &owner)
{
	if(!panel) panel = std::make_unique<Panel>(owner);
	if(!reviewPanel) reviewPanel = std::make_unique<ReviewPanel>(owner, *panel);
}
#ifdef ENABLE_TESTS
void IntegrationHost(CWnd &owner, CModDoc &doc, const wchar_t *report)
{
	TestTrace("host: enter");
	Start(owner);
	panel->testReport = report;
	panel->testDeadline = GetTickCount64() + 60000;
	panel->document = &doc;
	panel->enable.SetCheck(BST_CHECKED);
	if(!panel->broker) panel->broker = panel->CreateBroker();
	TestTrace("host: started, broker=" + std::string(panel->broker && panel->broker->Running() ? "1" : "0"));
	doc.ActivateView(IDD_CONTROL_PATTERNS, 0);
	TestTrace(std::string("host: activated, view=") + (PatternView(doc) ? "1" : "0"));
	// The integration process runs hidden, so CModControlView::OnActivateModView
	// skips the tab switch unless our frame is the thread's active window. A
	// visible Patterns view is also the owner-demo precondition.
	if(!PatternView(doc))
	{
		CMainFrame::GetMainFrame()->ShowWindow(SW_SHOW);
		CMainFrame::GetMainFrame()->SetActiveWindow();
		doc.ActivateView(IDD_CONTROL_PATTERNS, 0);
		TestTrace(std::string("host: re-activated shown, view=") + (PatternView(doc) ? "1" : "0"));
	}
	// Issue 30 AC: prove no IPC path runs on the realtime audio callback while
	// probe traffic flows. Open the audio device even though no one is listening.
	doc.OnPatternPlay();
	TestTrace("host: playback requested");
}
#endif
void Stop() { reviewPanel.reset(); panel.reset(); }
bool AttachPanel(CWnd &host, const CRect &rect)
{
	if(!panel || !panel->GetSafeHwnd() || !host.GetSafeHwnd()) return false;
	// Only a regular tab host may claim the connection panel; the AI lower view
	// hosts the review half instead.
	if(!host.IsKindOf(RUNTIME_CLASS(CModControlView))) return false;
	if(panel->GetParent() != &host)
		panel->SetParent(&host);
	panel->SetWindowPos(nullptr, rect.left, rect.top, std::max(1, rect.Width()), std::max(1, rect.Height()),
		SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
	panel->LayoutChildren();
	return true;
}
void LayoutPanel(CWnd &host, const CRect &rect)
{
	if(!panel || !panel->GetSafeHwnd()) return;
	if(panel->GetParent() != &host) return;
	panel->SetWindowPos(nullptr, rect.left, rect.top, std::max(1, rect.Width()), std::max(1, rect.Height()), SWP_NOZORDER | SWP_NOACTIVATE);
	panel->LayoutChildren();
}
void DetachPanel(CWnd *host)
{
	if(!panel || !panel->GetSafeHwnd()) return;
	if(host && panel->GetParent() != host) return;
	if(host)
	{
		const HWND focus = ::GetFocus();
		if(focus == panel->m_hWnd || ::IsChild(panel->m_hWnd, focus))
			::SetFocus(host->GetSafeHwnd());
	}
	panel->SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);
	if(panel->parking && panel->parking->GetSafeHwnd() && panel->GetParent() != panel->parking)
		panel->SetParent(panel->parking);
}
void DocumentClosed(CModDoc &doc)
{
	if(panel)
	{
		std::lock_guard lock(panel->threadProbe.mutex);
		if(panel->threadProbe.document == &doc)
		{
			panel->threadProbe.capability.reset();
			panel->threadProbe.document = nullptr;
		}
	}
	if(panel && panel->document == &doc) { if(panel->pending) { panel->pending->Complete(Failure("documentGone", "Document lifetime ended", "attachment")); panel->pending.reset(); } panel->ReleaseNow(); panel->capability.reset(); panel->document = nullptr; panel->RefreshReview(); }
}

bool IsReadOnlyCommand(UINT command)
{
	switch(command)
	{
	case ID_PLAYER_PLAY: case ID_PLAYER_STOP: case ID_PLAYER_PAUSE:
	case ID_PATTERN_PLAY: case ID_PATTERN_PLAYNOLOOP: case ID_PATTERN_RESTART:
	case ID_VIEW_PATTERNS: case ID_VIEW_SAMPLES: case ID_VIEW_INSTRUMENTS: case ID_VIEW_GLOBALS: case ID_VIEW_COMMENTS:
	case ID_EDIT_COPY: case ID_FILE_CLOSE:
		return true;
	default: return false;
	}
}

bool BlockCommand(UINT command)
{
	return panel && panel->capability && (panel->capability->Occupied() || panel->capability->PendingSwitch()) && !IsReadOnlyCommand(command);
}

bool FilterInput(MSG &msg)
{
	if(!panel || !panel->capability || (!panel->capability->Occupied() && !panel->capability->PendingSwitch())) return false;
	if(msg.hwnd == panel->m_hWnd || ::IsChild(panel->m_hWnd, msg.hwnd)) return false;
	// Review controls live in their own window; they must stay usable while the
	// document is AI-occupied, so their input is never filtered.
	if(reviewPanel && reviewPanel->GetSafeHwnd() && (msg.hwnd == reviewPanel->m_hWnd || ::IsChild(reviewPanel->m_hWnd, msg.hwnd))) return false;
	auto *view = panel->document ? PatternView(*panel->document) : nullptr;
	const bool onPattern = view && (msg.hwnd == view->m_hWnd || ::IsChild(view->m_hWnd, msg.hwnd));
	if(msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN)
	{
		if(msg.wParam == VK_F5 || msg.wParam == VK_F6) { panel->document->OnPatternPlay(); return true; }
		if(msg.wParam == VK_F8) { CMainFrame::GetMainFrame()->SendMessage(WM_COMMAND, ID_PLAYER_STOP); return true; }
		if(onPattern && msg.wParam >= VK_PRIOR && msg.wParam <= VK_DOWN)
		{
			int row = view->GetCurrentRow(), channel = view->GetCurrentChannel();
			switch(msg.wParam) { case VK_UP: --row; break; case VK_DOWN: ++row; break; case VK_LEFT: --channel; break; case VK_RIGHT: ++channel; break;
			case VK_PRIOR: row -= 16; break; case VK_NEXT: row += 16; break; case VK_HOME: row = 0; break; case VK_END: row = panel->document->GetSoundFile().Patterns[view->GetCurrentPattern()].GetNumRows() - 1; break; }
			const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
			if(!shift || !panel->keyboardSelecting) panel->selectionAnchor = PatternCursor(view->GetCurrentRow(), view->GetCurrentChannel());
			panel->keyboardSelecting = shift;
			view->SetCursorPosition(PatternCursor(static_cast<ROWINDEX>(std::clamp(row, 0, int(panel->document->GetSoundFile().Patterns[view->GetCurrentPattern()].GetNumRows()) - 1)), static_cast<CHANNELINDEX>(std::clamp(channel, 0, int(panel->document->GetSoundFile().GetNumChannels()) - 1))));
			if(GetKeyState(VK_SHIFT) & 0x8000) view->SetCurSel(panel->selectionAnchor, PatternCursor(view->GetCurrentRow(), view->GetCurrentChannel(), PatternCursor::lastColumn));
			else { view->SetSelToCursor(); panel->selectionAnchor = PatternCursor(view->GetCurrentRow(), view->GetCurrentChannel()); }
			return true;
		}
		return true;
	}
	if(msg.message == WM_CHAR || msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP || msg.message == WM_SYSCHAR) return true;
	if(msg.message == WM_DROPFILES || msg.message == WM_RBUTTONDOWN || msg.message == WM_RBUTTONUP || msg.message == WM_LBUTTONDBLCLK) return true;
	if(msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)
	{
		if(msg.message == WM_MOUSEWHEEL || msg.message == WM_MOUSEHWHEEL) return false;
		wchar_t cls[64]{}; GetClassNameW(msg.hwnd, cls, 64);
		if(wcscmp(cls, L"SysTabControl32") == 0) return false;
		if(onPattern && msg.message == WM_LBUTTONDOWN)
		{
			CPoint point(GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam));
			::MapWindowPoints(msg.hwnd, view->m_hWnd, &point, 1);
			auto cursor = view->GetPositionFromPoint(point); cursor.Sanitize(panel->document->GetSoundFile().Patterns[view->GetCurrentPattern()].GetNumRows(), panel->document->GetSoundFile().GetNumChannels()); view->SetCursorPosition(cursor); view->SetCurSel(cursor); panel->selectionAnchor = cursor; panel->selecting = true; panel->keyboardSelecting = false;
		}
		if(onPattern && msg.message == WM_MOUSEMOVE && panel->selecting && (msg.wParam & MK_LBUTTON))
		{
			CPoint point(GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam)); ::MapWindowPoints(msg.hwnd, view->m_hWnd, &point, 1);
			auto cursor = view->GetPositionFromPoint(point); cursor.Sanitize(panel->document->GetSoundFile().Patterns[view->GetCurrentPattern()].GetNumRows(), panel->document->GetSoundFile().GetNumChannels());
			view->SetCurSel(panel->selectionAnchor, cursor);
		}
		if(msg.message == WM_LBUTTONUP) panel->selecting = false;
		return true;
	}
	return false;
}
}
OPENMPT_NAMESPACE_END
