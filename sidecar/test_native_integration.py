"""Opt-in real process/pipe tests; no human Apply, playback or Save actions."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parent.parent
EXE = ROOT / "openmpt-original_ref/bin/debug/vs2022-win10-static/amd64/OpenMPT.exe"


@unittest.skipUnless(os.environ.get("OPENMPT_RUN_NATIVE_INTEGRATION") == "1", "Opt-in native executable integration")
class NativeIntegrationTests(unittest.TestCase):
    def test_both_directions_through_real_sidecar_and_app(self):
        for pattern, channels in ((0, [1, 2]), (1, [0])):
            with self.subTest(pattern=pattern), tempfile.TemporaryDirectory(prefix="openmpt-ai-") as directory:
                report = Path(directory) / "endpoint.json"
                stop = Path(directory) / "stop"
                env = dict(os.environ, OPENMPT_AI_TEST_FIXTURE=str(ROOT / "test-fixtures/ai-collab-fixture.mptm"),
                           OPENMPT_AI_ENDPOINT_REPORT=str(report), OPENMPT_AI_STOP_FILE=str(stop),
                           OPENMPT_AI_PATTERN=str(pattern))
                startup = subprocess.STARTUPINFO()
                startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
                startup.wShowWindow = 0
                app = subprocess.Popen([str(EXE), "/noSysCheck", "/noTests", "/noPlugins", "/noDls"],
                                       env=env, startupinfo=startup)
                sidecar = None
                try:
                    deadline = time.monotonic() + 30
                    while not report.exists() and time.monotonic() < deadline and app.poll() is None:
                        time.sleep(0.1)
                    trace = Path(str(report) + ".trace")
                    self.assertTrue(report.exists(), f"No app endpoint, exit={app.poll()}, trace={trace.read_text() if trace.exists() else 'none'}")
                    endpoint = json.loads(report.read_text())
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
                finally:
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


if __name__ == "__main__":
    unittest.main()
