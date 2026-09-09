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
#include "UpdateHints.h"
#include <sddl.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
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
	std::mutex m_mutex;
	std::deque<std::shared_ptr<Request>> m_queue;
	std::mutex m_disconnectMutex;
	std::deque<DisconnectNotice> m_disconnects;
	uint64 m_nextConnection = 0;
	std::wstring m_name;
	std::function<Json()> m_directCall;
	std::atomic<bool> m_running = false;
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
		HANDLE pipe = CreateNamedPipeW(m_name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, 1, 65536, 65536, 0, &security);
		LocalFree(descriptor);
		if(pipe == INVALID_HANDLE_VALUE)
		{
			TestTrace("broker: CreateNamedPipe failed err=" + std::to_string(GetLastError()));
			return;
		}
		TestTrace("broker: pipe created");
		m_running = true;
		while(WaitForSingleObject(m_stop, 0) != WAIT_OBJECT_0)
		{
			const uint64 connection = ++m_nextConnection;
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
			if(!connected) break;
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
							request->ready.wait_for(lock, std::chrono::milliseconds(100));
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
			// Ticket 31 AC4: an attached client vanishing must leave no residue.
			// The owning thread drains this notice and releases exactly the state
			// bound to this connection; a later reattach on a new connection is
			// never touched by this stale identity.
			if(!attachedInstance.empty())
			{
				std::lock_guard lock(m_disconnectMutex);
				m_disconnects.push_back({connection});
			}
			DisconnectNamedPipe(pipe);
		}
		m_running = false;
		CloseHandle(pipe);
	}
public:
	Broker(std::wstring name, std::function<Json()> directCall)
		: m_name(std::move(name)), m_directCall(std::move(directCall))
	{
		m_worker = std::thread([this] { try { Run(); } catch(...) { m_running = false; } });
	}
	~Broker() { SetEvent(m_stop); if(m_worker.joinable()) m_worker.join(); CloseHandle(m_stop); }
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

