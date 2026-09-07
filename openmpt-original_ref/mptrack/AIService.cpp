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
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>

OPENMPT_NAMESPACE_BEGIN
namespace AI
{
namespace
{
constexpr DWORD MaxFrame = 4 * 1024 * 1024;
std::string UTF8(const CString &s) { return mpt::ToCharset(mpt::Charset::UTF8, mpt::ToUnicode(s)); }
CString Text(const std::string &s) { return mpt::ToCString(mpt::ToUnicode(mpt::Charset::UTF8, s)); }

struct Request
{
	Json envelope, response;
	std::mutex mutex;
	std::condition_variable ready;
	bool done = false;
	void Complete(Json result)
	{
		std::lock_guard lock(mutex);
		response = std::move(result);
		done = true;
		ready.notify_one();
	}
};

// The broker only handles bytes and envelopes. It has no document pointers.
class Broker
{
	HANDLE m_stop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	std::thread m_worker;
	std::mutex m_mutex;
	std::deque<std::shared_ptr<Request>> m_queue;
	std::wstring m_name;
	std::atomic<bool> m_running = false;
	bool IO(HANDLE pipe, void *buffer, DWORD size, bool write)
	{
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
		if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return;
		DWORD size = 0;
		GetTokenInformation(token, TokenUser, nullptr, 0, &size);
		std::vector<char> info(size);
		bool valid = GetTokenInformation(token, TokenUser, info.data(), size, &size) != FALSE;
		CloseHandle(token);
		LPWSTR sid = nullptr;
		if(!valid || !ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER *>(info.data())->User.Sid, &sid)) return;
		const std::wstring acl = L"D:P(A;;GA;;;" + std::wstring(sid) + L")";
		LocalFree(sid);
		if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(acl.c_str(), SDDL_REVISION_1, &descriptor, nullptr)) return;
		SECURITY_ATTRIBUTES security{sizeof(security), descriptor, FALSE};
		HANDLE pipe = CreateNamedPipeW(m_name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, 1, 65536, 65536, 0, &security);
		LocalFree(descriptor);
		if(pipe == INVALID_HANDLE_VALUE) return;
		m_running = true;
		while(WaitForSingleObject(m_stop, 0) != WAIT_OBJECT_0)
		{
			OVERLAPPED connect{};
			connect.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
			BOOL connected = ConnectNamedPipe(pipe, &connect);
			DWORD error = connected ? ERROR_SUCCESS : GetLastError();
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
					else
					{
						auto request = std::make_shared<Request>();
						request->envelope = envelope;
						{ std::lock_guard lock(m_mutex); m_queue.push_back(request); }
						std::unique_lock lock(request->mutex);
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
			DisconnectNamedPipe(pipe);
		}
		m_running = false;
		CloseHandle(pipe);
	}
public:
	Broker(std::wstring name) : m_name(std::move(name)) { m_worker = std::thread([this] { try { Run(); } catch(...) { m_running = false; } }); }
	~Broker() { SetEvent(m_stop); if(m_worker.joinable()) m_worker.join(); CloseHandle(m_stop); }
	bool Running() const { return m_running; }
	std::shared_ptr<Request> Pop()
	{
		std::lock_guard lock(m_mutex);
		if(m_queue.empty()) return {};
		auto result = m_queue.front(); m_queue.pop_front(); return result;
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
	std::unique_ptr<Broker> broker;
	std::unique_ptr<PatternCapability> capability;
	CModDoc *document = nullptr;
	std::shared_ptr<Request> pending;
	CButton enable, always;
	CEdit timeout, identity;
	CListBox evidence;
	std::vector<std::unique_ptr<CButton>> buttons;
	std::string instance;
	std::wstring pipe;
	Json review;
	unsigned seconds = 300;
	bool expanded = false;
	CString lastMessage;
	Panel(CWnd &owner)
	{
		GUID guid{}; CoCreateGuid(&guid); wchar_t id[40]{}; StringFromGUID2(guid, id, 40);
		instance = UTF8(id);
		pipe = L"\\\\.\\pipe\\OpenMPT-AI-" + std::to_wstring(GetCurrentProcessId()) + L"-" + id;
		CreateEx(WS_EX_TOOLWINDOW, AfxRegisterWndClass(0, LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1)),
			_T("AI / MCP - Pattern collaboration"), WS_OVERLAPPEDWINDOW, CRect(120, 100, 1220, 850), &owner, 0);
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
		if(enable.GetCheck()) broker = std::make_unique<Broker>(pipe);
		SetTimer(1, 100, nullptr);
		RefreshIdentity();
	}
	~Panel() { if(pending) pending->Complete(Failure("instanceGone", "Application stopping", "attachment")); capability.reset(); broker.reset(); }
	void RefreshIdentity()
	{
		auto *active = CMainFrame::GetMainFrame()->GetActiveDoc();
		CString value = _T("Pipe: ") + CString(pipe.c_str()) + _T("\r\nInstance: ") + Text(instance);
		value += _T("\r\nActive document: ");
		if(active) value += Text(active->AIIdentity()) + _T(" | ") + active->GetTitle();
		else value += _T("Open a document and its Patterns tab");
		value += _T("\r\nSession binds the displayed document's Pattern and selection on the first read.");
		identity.SetWindowText(value);
	}
	void ReleaseNow()
	{
		if(capability) capability->ForceRelease();
		if(pending) { pending->Complete(Failure("occupancyLost", "Human released the session")); pending.reset(); }
	}
	Json Dispatch(const Json &envelope)
	{
		if(!theApp.InGuiThread()) return Failure("owningThreadRequired", "Use document owning thread", "dispatch");
		if(envelope.at("instance") != instance) return Failure("instanceGone", "Instance identity no longer exists", "attachment");
		CModDoc *target = nullptr;
		for(auto *doc : theApp.GetOpenDocuments()) if(envelope.at("document") == doc->AIIdentity()) { target = doc; break; }
		if(!target) return Failure("documentGone", "Document lifetime ended", "attachment");
		if(envelope.at("operation") == "attach") return {{"ok", true}};
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
	void RefreshReview()
	{
		review = capability && capability->HasProposal() ? capability->Review() : Json{};
		evidence.ResetContent();
		if(review.is_object() && review.value("ok", false))
		{
			for(const auto &cell : review.at("diff"))
			{
				const int row = cell.at("row").get<int>(), channel = cell.at("channel").get<int>();
				const auto &sf = document->GetSoundFile();
				const auto &live = *sf.Patterns[capability->Pattern()].GetpModCommand(static_cast<ROWINDEX>(row), static_cast<CHANNELINDEX>(channel));
				CString line; line.Format(_T("Row %03d Ch %02d  |  %s  ->  %s  |  current %s"), row, channel + 1,
					mpt::ToCString(sf.GetNoteName(cell["before"]["note"].get<uint8>(), cell["before"]["instrument"].get<uint8>())).GetString(),
					mpt::ToCString(sf.GetNoteName(cell["after"]["note"].get<uint8>(), cell["after"]["instrument"].get<uint8>())).GetString(), mpt::ToCString(sf.GetNoteName(live.note, live.instr)).GetString());
				line += _T("  ") + Text(cell["before"].dump()) + _T(" -> ") + Text(cell["after"].dump());
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
				else if(!broker) broker = std::make_unique<Broker>(pipe);
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
			if(capability) capability->Tick();
			if(pending && capability && !capability->Occupied()) { pending->Complete(Failure("occupancyLost", "Session expired")); pending.reset(); }
			if(broker && !pending)
				if(auto request = broker->Pop())
				{
					Json result;
					try { result = Dispatch(request->envelope); }
					catch(const Json::exception &) { result = Failure("schemaFailure", "Invalid request", "transport"); }
					catch(const std::exception &) { result = Failure("internalError", "Application capability failed"); }
					if(result.value("pending_approval", false)) pending = request;
					else request->Complete(result);
					if(capability && (capability->Occupied() || capability->HasProposal())) ShowWindow(SW_SHOWNOACTIVATE);
					RefreshReview();
				}
			GetDlgItem(Approve)->EnableWindow(pending != nullptr); GetDlgItem(Decline)->EnableWindow(pending != nullptr);
			GetDlgItem(Apply)->EnableWindow(capability && capability->HasProposal()); GetDlgItem(Reject)->EnableWindow(capability && capability->HasProposal());
			InvalidateRect(CRect(12, 215, 1040, 260), FALSE);
		} catch(...) { lastMessage = _T("AI service error."); }
	}
	afx_msg void OnClose()
	{
		if(capability && capability->Occupied()) { lastMessage = _T("Release AI before hiding this window."); return; }
		ShowWindow(SW_HIDE);
	}
	afx_msg void OnPaint()
	{
		CPaintDC dc(this);
		dc.FillSolidRect(CRect(12, 215, 1040, 260), GetSysColor(COLOR_WINDOW));
		CString state = broker && broker->Running() ? _T("MCP ready") : _T("MCP stopped / starting");
		if(capability && capability->Occupied()) state += pending ? _T(" | AI OCCUPIED - expansion approval waiting (timer paused)") : _T(" | AI OCCUPIED - navigation and playback available; writes blocked");
		if(review.is_object() && review.contains("status")) state += _T(" | Proposal ") + Text(review["status"].get<std::string>());
		dc.TextOut(12, 216, state); dc.TextOut(12, 238, lastMessage);
		DrawEvidence(dc);
	}
	void DrawEvidence(CDC &dc)
	{
		dc.FillSolidRect(CRect(12, 475, 1040, 680), RGB(25, 28, 35));
		if(!review.is_object() || !review.contains("diff") || review["diff"].empty()) return;
		const auto &diff = review["diff"];
		int first = INT_MAX, last = 0, low = 128, high = 1;
		for(const auto &cell : diff)
		{
			first = std::min(first, cell["row"].get<int>()); last = std::max(last, cell["row"].get<int>());
			for(const char *version : {"before", "after"}) { int n = cell[version]["note"].get<int>(); if(n >= 1 && n <= 128) { low = std::min(low, n); high = std::max(high, n); } }
		}
		if(low > high) { low = 48; high = 72; }
		for(int projection = 0; projection < 3; ++projection)
		{
			const int x = 20 + projection * 340;
			dc.SetTextColor(RGB(230, 235, 240)); dc.SetBkMode(TRANSPARENT);
			dc.TextOut(x, 480, projection == 0 ? _T("Baseline") : projection == 1 ? _T("Proposal") : _T("Current document"));
			for(const auto &cell : diff)
			{
				int note = cell[projection == 0 ? "before" : "after"]["note"].get<int>();
				if(projection == 2 && document && document->GetSoundFile().Patterns.IsValidPat(capability->Pattern())) note = document->GetSoundFile().Patterns[capability->Pattern()].GetpModCommand(cell["row"].get<ROWINDEX>(), cell["channel"].get<CHANNELINDEX>())->note;
				int px = x + (cell["row"].get<int>() - first) * 310 / std::max(1, last - first + 1);
				if(note >= 1 && note <= 128)
				{
					int py = 645 - (note - low) * 120 / std::max(1, high - low);
					dc.FillSolidRect(CRect(px, py, px + 7, py + 6), RGB(100, 180, 240));
				} else dc.FillSolidRect(CRect(px, 658, px + 5, 663), RGB(240, 165, 70));
			}
		}
	}
	DECLARE_MESSAGE_MAP()
};
BEGIN_MESSAGE_MAP(Panel, CWnd)
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_WM_PAINT()
END_MESSAGE_MAP()
std::unique_ptr<Panel> panel;
}

void Start(CWnd &owner) { if(!panel) panel = std::make_unique<Panel>(owner); }
void Stop() { panel.reset(); }
void ShowPanel() { if(panel) { panel->RefreshIdentity(); panel->ShowWindow(SW_SHOW); panel->SetForegroundWindow(); } }
void DocumentClosed(CModDoc &doc)
{
	if(panel && panel->document == &doc) { panel->ReleaseNow(); panel->capability.reset(); panel->document = nullptr; panel->RefreshReview(); }
}

bool BlockCommand(UINT command)
{
	if(!panel || !panel->capability || !panel->capability->Occupied()) return false;
	switch(command)
	{
	case ShowPanelCommand: case ID_PLAYER_PLAY: case ID_PLAYER_STOP: case ID_PLAYER_PAUSE:
	case ID_PATTERN_PLAY: case ID_PATTERN_PLAYNOLOOP: case ID_PATTERN_RESTART:
	case ID_VIEW_PATTERNS: case ID_VIEW_SAMPLES: case ID_VIEW_INSTRUMENTS: case ID_VIEW_GLOBALS: case ID_VIEW_COMMENTS:
		return false;
	default: return true;
	}
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
			view->SetCursorPosition(PatternCursor(static_cast<ROWINDEX>(std::max(0, row)), static_cast<CHANNELINDEX>(std::clamp(channel, 0, int(panel->document->GetSoundFile().GetNumChannels()) - 1))));
			view->SetSelToCursor(); return true;
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
			if(view->GetPianoRollPrototypeRect().PtInRect(point)) view->HandlePianoRollPrototypeLButtonDown(0, point);
			else { auto cursor = view->GetPositionFromPoint(point); view->SetCursorPosition(cursor); view->SetCurSel(cursor); }
		}
		return true;
	}
	return false;
}
}
OPENMPT_NAMESPACE_END
