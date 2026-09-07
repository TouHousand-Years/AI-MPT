"""Local stdio MCP translator. Application state stays behind the named pipe."""
import argparse
import json
import struct
import sys

PROTOCOL = "2025-11-25"
MAX_MESSAGE = 4 * 1024 * 1024
ENDING = "Finish retained work with handoff_for_review or abort_session; use release_occupancy only for read-only work."


def object_schema(properties, required=()):
    return {"type": "object", "properties": properties, "required": list(required),
            "additionalProperties": False}


INTEGER = {"type": "integer", "minimum": 0}
SESSION = {"type": "string", "minLength": 1, "description": "Opaque token from an occupied read; never infer or reuse a lost token."}
CELL = object_schema({name: {"type": "integer", "minimum": 0, "maximum": 255}
                      for name in ("note", "instrument", "volume_command", "volume", "effect_command", "effect_parameter")},
                     ("note", "instrument", "volume_command", "volume", "effect_command", "effect_parameter"))
RANGE = object_schema({"first_row": INTEGER, "row_count": {"type": "integer", "minimum": 1},
                       "first_channel": INTEGER, "channel_count": {"type": "integer", "minimum": 1}},
                      ("first_row", "row_count", "first_channel", "channel_count"))
TOOLS = [
    {"name": "get_pattern_context", "description": "Read sparse semantic Score Context of the bound Pattern. Defaults to the candidate and the whole Pattern. Reads can exceed the write envelope. occupy=true retains occupancy. " + ENDING,
     "inputSchema": object_schema({"session": SESSION, "occupy": {"type": "boolean"},
                                    "baseline": {"type": "boolean"}, "range": RANGE})},
    {"name": "replace_pattern_segment", "description": "Replace one contiguous segment of one channel in the private candidate. Indices are zero-based; omitted rows become complete empty cells. Preserve all unsupported raw fields exactly. Expansion can wait for human approval. " + ENDING,
     "inputSchema": object_schema({"session": SESSION, "channel": INTEGER, "first_row": INTEGER,
                                    "row_count": {"type": "integer", "minimum": 1},
                                    "cells": {"type": "array", "items": object_schema({"row": INTEGER, "cell": CELL}, ("row", "cell"))}},
                                   ("session", "channel", "first_row", "row_count", "cells"))},
    {"name": "handoff_for_review", "description": "Freeze the entire final proposal and release occupancy atomically. The human reviews and applies it. " + ENDING,
     "inputSchema": object_schema({"session": SESSION}, ("session",))},
    {"name": "abort_session", "description": "Discard the whole candidate and release occupancy atomically. " + ENDING,
     "inputSchema": object_schema({"session": SESSION}, ("session",))},
    {"name": "release_occupancy", "description": "End retained read-only occupancy; fails if candidate edits exist. " + ENDING,
     "inputSchema": object_schema({"session": SESSION}, ("session",))},
]


def decode_json(value):
    def invalid_constant(value):
        raise ValueError("Non-finite JSON number")
    result = json.loads(value, parse_constant=invalid_constant)
    # Reject escaped lone surrogates before using an ID or forwarding a payload.
    encode_json(result)
    return result


def encode_json(value):
    return json.dumps(value, ensure_ascii=False, allow_nan=False, separators=(",", ":")).encode("utf-8")


def failure(code, reason, layer="transport"):
    return {"ok": False, "error": {"layer": layer, "code": code, "reason": reason}}


def tool_result(value):
    return {"isError": not value.get("ok", False), "structuredContent": value,
            "content": [{"type": "text", "text": encode_json(value).decode("utf-8") + "\n" + ENDING}]}


class RpcError(Exception):
    def __init__(self, code, message):
        super().__init__(message)
        self.code = code


