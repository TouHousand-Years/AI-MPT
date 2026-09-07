"""Opt-in real process/pipe tests; no human Apply, playback or Save actions."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import probe

ROOT = Path(__file__).resolve().parent.parent
EXE = ROOT / "openmpt-original_ref/bin/debug/vs2022-win10-static/amd64/OpenMPT.exe"


@unittest.skipUnless(os.environ.get("OPENMPT_RUN_NATIVE_INTEGRATION") == "1", "Opt-in native executable integration")
class NativeIntegrationTests(unittest.TestCase):
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
        self.assertTrue(report.exists(), f"No app endpoint, exit={app.poll()}, trace={trace.read_text() if trace.exists() else 'none'}")
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
                    self.assertEqual(len(request("tools/list")["tools"]), 5)
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
