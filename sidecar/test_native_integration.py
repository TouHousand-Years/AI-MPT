"""Opt-in real process/pipe tests; no human Apply, playback or Save actions."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest
import ctypes
from ctypes import wintypes
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, str(Path(__file__).resolve().parent))
import probe

ROOT = Path(__file__).resolve().parent.parent
EXE = Path(os.environ.get(
    "OPENMPT_TEST_EXE",
    ROOT / "openmpt-original_ref/bin/debug/vs2022-win10-static/amd64/OpenMPT.exe",
))


@unittest.skipUnless(os.environ.get("OPENMPT_RUN_NATIVE_INTEGRATION") == "1", "Opt-in native executable integration")
class NativeIntegrationTests(unittest.TestCase):
    def control(self, app, caption, class_name=False):
        user = ctypes.WinDLL("user32", use_last_error=True)
        callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
        user.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
        user.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
        user.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
        user.EnumWindows.argtypes = [callback_type, wintypes.LPARAM]
        user.EnumChildWindows.argtypes = [wintypes.HWND, callback_type, wintypes.LPARAM]
        found = []
        @callback_type
        def child(hwnd, _):
            text = ctypes.create_unicode_buffer(256)
            (user.GetClassNameW if class_name else user.GetWindowTextW)(hwnd, text, 256)
            if text.value == caption:
                found.append(hwnd)
            return True
        @callback_type
        def top(hwnd, _):
            pid = wintypes.DWORD()
            user.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
            if pid.value == app.pid:
                user.EnumChildWindows(hwnd, child, 0)
            return True
        user.EnumWindows(top, 0)
        self.assertTrue(found, f"Missing native control: {caption}")
        return found[0]

    def activate_page(self, app, page):
        user = ctypes.WinDLL("user32", use_last_error=True)
        user.GetParent.argtypes = [wintypes.HWND]
        user.GetParent.restype = wintypes.HWND
        user.GetAncestor.argtypes = [wintypes.HWND, wintypes.UINT]
        user.GetAncestor.restype = wintypes.HWND
        user.SetForegroundWindow.argtypes = [wintypes.HWND]
        user.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
        tab = self.control(app, "SysTabControl32", class_name=True)
        user.SetForegroundWindow(user.GetAncestor(tab, 2))
        # WM_MOD_ACTIVATEVIEW; -1 restores the page without selecting an Order.
        user.SendMessageW(user.GetParent(tab), 1024 + 1975, page, -1)
        time.sleep(0.3)

    def click_control(self, app, caption):
        user = ctypes.WinDLL("user32", use_last_error=True)
        user.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
        user.SendMessageW.restype = wintypes.LPARAM
        user.SendMessageW(self.control(app, caption), 0x00F5, 0, 0)  # BM_CLICK

    def checked_control(self, app, caption):
        user = ctypes.WinDLL("user32", use_last_error=True)
        user.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
        user.SendMessageW.restype = wintypes.LPARAM
        return bool(user.SendMessageW(self.control(app, caption), 0x00F0, 0, 0))

    def test_manual_switch_and_enable_automatic_acceptance(self):
        with tempfile.TemporaryDirectory(prefix="openmpt-ai-switch-") as directory:
            report, stop = Path(directory) / "endpoint.json", Path(directory) / "stop"
            app, _, endpoint = self.start_app(report, stop, 0)
            client = probe.PipeClient(endpoint["pipe"])
            preferences = {}
            try:
                client.connect()
                client.transact(probe.envelope(endpoint["instance"], endpoint["document"], "attach"))
                def invoke(tool, arguments):
                    return client.transact(probe.envelope(endpoint["instance"], endpoint["document"], "call", tool=tool, arguments=arguments))
                for caption in ("Always allow Pattern switching", "Always accept submissions"):
                    preferences[caption] = self.checked_control(app, caption)
                    if preferences[caption]:
                        self.click_control(app, caption)
                self.activate_page(app, 49001)  # AI::PanelPageId
                with ThreadPoolExecutor(max_workers=1) as pool:
                    future = pool.submit(invoke, "switch_pattern", {"pattern": 1})
                    time.sleep(0.4)
                    self.assertFalse(future.done(), "Manual switch must wait for the human")
                    self.click_control(app, "Approve Pattern switch")
                    switched = future.result(timeout=5)
                self.assertTrue(switched["ok"], switched)
                self.assertEqual(switched["source"]["pattern"], 0)
                self.assertEqual(switched["context"]["pattern"], 1)
                token = switched["session"]
                segment = {"session": token, "channel": 2, "first_row": 0, "row_count": 1,
                           "cells": [{"row": 0, "cell": {"note": 60, "instrument": 2, "volume_command": 1,
                                                        "volume": 40, "effect_command": 0, "effect_parameter": 0}}]}
                self.assertTrue(invoke("replace_pattern_segment", segment)["ok"])
                self.assertEqual(invoke("handoff_for_review", {"session": token})["status"], "pending_review")
                self.click_control(app, "Always accept submissions")
                self.activate_page(app, 116)  # IDD_CONTROL_PATTERNS
                read = invoke("get_pattern_context", {})
                self.assertTrue(read["ok"], read)
                self.assertEqual(read["context"]["pattern"], 1)
                self.assertTrue(any(c["row"] == 0 and c["channel"] == 2 and c["raw"]["note"] == 60 for c in read["context"]["cells"]))
                self.click_control(app, "Always accept submissions")
            finally:
                client.close()
                for caption, checked in preferences.items():
                    if self.checked_control(app, caption) != checked:
                        self.click_control(app, caption)
                self.stop_app(app, stop)

    def test_order_read_before_session_and_from_another_connection(self):
        with tempfile.TemporaryDirectory(prefix="openmpt-ai-order-") as directory:
            report, stop = Path(directory) / "endpoint.json", Path(directory) / "stop"
            app, _, endpoint = self.start_app(report, stop, 0)
            clients = [probe.PipeClient(endpoint["pipe"]) for _ in range(2)]
            try:
                for client in clients:
                    client.connect()
                    self.assertTrue(client.transact(probe.envelope(endpoint["instance"], endpoint["document"], "attach"))["ok"])
                def invoke(client, tool, arguments):
                    return client.transact(probe.envelope(endpoint["instance"], endpoint["document"], "call", tool=tool, arguments=arguments))
                order = invoke(clients[0], "get_pattern_order", {})
                self.assertTrue(order["ok"], order)
                self.assertEqual(order["entries"][0]["pattern"], 0)
                retained = invoke(clients[0], "get_pattern_context", {"occupy": True})
                self.assertTrue(retained["ok"], retained)
                other = invoke(clients[1], "get_pattern_order", {})
                self.assertTrue(other["ok"], other)
                self.assertTrue(invoke(clients[0], "release_occupancy", {"session": retained["session"]})["ok"])
            finally:
                for client in clients:
                    client.close()
                self.stop_app(app, stop)

    def test_always_switch_without_selection_defaults_to_first_order_pattern(self):
        with tempfile.TemporaryDirectory(prefix="openmpt-ai-default-pattern-") as directory:
            report, stop = Path(directory) / "endpoint.json", Path(directory) / "stop"
            # Display Pattern 1 first: the fallback must still follow Order, whose
            # first valid entry in the fixture is Pattern 0.
            app, _, endpoint = self.start_app(report, stop, 1)
            client = probe.PipeClient(endpoint["pipe"])
            preference = None
            try:
                preference = self.checked_control(app, "Always allow Pattern switching")
                if not preference:
                    self.click_control(app, "Always allow Pattern switching")
                self.activate_page(app, 49001)  # No live Patterns view or drawn selection.
                client.connect()
                self.assertTrue(client.transact(probe.envelope(endpoint["instance"], endpoint["document"], "attach"))["ok"])
                result = client.transact(probe.envelope(
                    endpoint["instance"], endpoint["document"], "call",
                    tool="get_pattern_context", arguments={}))
                self.assertTrue(result["ok"], result)
                self.assertEqual(result["context"]["pattern"], 0)
                self.assertEqual(result["context"]["range"], {
                    "first_row": 0, "row_count": result["context"]["rows"],
                    "first_channel": 0, "channel_count": result["context"]["channels"],
                })
            finally:
                client.close()
                if preference is not None and self.checked_control(app, "Always allow Pattern switching") != preference:
                    self.click_control(app, "Always allow Pattern switching")
                self.stop_app(app, stop)

    def test_switch_wait_rejection_release_and_document_close(self):
        for action in ("reject", "release", "close"):
            with self.subTest(action=action), tempfile.TemporaryDirectory(prefix="openmpt-ai-wait-") as directory:
                report, stop = Path(directory) / "endpoint.json", Path(directory) / "stop"
                app, _, endpoint = self.start_app(report, stop, 0)
                owner, observer = probe.PipeClient(endpoint["pipe"]), probe.PipeClient(endpoint["pipe"])
                preference = None
                pool = ThreadPoolExecutor(max_workers=1)
                try:
                    preference = self.checked_control(app, "Always allow Pattern switching")
                    if preference:
                        self.click_control(app, "Always allow Pattern switching")
                    for client in (owner, observer):
                        client.connect()
                        client.transact(probe.envelope(endpoint["instance"], endpoint["document"], "attach"))
                    def invoke(client, tool, args, **extra):
                        return client.transact(probe.envelope(endpoint["instance"], endpoint["document"], "call", tool=tool, arguments=args, **extra))
                    retained = invoke(owner, "get_pattern_context", {"occupy": True})
                    self.assertTrue(retained["ok"], retained)
                    token = retained["session"]
                    future = pool.submit(invoke, owner, "switch_pattern", {"pattern": 1, "session": token})
                    time.sleep(0.4)
                    self.assertFalse(future.done())
                    blocked = invoke(observer, "switch_pattern", {"pattern": 1, "session": token})
                    self.assertEqual(blocked["error"]["code"], "busy")
                    if action == "reject":
                        self.click_control(app, "Reject Pattern switch")
                        self.assertEqual(future.result(timeout=5)["error"]["code"], "patternSwitchRejected")
                        read = invoke(owner, "get_pattern_context", {"session": token})
                        self.assertEqual(read["context"]["pattern"], 0)
                        invoke(owner, "abort_session", {"session": token})
                    elif action == "release":
                        self.click_control(app, "RELEASE AI NOW")
                        self.assertEqual(future.result(timeout=5)["error"]["code"], "occupancyLost")
                        self.assertTrue(invoke(observer, "get_pattern_context", {})["ok"])
                    elif action == "close":
                        result = invoke(observer, "get_pattern_order", {}, test_close_before_dispatch=True)
                        self.assertEqual(result["error"]["code"], "documentGone")
                        self.assertEqual(future.result(timeout=5)["error"]["code"], "documentGone")
                finally:
                    observer.close()
                    if preference is not None and self.checked_control(app, "Always allow Pattern switching") != preference:
                        self.click_control(app, "Always allow Pattern switching")
                    self.stop_app(app, stop)
                    pool.shutdown(wait=True)
                    owner.close()

    def test_disconnected_switch_request_releases_reservation(self):
        with tempfile.TemporaryDirectory(prefix="openmpt-ai-disconnect-") as directory:
            report, stop = Path(directory) / "endpoint.json", Path(directory) / "stop"
            app, _, endpoint = self.start_app(report, stop, 0)
            child = None
            observer = probe.PipeClient(endpoint["pipe"])
            preference = None
            try:
                preference = self.checked_control(app, "Always allow Pattern switching")
                if preference:
                    self.click_control(app, "Always allow Pattern switching")
                child = subprocess.Popen([sys.executable, "-c",
                    "import json,sys; from openmpt_mcp import Sidecar; e=json.loads(sys.argv[1]); "
                    "s=Sidecar(e['pipe'],e['instance'],e['document']); "
                    "s.call('switch_pattern',{'pattern':1})", json.dumps(endpoint)],
                    cwd=ROOT / "sidecar", stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
                time.sleep(0.5)
                self.assertIsNone(child.poll(), "Switch must be waiting when client exits")
                observer.connect()
                observer.transact(probe.envelope(endpoint["instance"], endpoint["document"], "attach"))
                def read():
                    return observer.transact(probe.envelope(endpoint["instance"], endpoint["document"], "call", tool="get_pattern_context", arguments={}))
                self.assertEqual(read()["error"]["code"], "busy")
                child.kill()
                child.wait(timeout=5)
                result = read()
                deadline = time.monotonic() + 5
                while not result["ok"] and time.monotonic() < deadline:
                    time.sleep(0.1)
                    result = read()
                self.assertTrue(result["ok"], result)
            finally:
                if child:
                    if child.poll() is None:
                        child.kill()
                        child.wait(timeout=5)
                    child.stderr.close()
                observer.close()
                if preference is not None and self.checked_control(app, "Always allow Pattern switching") != preference:
                    self.click_control(app, "Always allow Pattern switching")
                self.stop_app(app, stop)

    def start_app(self, report, stop, pattern):
        env = dict(os.environ, OPENMPT_AI_TEST_FIXTURE=str(ROOT / "test-fixtures/ai-collab-fixture.mptm"),
                   OPENMPT_AI_ENDPOINT_REPORT=str(report), OPENMPT_AI_STOP_FILE=str(stop),
                   OPENMPT_AI_PATTERN=str(pattern))
        startup = subprocess.STARTUPINFO()
        startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = 0
        app = subprocess.Popen([str(EXE), "/noSysCheck", "/noTests", "/noPlugins", "/noDls"],
                               env=env, startupinfo=startup)
        deadline = time.monotonic() + 30
        while not report.exists() and time.monotonic() < deadline and app.poll() is None:
            time.sleep(0.1)
        trace = Path(str(report) + ".trace")
        if not report.exists():
            app.kill()
            app.wait()
            self.fail(f"No app endpoint, exit={app.returncode}, trace={trace.read_text() if trace.exists() else 'none'}")
        return app, trace, json.loads(report.read_text())

    def assert_audio_path_clean(self, trace):
        text = trace.read_text()
        self.assertNotIn("VIOLATION", text)
        # The instrumented line proves the check itself ran on the realtime
        # audio thread while IPC traffic was flowing; it requires a sound device.
        self.assertIn("audio-callback: IPC check instrumented", text)

    def stop_app(self, app, stop, sidecar=None):
        if sidecar:
            sidecar.stdin.close()
            try:
                sidecar.wait(timeout=5)
            except subprocess.TimeoutExpired:
                sidecar.kill()
                sidecar.wait()
            sidecar.stdout.close()
            sidecar.stderr.close()
        stop.touch()
        try:
            app.wait(timeout=10)
        except subprocess.TimeoutExpired:
            app.kill()
            app.wait()

    def exchange_sidecar(self, endpoint, calls):
        messages = [
            {"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {
                "protocolVersion": "2025-11-25", "capabilities": {},
                "clientInfo": {"name": "native-restart-test", "version": "1"}}},
            {"jsonrpc": "2.0", "method": "notifications/initialized"},
        ]
        for request_id, (name, arguments) in enumerate(calls, 2):
            messages.append({"jsonrpc": "2.0", "id": request_id, "method": "tools/call",
                             "params": {"name": name, "arguments": arguments}})
        process = subprocess.run(
            [sys.executable, str(ROOT / "sidecar/openmpt_mcp.py"),
             "--pipe", endpoint["pipe"], "--instance", endpoint["instance"],
             "--document", endpoint["document"]],
            input="".join(json.dumps(message) + "\n" for message in messages),
            text=True, encoding="utf-8", capture_output=True, timeout=15,
        )
        self.assertEqual(process.returncode, 0, process.stderr)
        return [json.loads(line)["result"] for line in process.stdout.splitlines()][1:]

    def test_sidecar_restart_releases_retained_occupancy(self):
        """Issue 31 AC4: process loss leaves no app state that blocks reattach."""
        with tempfile.TemporaryDirectory(prefix="openmpt-ai-") as directory:
            report = Path(directory) / "endpoint.json"
            stop = Path(directory) / "stop"
            app, _, endpoint = self.start_app(report, stop, 0)
            try:
                first = self.exchange_sidecar(endpoint, [("get_pattern_context", {"occupy": True})])
                self.assertTrue(first[0]["structuredContent"]["ok"], first)

                # EOF terminates the first client-supervised Sidecar. A fresh
                # process must explicitly attach and be able to occupy again.
                second = self.exchange_sidecar(endpoint, [("get_pattern_context", {})])
                result = second[0]["structuredContent"]
                self.assertTrue(result["ok"], result)
            finally:
                self.stop_app(app, stop)

    def test_concurrent_read_only_connections_and_retained_owner(self):
        """Readers coexist; retained state belongs to one exact connection."""
        with tempfile.TemporaryDirectory(prefix="openmpt-ai-") as directory:
            report = Path(directory) / "endpoint.json"
            stop = Path(directory) / "stop"
            app, _, endpoint = self.start_app(report, stop, 0)
            clients = [probe.PipeClient(endpoint["pipe"]) for _ in range(2)]
            try:
                for client in clients:
                    self.assertIsNone(client.connect(), "concurrent pipe open failed")
                    attached = client.transact(probe.envelope(endpoint["instance"], endpoint["document"], "attach"))
                    self.assertTrue(attached.get("ok"), attached)

                for client in clients:
                    result = client.transact(probe.envelope(endpoint["instance"], endpoint["document"], "call",
                                                            tool="get_pattern_context", arguments={}))
                    self.assertTrue(result.get("ok"), result)

                retained = clients[0].transact(probe.envelope(endpoint["instance"], endpoint["document"], "call",
                                                               tool="get_pattern_context", arguments={"occupy": True}))
                self.assertTrue(retained.get("ok"), retained)
                blocked = clients[1].transact(probe.envelope(endpoint["instance"], endpoint["document"], "call",
                                                              tool="get_pattern_context", arguments={}))
                self.assertEqual(blocked["error"]["code"], "busy")
                stolen = clients[1].transact(probe.envelope(endpoint["instance"], endpoint["document"], "call",
                                                             tool="get_pattern_context",
                                                             arguments={"session": retained["session"]}))
                self.assertEqual(stolen["error"]["code"], "busy")

                # Closing an unrelated reader cannot release the retained owner.
                clients[1].close()
                owner_read = clients[0].transact(probe.envelope(endpoint["instance"], endpoint["document"], "call",
                                                                 tool="get_pattern_context",
                                                                 arguments={"session": retained["session"], "baseline": True}))
                self.assertTrue(owner_read.get("ok"), owner_read)
            finally:
                for client in clients:
                    client.close()
                self.stop_app(app, stop)

    def test_both_directions_through_real_sidecar_and_app(self):
        for pattern, channels in ((0, [1, 2]), (1, [0])):
            with self.subTest(pattern=pattern), tempfile.TemporaryDirectory(prefix="openmpt-ai-") as directory:
                report = Path(directory) / "endpoint.json"
                stop = Path(directory) / "stop"
                app, trace, endpoint = self.start_app(report, stop, pattern)
                sidecar = None
                try:
                    sidecar = subprocess.Popen([sys.executable, str(ROOT / "sidecar/openmpt_mcp.py"),
                                                "--pipe", endpoint["pipe"], "--instance", endpoint["instance"],
                                                "--document", endpoint["document"]],
                                               stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                               text=True, encoding="utf-8")
                    counter = 0

                    def request(method, params=None):
                        nonlocal counter
                        counter += 1
                        sidecar.stdin.write(json.dumps({"jsonrpc": "2.0", "id": counter, "method": method, "params": params or {}}) + "\n")
                        sidecar.stdin.flush()
                        reply = json.loads(sidecar.stdout.readline())
                        self.assertEqual(reply["id"], counter)
                        return reply["result"]

                    def call(name, arguments):
                        return request("tools/call", {"name": name, "arguments": arguments})["structuredContent"]

                    request("initialize", {"protocolVersion": "2025-11-25", "capabilities": {},
                                           "clientInfo": {"name": "native-boundary-test", "version": "1"}})
                    sidecar.stdin.write('{"jsonrpc":"2.0","method":"notifications/initialized"}\n')
                    sidecar.stdin.flush()
                    self.assertEqual(len(request("tools/list")["tools"]), 7)
                    baseline = call("get_pattern_context", {"occupy": True})
                    self.assertTrue(baseline["ok"], baseline)
                    self.assertEqual(baseline["context"]["pattern"], pattern)
                    self.assertEqual(baseline["context"]["rows"], 128)
                    token = baseline["session"]
                    for channel in channels:
                        cell = dict(note=61 if pattern else 49 + channel * 4, instrument=1 if pattern else 2,
                                    volume_command=1, volume=40, effect_command=0, effect_parameter=0)
                        result = call("replace_pattern_segment", dict(session=token, channel=channel, first_row=0,
                                                                       row_count=8, cells=[dict(row=0, cell=cell)]))
                        self.assertTrue(result["ok"], result)
                        self.assertEqual(len(result["diff"]), 1)
                    self.assertEqual(call("get_pattern_context", {"session": token, "baseline": True})["context"], baseline["context"])
                    proposal = call("handoff_for_review", {"session": token})
                    self.assertTrue(proposal["ok"], proposal)
                    self.assertEqual(len(proposal["diff"]), len(channels))
                    self.assertEqual(call("get_pattern_context", {"session": token})["error"]["code"], "occupancyLost")
                    self.assert_audio_path_clean(trace)
                finally:
                    self.stop_app(app, stop, sidecar)

    def test_guided_failure_cases_from_real_transport(self):
        """Issue 30 AC2/AC3: typed failures from the real pipe, app exit → instanceGone."""
        with tempfile.TemporaryDirectory(prefix="openmpt-ai-") as directory:
            report = Path(directory) / "endpoint.json"
            stop = Path(directory) / "stop"
            app, trace, endpoint = self.start_app(report, stop, 0)
            try:
                pipe_name, instance, document = endpoint["pipe"], endpoint["instance"], endpoint["document"]

                def transact(client, operation, **fields):
                    return client.transact(probe.envelope(instance, document, operation, **fields))

                # 1. A call on a fresh connection, before any attach: notAttached.
                client = probe.PipeClient(pipe_name)
                self.assertIsNone(client.connect(), "pipe open failed")
                result = client.transact(probe.envelope(instance, document, "call",
                                                        tool="get_pattern_context", arguments={}))
                self.assertEqual(result["error"]["code"], "notAttached")
                self.assertEqual(result["error"]["layer"], "attachment")

                # 2. Attach naming a different app instance: instanceGone, nothing bound.
                result = client.transact(probe.envelope("instance-from-another-lifetime", document, "attach"))
                self.assertEqual(result["error"]["code"], "instanceGone")

                # 3. Correct attach: ok.
                result = transact(client, "attach")
                self.assertTrue(result.get("ok"), result)

                # 4. Direct model read attempt (never queued): owningThreadRequired.
                result = transact(client, "call", tool="get_pattern_context", arguments={}, direct=True)
                self.assertEqual(result["error"]["code"], "owningThreadRequired")
                self.assertEqual(result["error"]["layer"], "dispatch")

                # 5. Attach naming an explicit but absent document: documentGone.
                #    (A *call* naming a different document than the attached one
                #    is refused at the attachment layer as notAttached, so the
                #    dispatch-time liveness recheck is reached via attach.)
                result = client.transact(probe.envelope(instance, "document-999999", "attach"))
                self.assertEqual(result["error"]["code"], "documentGone")

                # 6. The failures above changed nothing: the same attachment still reads.
                result = transact(client, "call", tool="get_pattern_context", arguments={})
                self.assertTrue(result.get("ok"), result)
                self.assertIn("context", result)

                # 7. The request is already in the broker queue when the test
                #    hook closes its explicitly addressed document. Dispatch
                #    must recheck lifetime and refuse the now-stale identity.
                result = transact(client, "call", tool="get_pattern_context", arguments={},
                                  test_close_before_dispatch=True)
                self.assertEqual(result["error"]["code"], "documentGone")
                self.assertEqual(result["error"]["layer"], "attachment")
                client.close()

                # 8. App exit: the connection dies and maps to instanceGone.
                stop.touch()
                deadline = time.monotonic() + 15
                while app.poll() is None and time.monotonic() < deadline:
                    time.sleep(0.1)
                self.assertEqual(app.poll(), 0, "app did not exit after stop file")
                client = probe.PipeClient(pipe_name)
                error = client.connect()
                if error is None:  # pipe lingered; one request must fail as lost
                    with self.assertRaises(probe.ConnectionLost):
                        client.transact(probe.envelope(instance, document, "attach"))
                    client.close()
                    error = probe.failure("instanceGone", "connection lost after app exit")
                self.assertEqual(error["error"]["code"], "instanceGone")
                self.assert_audio_path_clean(trace)
            finally:
                self.stop_app(app, stop)


if __name__ == "__main__":
    unittest.main()