class Sidecar:
    def __init__(self, pipe=None, instance=None, document=None):
        self.pipe_name, self.instance, self.document = pipe, instance, document
        self.pipe = None
        self.attachment_error = None

    def close(self):
        if self.pipe is not None:
            self.pipe.close()
            self.pipe = None

    def envelope(self, operation, **fields):
        return {"version": 1, "operation": operation, "instance": self.instance,
                "document": self.document, **fields}

    def transact(self, envelope):
        payload = encode_json(envelope)
        if len(payload) > MAX_MESSAGE:
            return failure("schemaFailure", "Application envelope exceeds the frame limit")
        frame = memoryview(struct.pack("<I", len(payload)) + payload)
        while frame:
            count = self.pipe.write(frame)
            if not count:
                raise EOFError()
            frame = frame[count:]

        def read_exact(size):
            data = bytearray()
            while len(data) < size:
                fragment = self.pipe.read(size - len(data))
                if not fragment:
                    raise EOFError()
                data.extend(fragment)
            return bytes(data)

        size, = struct.unpack("<I", read_exact(4))
        if not 0 < size <= MAX_MESSAGE:
            raise ValueError("Invalid application frame length")
        result = decode_json(read_exact(size))
        if not isinstance(result, dict) or type(result.get("ok")) is not bool:
            raise ValueError("Application result requires a boolean ok field")
        if not result["ok"]:
            error = result.get("error")
            if not isinstance(error, dict) or any(not isinstance(error.get(key), str) or not error[key] for key in ("layer", "code")):
                raise ValueError("Application failure requires an error layer and code")
        return result

    def call(self, name, arguments):
        if not all((self.pipe_name, self.instance, self.document)):
            return failure("notAttached", "Launch with an explicit --pipe, --instance and --document", "attachment")
        if self.attachment_error:
            return self.attachment_error
        try:
            if self.pipe is None:
                if not self.pipe_name.startswith("\\\\.\\pipe\\") or "/" in self.pipe_name or "\\" in self.pipe_name[9:]:
                    return failure("schemaFailure", "Only a local Windows named pipe is supported")
                self.pipe = open(self.pipe_name, "r+b", buffering=0)
                attached = self.transact(self.envelope("attach"))
                if not attached["ok"]:
                    self.attachment_error = attached
                    self.close()
                    return attached
            return self.transact(self.envelope("call", tool=name, arguments=arguments))
        except (OSError, EOFError):
            self.attachment_error = failure("instanceGone", "Application connection lost; do not replay mutations. Relaunch and explicitly attach again.")
        except (ValueError, UnicodeError, RecursionError):
            self.attachment_error = failure("schemaFailure", "Invalid application response; connection closed")
        self.close()
        return self.attachment_error

    def handle(self, request):
        method = request["method"]
        if method == "initialize":
            return {"protocolVersion": PROTOCOL, "capabilities": {"tools": {}},
                    "serverInfo": {"name": "openmpt-pattern", "version": "0.1.0"}}
        if method == "ping":
            return {}
        if method == "tools/list":
            return {"tools": TOOLS}
        if method == "tools/call":
            params = request.get("params", {})
            if not isinstance(params, dict) or not isinstance(params.get("name"), str) or params["name"] not in {tool["name"] for tool in TOOLS}:
                raise RpcError(-32602, "Unknown tool")
            arguments = params.get("arguments", {})
            if not isinstance(arguments, dict):
                raise RpcError(-32602, "Arguments must be an object")
            return tool_result(self.call(params["name"], arguments))
        raise LookupError("Unknown method")


def serve(sidecar):
    source, target = sys.stdin.buffer, sys.stdout.buffer
    while True:
        line = source.readline(MAX_MESSAGE + 1)
        if not line:
            return
        response_id = None
        try:
            if len(line) > MAX_MESSAGE:
                # Stop on an oversized record rather than interpreting its tail as a request.
                return
            request = decode_json(line)
            if isinstance(request, dict) and type(request.get("id")) in (str, int, type(None)):
                response_id = request.get("id")
            if not isinstance(request, dict) or request.get("jsonrpc") != "2.0" or not isinstance(request.get("method"), str) or type(request.get("id")) not in (str, int, type(None)):
                raise RpcError(-32600, "Expected a JSON-RPC request object")
            if "id" not in request:
                continue
            result = sidecar.handle(request)
            response = {"jsonrpc": "2.0", "id": response_id, "result": result}
        except (ValueError, UnicodeError, RecursionError):
            response = {"jsonrpc": "2.0", "id": response_id, "error": {"code": -32700, "message": "Invalid JSON-RPC message"}}
        except RpcError as error:
            response = {"jsonrpc": "2.0", "id": response_id, "error": {"code": error.code, "message": str(error)}}
        except LookupError:
            response = {"jsonrpc": "2.0", "id": response_id, "error": {"code": -32601, "message": "Method not found"}}
        target.write(encode_json(response) + b"\n")
        target.flush()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pipe", help="Exact current-user local pipe printed by the app")
    parser.add_argument("--instance", help="Exact app lifetime ID printed by the app")
    parser.add_argument("--document", help="Exact document lifetime ID printed by the app")
    options = parser.parse_args()
    sidecar = Sidecar(options.pipe, options.instance, options.document)
    try:
        serve(sidecar)
    finally:
        sidecar.close()