enum Control : UINT { Enable = 1, AlwaysApprove, Timeout, Apply, Reject, Release, Approve, Decline, Refresh, Evidence, SettingsSave };

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
	CButton enable, always;
	CEdit timeout, identity;
	CListBox evidence;
	std::vector<std::unique_ptr<CButton>> buttons;
	std::string instance;
	// Broker connection id whose attachment owns the current session state.
	uint64 attachedConnection = 0;
	std::wstring pipe;
	Json review;
	CString identityText;
	unsigned seconds = 300;
	bool selecting = false, keyboardSelecting = false;
	PatternCursor selectionAnchor;
	uint64 displayedRevision = 0;
	CString lastMessage;
	CRect identityRect, stateRect, listRect, evidenceRect;
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
		timeout.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, CRect(470, 12, 530, 38), this, Timeout);
		seconds = std::clamp(theApp.GetSettings().Read<unsigned>(U_("AI/MCP"), U_("TimeoutSeconds"), 300), 1u, 3600u);
		CString value; value.Format(_T("%u"), seconds); timeout.SetWindowText(value);
		always.SetCheck(theApp.GetSettings().Read<bool>(U_("AI/MCP"), U_("AlwaysApprove"), false));
		enable.SetCheck(theApp.GetSettings().Read<bool>(U_("AI/MCP"), U_("Enabled"), true));
		auto button = [&](UINT id, LPCTSTR text, CRect rect)
		{
			auto b = std::make_unique<CButton>(); b->Create(text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, rect, this, id); buttons.push_back(std::move(b));
		};
		button(SettingsSave, _T("Save settings (seconds)"), CRect(540, 12, 750, 38));
		button(Refresh, _T("Refresh attachment"), CRect(770, 12, 990, 38));
		identity.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOHSCROLL, CRect(12, 48, 1040, 155), this, 20);
		button(Release, _T("RELEASE AI NOW"), CRect(12, 170, 205, 204));
		button(Approve, _T("Approve expansion"), CRect(215, 170, 410, 204));
		button(Decline, _T("Decline expansion"), CRect(420, 170, 615, 204));
		button(Apply, _T("Apply whole proposal"), CRect(625, 170, 820, 204));
		button(Reject, _T("Reject whole proposal"), CRect(830, 170, 1035, 204));
		evidence.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, CRect(12, 265, 1040, 460), this, Evidence);
		evidence.SetFont(CFont::FromHandle(static_cast<HFONT>(GetStockObject(ANSI_FIXED_FONT))));
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
		CString value = _T("Pipe: ") + CString(pipe.c_str()) + _T("\r\nInstance: ") + Text(instance);
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
	// Issue 35: the panel is embedded in the tab client area, so every control
	// is repositioned to the current client size. Rows keep their order and the
	// critical action buttons stay above the evidence areas at any size.
	void LayoutChildren()
	{
		if(!GetSafeHwnd() || !enable.GetSafeHwnd() || !identity.GetSafeHwnd() || !evidence.GetSafeHwnd()) return;
		CRect client;
		GetClientRect(&client);
		const int cx = std::max<int>(client.Width(), 320);
		const int cy = std::max<int>(client.Height(), 240);
		const double sx = std::clamp((cx - 24.0) / (1052.0 - 24.0), 0.4, 2.0);
		const auto X = [sx](int x) { return static_cast<int>((x - 12) * sx + 12.5); };

		// Vertical budget: top row (26), identity (107), buttons (34), state (45),
		// evidence list (195) and drawn evidence (205) with their gaps add up to
		// 692. Shrink the three flexible areas first, then scale everything if
		// the page becomes smaller than their combined minimum.
		int identityH = 107, listH = 195, drawnH = 205;
		if(cy < 692)
		{
			const int available = std::max(cy - 185, 0);
			const int minSum = 45 + 60 + 60, baseFlex = 107 + 195 + 205;
			if(available >= minSum)
			{
				const int span = available - minSum;
				identityH = 45 + (107 - 45) * span / (baseFlex - minSum);
				listH = 60 + (195 - 60) * span / (baseFlex - minSum);
				drawnH = 60 + (205 - 60) * span / (baseFlex - minSum);
			} else
			{
				identityH = std::max(18, 107 * available / baseFlex);
				listH = std::max(18, 195 * available / baseFlex);
				drawnH = std::max(18, 205 * available / baseFlex);
			}
		}
		int y = 12;
		const int topRow = y; y += 26 + 10;
		const int identityTop = y; y += identityH + 15;
		const int buttonsTop = y; y += 34 + 11;
		const int stateTop = y; y += 45 + 5;
		const int listTop = y; y += listH + 15;
		const int drawnTop = y;

		identityRect.SetRect(X(12), identityTop, X(1040), identityTop + identityH);
		stateRect.SetRect(X(12), stateTop, X(1040), stateTop + 45);
		listRect.SetRect(X(12), listTop, X(1040), listTop + listH);
		evidenceRect.SetRect(X(12), drawnTop, X(1040), drawnTop + drawnH);

		const UINT flags = SWP_NOZORDER | SWP_NOACTIVATE;
		enable.SetWindowPos(nullptr, X(12), topRow, std::max(40, X(150) - X(12)), 26, flags);
		always.SetWindowPos(nullptr, X(160), topRow, std::max(40, X(460) - X(160)), 26, flags);
		timeout.SetWindowPos(nullptr, X(470), topRow, std::max(30, X(530) - X(470)), 26, flags);
		identity.SetWindowPos(nullptr, identityRect.left, identityRect.top, identityRect.Width(), identityRect.Height(), flags);
		evidence.SetWindowPos(nullptr, listRect.left, listRect.top, listRect.Width(), listRect.Height(), flags);
		const CRect buttonBase[]{CRect(540, 12, 750, 38), CRect(770, 12, 990, 38), CRect(12, 170, 205, 204),
			CRect(215, 170, 410, 204), CRect(420, 170, 615, 204), CRect(625, 170, 820, 204), CRect(830, 170, 1035, 204)};
		for(size_t i = 0; i < buttons.size() && i < 7; i++)
		{
			const bool top = buttonBase[i].bottom <= 38;
			buttons[i]->SetWindowPos(nullptr, X(buttonBase[i].left), top ? topRow : buttonsTop,
				std::max(40, X(buttonBase[i].right) - X(buttonBase[i].left)), top ? 26 : 34, flags);
		}
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
		if(pending) { pending->Complete(Failure("occupancyLost", "Human released the session")); pending.reset(); }
	}
	void HandleDisconnect(const DisconnectNotice &notice)
	{
		// Ticket 31 AC4: process loss leaves no app residue. Release only the
		// state bound to this exact attachment; connection ids are monotonic and
		// unique within the broker, so the id alone identifies the attachment,
		// and a newer client already attached on another connection is never
		// touched by this stale notice.
		if(notice.connection != attachedConnection) return;
		attachedConnection = 0;
		// Unhanded retained work and pending requests die with their connection.
		if(pending) { pending->Complete(Failure("occupancyLost", "Client disconnected")); pending.reset(); }
		// A proposal frozen by handoff_for_review belongs to human review and
		// must survive the client; only a proposal-free capability is released.
		if(capability && !capability->HasProposal()) { capability.reset(); document = nullptr; RefreshReview(); }
		std::lock_guard lock(threadProbe.mutex);
		if(threadProbe.connection == notice.connection)
		{
			threadProbe.capability.reset();
			threadProbe.document = nullptr;
			threadProbe.connection = 0;
		}
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
			attachedConnection = connection;
			return {{"ok", true}};
		}
		if(envelope.at("operation") != "call") return Failure("schemaFailure", "Unknown operation", "transport");
		const std::string tool = envelope.at("tool").get<std::string>();
		const auto &args = envelope.at("arguments");
		if(!args.is_object()) return Failure("schemaFailure", "Arguments must be an object", "transport");
		if(capability && (capability->Occupied() || capability->HasProposal()) && document != target) return Failure("busy", "Another document has active work");
		if(tool == "get_pattern_context" && !args.contains("session") && (!capability || (!capability->Occupied() && !capability->HasProposal())))
		{
			auto *view = PatternView(*target);
			if(!view) return Failure("patternRequired", "Open the document's Patterns tab first");
			capability = std::make_unique<PatternCapability>(*target, view->GetCurrentPattern(), view->AISelection());
			document = target;
		}
		if(!capability || document != target) return Failure("occupancyLost", "Start with an occupied context read");
		capability->Configure(seconds, always.GetCheck() == BST_CHECKED);
		return capability->Call(tool, args);
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
	void RefreshReview()
	{
		// Commands such as EN_CHANGE from the timeout edit re-enter here while
		// the constructor is still creating child controls.
		if(!evidence.GetSafeHwnd()) return;
		review = capability && capability->HasProposal() ? capability->Review() : Json{};
		evidence.ResetContent();
		if(review.is_object() && review.value("ok", false))
		{
			for(const auto &cell : review.at("diff"))
			{
				const int row = cell.at("row").get<int>(), channel = cell.at("channel").get<int>();
				const auto &sf = document->GetSoundFile();
				const auto live = CurrentCell(cell);
				const CString liveName = live ? mpt::ToCString(sf.GetNoteName(live->note, live->instr)) : CString(_T("unavailable"));
				CString line; line.Format(_T("Row %03d Ch %02d  |  %s  ->  %s  |  current %s"), row, channel + 1,
					mpt::ToCString(sf.GetNoteName(cell["before"]["note"].get<uint8>(), cell["before"]["instrument"].get<uint8>())).GetString(),
					mpt::ToCString(sf.GetNoteName(cell["after"]["note"].get<uint8>(), cell["after"]["instrument"].get<uint8>())).GetString(), liveName.GetString());
				evidence.AddString(line);
			}
		}
		Invalidate(FALSE);
	}
	BOOL OnCommand(WPARAM wParam, LPARAM lParam) override
	{
		const UINT id = LOWORD(wParam);
		try
		{
			if(id == Release) ReleaseNow();
			if(id == Refresh) RefreshIdentity();
			if(id == SettingsSave || id == Enable || id == AlwaysApprove)
			{
				CString value; timeout.GetWindowText(value); seconds = std::clamp(_ttoi(value), 1, 3600);
				theApp.GetSettings().Write<bool>(U_("AI/MCP"), U_("Enabled"), enable.GetCheck() != 0);
				theApp.GetSettings().Write<bool>(U_("AI/MCP"), U_("AlwaysApprove"), always.GetCheck() != 0);
				theApp.GetSettings().Write<unsigned>(U_("AI/MCP"), U_("TimeoutSeconds"), seconds);
				if(!enable.GetCheck()) { ReleaseNow(); broker.reset(); }
				else if(!broker) broker = CreateBroker();
				if(capability) capability->Configure(seconds, always.GetCheck() == BST_CHECKED);
			}
			if((id == Approve || id == Decline) && pending && capability)
			{
				pending->Complete(capability->ResolveExpansion(id == Approve)); pending.reset();
			}
			if(id == Apply && capability) lastMessage = Text(capability->Apply().dump());
			if(id == Reject && capability) { capability->Reject(); lastMessage = _T("Proposal rejected."); }
			if(id == Evidence && HIWORD(wParam) == LBN_SELCHANGE && document && capability)
			{
				const int index = evidence.GetCurSel();
				if(index >= 0 && review.contains("diff") && size_t(index) < review["diff"].size())
				{
					const auto &cell = review["diff"][index];
					if(auto *view = PatternView(*document))
					{
						view->SetCurrentPattern(capability->Pattern());
						PatternCursor cursor(cell["row"].get<ROWINDEX>(), cell["channel"].get<CHANNELINDEX>());
						view->SetCursorPosition(cursor); view->SetCurSel(cursor); view->InvalidatePattern();
					}
				}
				Invalidate(FALSE); return TRUE;
			}
			RefreshReview();
		} catch(const std::exception &) { lastMessage = _T("Operation failed; document was not changed."); }
		return CWnd::OnCommand(wParam, lParam);
	}
	afx_msg void OnTimer(UINT_PTR)
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
			if(pending && capability && !capability->Occupied()) { pending->Complete(Failure("occupancyLost", "Session expired")); pending.reset(); }
			// Ticket 31 AC4: drain disconnect notices before new work so a
			// vanished client's occupancy is released before its successor's
			// reattach is dispatched.
			if(broker)
				while(auto notice = broker->PopDisconnect())
					HandleDisconnect(*notice);
			if(broker && !pending)
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
			GetDlgItem(Approve)->EnableWindow(pending != nullptr); GetDlgItem(Decline)->EnableWindow(pending != nullptr);
			GetDlgItem(Apply)->EnableWindow(capability && capability->HasProposal()); GetDlgItem(Reject)->EnableWindow(capability && capability->HasProposal());
			InvalidateRect(stateRect, FALSE);
		} catch(...) { lastMessage = _T("AI service error."); }
	}
	afx_msg void OnPaint()
	{
		CPaintDC dc(this);
		dc.FillSolidRect(stateRect, GetSysColor(COLOR_WINDOW));
		CString state = broker && broker->Running() ? _T("MCP ready") : _T("MCP stopped / starting");
		if(capability && capability->Occupied()) state += pending ? _T(" | AI OCCUPIED - expansion approval waiting (timer paused)") : _T(" | AI OCCUPIED - navigation and playback available; writes blocked");
		if(review.is_object() && review.contains("status")) state += _T(" | Proposal ") + Text(review["status"].get<std::string>());
		dc.TextOut(stateRect.left, stateRect.top + 1, state);
		if(pending && capability)
		{
			const auto range = capability->ExpansionRange();
			CString grant; grant.Format(_T("Requested: rows %d-%d, channel %d. Current grant: rows %d-%d, channels %d-%d (1-based channels)."),
				range["first_row"].get<int>(), range["last_row"].get<int>(), range["channel"].get<int>() + 1,
				range["grant_first_row"].get<int>(), range["grant_last_row"].get<int>(), range["grant_first_channel"].get<int>() + 1, range["grant_last_channel"].get<int>() + 1);
			dc.TextOut(stateRect.left, stateRect.top + 23, grant);
		} else dc.TextOut(stateRect.left, stateRect.top + 23, lastMessage);
		DrawEvidence(dc);
	}
	void DrawEvidence(CDC &dc)
	{
		dc.FillSolidRect(evidenceRect, RGB(25, 28, 35));
		if(!review.is_object() || !review.contains("diff") || review["diff"].empty()) return;
		const auto &diff = review["diff"];
		int first = INT_MAX, last = 0, low = 128, high = 1;
		for(const auto &cell : diff)
		{
			first = std::min(first, cell["row"].get<int>()); last = std::max(last, cell["row"].get<int>());
			for(const char *version : {"before", "after"}) { int n = cell[version]["note"].get<int>(); if(n >= 1 && n <= 128) { low = std::min(low, n); high = std::max(high, n); } }
			if(const auto live = CurrentCell(cell); live && live->IsNote()) { low = std::min<int>(low, live->note); high = std::max<int>(high, live->note); }
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
				if(projection == 2) { const auto live = CurrentCell(cell); note = live ? live->note : 0; }
				int px = x + (cell["row"].get<int>() - first) * std::max(1, column - 30) / std::max(1, last - first + 1);
				if(note >= 1 && note <= 128)
				{
					int py = plotBottom - (note - low) * plotHeight / std::max(1, high - low);
					dc.FillSolidRect(CRect(px, py, px + 7, py + 6), RGB(100, 180, 240));
				} else dc.FillSolidRect(CRect(px, plotBottom - 5, px + 5, plotBottom), RGB(240, 165, 70));
			}
		}
	}
	DECLARE_MESSAGE_MAP()
};
BEGIN_MESSAGE_MAP(Panel, CWnd)
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_WM_PAINT()
END_MESSAGE_MAP()
std::unique_ptr<Panel> panel;
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

