"""Process-boundary tests; the endpoint double never accesses a music model."""
import json
from pathlib import Path
import subprocess
import sys
import unittest
import ctypes
from ctypes import wintypes
import os
import struct
import tempfile
import threading
import uuid

from openmpt_mcp import Sidecar, automatic_target_file

ROOT = Path(__file__).resolve().parent


def call(name="get_pattern_context", arguments=None):
    return {"jsonrpc": "2.0", "id": 3, "method": "tools/call",
            "params": {"name": name, "arguments": arguments or {}}}


class EndpointDouble:
    """A real Windows pipe peer speaking the proposed app envelope, no facade mock."""
    def __init__(self, results):
        self.path = r"\\.\pipe\openmpt-ai-test-" + uuid.uuid4().hex
        self.results = results
        self.requests = []
        self.error = None
        self.ready = threading.Event()
        self.thread = threading.Thread(target=self.run, daemon=True)

    def run(self):
        kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel.CreateNamedPipeW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD,
                                           wintypes.DWORD, wintypes.DWORD, wintypes.DWORD,
                                           wintypes.DWORD, ctypes.c_void_p]
        kernel.CreateNamedPipeW.restype = wintypes.HANDLE
        for name in ("ConnectNamedPipe", "DisconnectNamedPipe", "CloseHandle", "FlushFileBuffers"):
            getattr(kernel, name).argtypes = [wintypes.HANDLE] + ([ctypes.c_void_p] if name == "ConnectNamedPipe" else [])
        for name in ("ReadFile", "WriteFile"):
            getattr(kernel, name).argtypes = [wintypes.HANDLE, ctypes.c_void_p, wintypes.DWORD,
                                             ctypes.POINTER(wintypes.DWORD), ctypes.c_void_p]
        handle = kernel.CreateNamedPipeW(self.path, 3, 8, 1, 65536, 65536, 0, None)
        try:
            if handle == ctypes.c_void_p(-1).value:
                raise ctypes.WinError(ctypes.get_last_error())
            self.ready.set()
            if not kernel.ConnectNamedPipe(handle, None) and ctypes.get_last_error() != 535:
                raise ctypes.WinError(ctypes.get_last_error())

            def read(size):
                data = bytearray()
                while len(data) < size:
                    buffer = ctypes.create_string_buffer(size - len(data))
                    count = wintypes.DWORD()
                    if not kernel.ReadFile(handle, buffer, len(buffer), ctypes.byref(count), None) or not count.value:
                        raise RuntimeError("Peer disconnected before expected envelope")
                    data.extend(buffer.raw[:count.value])
                return bytes(data)

            for result in self.results:
                size = struct.unpack("<I", read(4))[0]
                if size > 4 * 1024 * 1024:
                    raise RuntimeError("Oversized request")
                self.requests.append(json.loads(read(size)))
                if result is None:
                    break
                payload = json.dumps(result).encode("utf-8")
                frame = struct.pack("<I", len(payload)) + payload
                # Split the header and body deliberately: the client must read exactly.
                for fragment in (frame[:2], frame[2:9], frame[9:]):
                    count = wintypes.DWORD()
                    if not kernel.WriteFile(handle, fragment, len(fragment), ctypes.byref(count), None):
                        raise ctypes.WinError(ctypes.get_last_error())
            kernel.FlushFileBuffers(handle)
        except Exception as error:
            self.error = error
            self.ready.set()
        finally:
            kernel.DisconnectNamedPipe(handle)
            kernel.CloseHandle(handle)

    def __enter__(self):
        self.thread.start()
        if not self.ready.wait(5) or self.error:
            raise RuntimeError(f"Endpoint startup failed: {self.error}")
        return self

    def __exit__(self, *args):
        self.thread.join(5)
        if self.error:
            raise self.error
        if self.thread.is_alive():
            raise RuntimeError("Endpoint did not finish")