void Start(CWnd &owner) { if(!panel) panel = std::make_unique<Panel>(owner); }
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
void Stop() { panel.reset(); }
bool AttachPanel(CWnd &host, const CRect &rect)
{
	if(!panel || !panel->GetSafeHwnd() || !host.GetSafeHwnd()) return false;
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
	return panel && panel->capability && panel->capability->Occupied() && !IsReadOnlyCommand(command);
}

bool FilterInput(MSG &msg)
{
	if(!panel || !panel->capability || !panel->capability->Occupied()) return false;
	if(msg.hwnd == panel->m_hWnd || ::IsChild(panel->m_hWnd, msg.hwnd)) return false;
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
			if(view->GetPianoRollPrototypeRect().PtInRect(point)) { view->HandlePianoRollPrototypeLButtonDown(0, point); view->AICancelPianoRollDrag(); panel->selecting = false; panel->keyboardSelecting = false; panel->selectionAnchor = PatternCursor(view->GetCurrentRow(), view->GetCurrentChannel()); }
			else { auto cursor = view->GetPositionFromPoint(point); cursor.Sanitize(panel->document->GetSoundFile().Patterns[view->GetCurrentPattern()].GetNumRows(), panel->document->GetSoundFile().GetNumChannels()); view->SetCursorPosition(cursor); view->SetCurSel(cursor); panel->selectionAnchor = cursor; panel->selecting = true; panel->keyboardSelecting = false; }
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