class SidecarTests(unittest.TestCase):
    def exchange(self, messages, *args):
        process = subprocess.run(
            [sys.executable, str(ROOT / "openmpt_mcp.py"), *args],
            input="".join(json.dumps(message) + "\n" for message in messages),
            text=True, encoding="utf-8", capture_output=True, timeout=10,
        )
        self.assertEqual(process.returncode, 0, process.stderr)
        return [json.loads(line) for line in process.stdout.splitlines()]

    def test_initialize_and_eight_pattern_tools(self):
        replies = self.exchange([
            {"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {
                "protocolVersion": "2025-11-25", "capabilities": {},
                "clientInfo": {"name": "test", "version": "1"}}},
            {"jsonrpc": "2.0", "method": "notifications/initialized"},
            {"jsonrpc": "2.0", "id": 2, "method": "tools/list"},
        ])
        self.assertEqual(len(replies), 2)
        self.assertEqual(replies[0]["result"]["protocolVersion"], "2025-11-25")
        self.assertEqual({t["name"] for t in replies[1]["result"]["tools"]}, {
            "get_pattern_context", "replace_pattern_segment", "handoff_for_review",
            "abort_session", "release_occupancy",
            "get_pattern_order", "reorder_pattern_order", "switch_pattern",
        })

    def test_switch_token_pins_document_until_failed_auto_apply_finishes_session(self):
        with tempfile.TemporaryDirectory() as directory:
            target_file = Path(directory) / "target.json"
            pending = {"ok": False, "status": "pending_review", "error": {
                "layer": "capability", "code": "emptyProposal", "reason": "No changes"}}
            first = {"version": 1, "pipe": r"\\.\pipe\first", "instance": "i1", "document": "d1", "generation": "g1"}
            target_file.write_text(json.dumps(first), encoding="utf-8")
            sidecar = Sidecar(target_file=target_file)
            self.assertIsNone(sidecar.refresh_target("switch_pattern"))
            sidecar.update_session_state("switch_pattern", {"ok": True, "status": "switched", "session": "switch-token"})
            second = dict(first, document="d2", generation="g2")
            target_file.write_text(json.dumps(second), encoding="utf-8")
            blocked = sidecar.refresh_target("get_pattern_order")
            self.assertIsNotNone(blocked, "Successful switch must retain the document connection")
            self.assertEqual(blocked["error"]["code"], "busy")
            self.assertEqual(sidecar.document, "d1")
            self.assertIsNone(sidecar.refresh_target("handoff_for_review"))
            sidecar.update_session_state("handoff_for_review", pending)
            self.assertIsNone(sidecar.refresh_target("get_pattern_order"))
            self.assertEqual(sidecar.document, "d2")

    def test_tools_require_explicit_attachment(self):
        result = self.exchange([call()])[0]["result"]
        self.assertTrue(result["isError"])
        self.assertEqual(result["structuredContent"]["error"]["code"], "notAttached")

    def test_target_file_requires_an_openmpt_publication(self):
        with tempfile.TemporaryDirectory() as directory:
            missing = Path(directory) / "target.json"
            result = self.exchange([call()], "--target-file", str(missing))[0]["result"]
        self.assertEqual(result["structuredContent"]["error"]["code"], "notAttached")
        self.assertIn("Connect active document", result["structuredContent"]["error"]["reason"])

    def test_auto_target_uses_current_user_local_app_data(self):
        old = os.environ.get("LOCALAPPDATA")
        try:
            os.environ["LOCALAPPDATA"] = r"C:\Users\test\AppData\Local"
            self.assertEqual(automatic_target_file(),
                             Path(r"C:\Users\test\AppData\Local") / "OpenMPT" / "AI" / "codex-target.json")
        finally:
            if old is None:
                os.environ.pop("LOCALAPPDATA", None)
            else:
                os.environ["LOCALAPPDATA"] = old

    def test_target_switch_waits_for_retained_work_to_end(self):
        with tempfile.TemporaryDirectory() as directory:
            target_file = Path(directory) / "target.json"
            first = {"version": 1, "pipe": r"\\.\pipe\first", "instance": "i1",
                     "document": "d1", "generation": "g1"}
            second = {"version": 1, "pipe": r"\\.\pipe\second", "instance": "i2",
                      "document": "d2", "generation": "g2"}
            target_file.write_text(json.dumps(first), encoding="utf-8")
            sidecar = Sidecar(target_file=target_file)
            self.assertIsNone(sidecar.refresh_target("get_pattern_context"))
            sidecar.retained_session = True
            target_file.write_text(json.dumps(second), encoding="utf-8")
            blocked = sidecar.refresh_target("replace_pattern_segment")
            self.assertEqual(blocked["error"]["code"], "busy")
            self.assertEqual(sidecar.document, "d1")
            self.assertIsNone(sidecar.refresh_target("abort_session"))
            self.assertEqual(sidecar.document, "d1")
            sidecar.retained_session = False
            self.assertIsNone(sidecar.refresh_target("get_pattern_context"))
            self.assertEqual(sidecar.document, "d2")

    def test_republishing_same_document_clears_latched_attachment_error(self):
        with tempfile.TemporaryDirectory() as directory:
            target_file = Path(directory) / "target.json"
            target = {"version": 1, "pipe": r"\\.\pipe\same", "instance": "i",
                      "document": "d", "generation": "g1"}
            target_file.write_text(json.dumps(target), encoding="utf-8")
            sidecar = Sidecar(target_file=target_file)
            self.assertIsNone(sidecar.refresh_target("get_pattern_context"))
            sidecar.attachment_error = {"ok": False, "error": {"code": "instanceGone"}}
            target["generation"] = "g2"
            target_file.write_text(json.dumps(target), encoding="utf-8")
            self.assertIsNone(sidecar.refresh_target("get_pattern_context"))
            self.assertIsNone(sidecar.attachment_error)

    @unittest.skipUnless(os.name == "nt", "Windows pipe transport")
    def test_published_target_attaches_without_cli_identities(self):
        with tempfile.TemporaryDirectory() as directory:
            target_file = Path(directory) / "target.json"
            with EndpointDouble([{"ok": True}, {"ok": True, "context": {}}]) as endpoint:
                target_file.write_text(json.dumps({"version": 1, "pipe": endpoint.path,
                                                   "instance": "published-instance",
                                                   "document": "published-document",
                                                   "generation": "publication-1"}), encoding="utf-8")
                reply = self.exchange([call()], "--target-file", str(target_file))[0]["result"]
            self.assertEqual(reply["structuredContent"], {"ok": True, "context": {}})
            self.assertEqual(endpoint.requests[0]["operation"], "attach")
            self.assertEqual(endpoint.requests[0]["document"], "published-document")

    @unittest.skipUnless(os.name == "nt", "Windows pipe transport")
    def test_typed_application_failures_preserved(self):
        for code in ("notAttached", "documentGone", "owningThreadRequired", "instanceGone"):
            with self.subTest(code=code):
                failure = {"ok": False, "error": {"layer": "capability", "code": code,
                           "row": 17, "field": "note", "value": 254, "reason": "scripted failure"}}
                with EndpointDouble([{"ok": True}, failure]) as endpoint:
                    reply = self.exchange([call()], "--pipe", endpoint.path,
                                          "--instance", "i", "--document", "d")[0]["result"]
                self.assertTrue(reply["isError"])
                self.assertEqual(reply["structuredContent"], failure)

    @unittest.skipUnless(os.name == "nt", "Windows pipe transport")
    def test_failed_attachment_never_calls_or_retargets(self):
        error = {"ok": False, "error": {"layer": "attachment", "code": "documentGone"}}
        with EndpointDouble([error]) as endpoint:
            replies = self.exchange([call(), call()], "--pipe", endpoint.path,
                                    "--instance", "i", "--document", "wrong-id")
        self.assertEqual(len(endpoint.requests), 1)
        self.assertEqual([reply["result"]["structuredContent"] for reply in replies], [error, error])

    @unittest.skipUnless(os.name == "nt", "Windows pipe transport")
    def test_disconnect_is_not_replayed(self):
        with EndpointDouble([{"ok": True}, None]) as endpoint:
            replies = self.exchange([call("replace_pattern_segment"), call()],
                                    "--pipe", endpoint.path, "--instance", "i", "--document", "d")
        self.assertEqual(len(endpoint.requests), 2)
        self.assertEqual([reply["result"]["structuredContent"]["error"]["code"] for reply in replies],
                         ["instanceGone", "instanceGone"])

    def test_remote_pipe_is_rejected(self):
        reply = self.exchange([call()], "--pipe", r"\\remote\pipe\service", "--instance", "i", "--document", "d")
        self.assertEqual(reply[0]["result"]["structuredContent"]["error"]["code"], "schemaFailure")

    def test_unknown_tool_and_malformed_request(self):
        replies = self.exchange([call("apply_proposal"), {"jsonrpc": "2.0", "id": 4},
                                 {"jsonrpc": "2.0", "id": 5, "method": "tools/call", "params": []}])
        self.assertEqual([reply["error"]["code"] for reply in replies], [-32602, -32600, -32602])

    def test_unpaired_surrogate_does_not_crash_process(self):
        replies = self.exchange([{"jsonrpc": "2.0", "id": "\ud800", "method": "ping"},
                                 {"jsonrpc": "2.0", "id": 5, "method": "ping"}])
        self.assertEqual(replies[0]["error"]["code"], -32700)
        self.assertEqual(replies[1]["result"], {})

    @unittest.skipUnless(os.name == "nt", "Windows pipe transport")
    def test_malformed_app_failures_close_connection(self):
        for malformed in ({"ok": False}, {"ok": False, "error": "bad"},
                          {"ok": False, "error": {"layer": "capability", "code": 123}}):
            with self.subTest(malformed=malformed):
                with EndpointDouble([{"ok": True}, malformed]) as endpoint:
                    replies = self.exchange([call(), call()], "--pipe", endpoint.path,
                                            "--instance", "i", "--document", "d")
                self.assertEqual(len(endpoint.requests), 2)
                self.assertEqual([reply["result"]["structuredContent"]["error"]["code"] for reply in replies],
                                 ["schemaFailure", "schemaFailure"])

    @unittest.skipUnless(os.name == "nt", "Windows pipe transport")
    def test_all_eight_calls_translate_without_rewriting_arguments(self):
        arguments = {"session": "opaque", "cells": [{"row": 17, "cell": {"note": 61}}]}
        names = ["get_pattern_context", "replace_pattern_segment", "handoff_for_review",
                 "abort_session", "release_occupancy", "get_pattern_order", "reorder_pattern_order", "switch_pattern"]
        replies = [{"ok": True}] + [{"ok": True, "echo": name} for name in names]
        with EndpointDouble(replies) as endpoint:
            results = self.exchange([call(name, arguments) for name in names],
                                    "--pipe", endpoint.path, "--instance", "instance-a", "--document", "document-b")
        self.assertEqual(endpoint.requests[0], {"version": 1, "operation": "attach",
                                               "instance": "instance-a", "document": "document-b"})
        for name, request, reply in zip(names, endpoint.requests[1:], results):
            self.assertEqual(request, {"version": 1, "operation": "call", "tool": name,
                                       "instance": "instance-a", "document": "document-b", "arguments": arguments})
            self.assertEqual(reply["result"]["structuredContent"], {"ok": True, "echo": name})


if __name__ == "__main__":
    unittest.main()
