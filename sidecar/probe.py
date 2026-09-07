"""Owner-facing command-line probe for the app-side IPC endpoint (issue 30).

Each command attaches to one explicitly addressed application and document and
prints the app's typed result. The guided cases walk the issue-30 failure
taxonomy against the real transport: notAttached, documentGone,
owningThreadRequired, instanceGone, plus a real Score Context read. Closing a
document or exiting the app is a human step; the probe pauses and says when.

Values for --pipe, --instance and --document are printed by the app's
"AI / MCP" panel. There is no discovery and no foreground-document fallback.
"""

import argparse
import json
import struct
import sys

from openmpt_mcp import MAX_MESSAGE, decode_json, encode_json, failure

ENDING = "Attachment is explicit and never guessed; failures never retarget a request."


class ConnectionLost(Exception):
    """The pipe closed or failed before a complete result arrived."""


class PipeClient:
    """Raw diagnostic client: full control over attach ordering and framing."""

    def __init__(self, pipe_name):
        self.pipe_name = pipe_name
        self.pipe = None

    def connect(self):
        if not self.pipe_name.startswith("\\\\.\\pipe\\") or "/" in self.pipe_name or "\\" in self.pipe_name[9:]:
            return failure("schemaFailure", "Only a local current-user Windows named pipe is supported")
        try:
            self.pipe = open(self.pipe_name, "r+b", buffering=0)
        except OSError as error:
            return failure("instanceGone", f"Pipe could not be opened: {error.strerror or error}")
        return None

    def close(self):
        if self.pipe is not None:
            self.pipe.close()
            self.pipe = None

    def transact(self, envelope):
        """Send one envelope and read one result. Raises ConnectionLost."""
        payload = encode_json(envelope)
        if len(payload) > MAX_MESSAGE:
            return failure("schemaFailure", "Probe envelope exceeds the frame limit")
        try:
            frame = memoryview(struct.pack("<I", len(payload)) + payload)
            while frame:
                count = self.pipe.write(frame)
                if not count:
                    raise ConnectionLost()
                frame = frame[count:]

            def read_exact(size):
                data = bytearray()
                while len(data) < size:
                    fragment = self.pipe.read(size - len(data))
                    if not fragment:
                        raise ConnectionLost()
                    data.extend(fragment)
                return bytes(data)

            size, = struct.unpack("<I", read_exact(4))
            if not 0 < size <= MAX_MESSAGE:
                raise ValueError("Invalid application frame length")
            result = decode_json(read_exact(size))
        except ConnectionLost:
            raise
        except (OSError, EOFError) as error:
            raise ConnectionLost() from error
        except (ValueError, UnicodeError, RecursionError) as error:
            raise ConnectionLost() from error
        if not isinstance(result, dict) or type(result.get("ok")) is not bool:
            raise ValueError("Application result requires a boolean ok field")
        return result


def envelope(instance, document, operation, **fields):
    return {"version": 1, "operation": operation, "instance": instance, "document": document, **fields}


def code_of(result):
    return result.get("error", {}).get("code") if not result.get("ok") else "ok"


def show(title, request, result):
    print(f"\n== {title} ==")
    print(f"request: {request}")
    print(f"result:  {json.dumps(result, ensure_ascii=False, indent=2)}")


def check(expected, result):
    actual = code_of(result)
    verdict = "PASS" if actual == expected else "FAIL"
    print(f"expected {expected}: {verdict} (got {actual})")
    return actual == expected


def require_options(options, *names):
    missing = [name for name in names if not getattr(options, name)]
    if missing:
        raise SystemExit(f"Missing explicit identities: {', '.join('--' + name for name in missing)}.\n"
                         "Copy the exact values from the app's AI / MCP panel.")


def attach(client, instance, document):
    """Attach and return (result, ok). A rejected attach leaves nothing bound."""
    result = client.transact(envelope(instance, document, "attach"))
    return result, bool(result.get("ok"))


def call(client, instance, document, tool="get_pattern_context", **extra):
    return client.transact(envelope(instance, document, "call", tool=tool, arguments={}, **extra))


def context_command(options):
    require_options(options, "pipe", "instance", "document")
    client = PipeClient(options.pipe)
    error = client.connect()
    if error:
        show("connect", {}, error)
        return 1
    try:
        attached, ok = attach(client, options.instance, options.document)
        show("attach", envelope(options.instance, options.document, "attach"), attached)
        if not ok:
            return 0 if check("ok", attached) else 1
        result = call(client, options.instance, options.document)
        show("get_pattern_context", envelope(options.instance, options.document, "call",
                                             tool="get_pattern_context", arguments={}), result)
        if result.get("ok"):
            print("\nThe Score Context above was captured on the document owning thread.")
            return 0
        return check("ok", result)
    except ConnectionLost:
        result = failure("instanceGone", "Application connection lost; relaunch and explicitly attach again.")
        show("connection lost", {}, result)
        return check("ok", result)
    finally:
        client.close()


def case_command(options):
    cases = ("not-attached", "stale-document", "owning-thread", "wrong-instance", "document-gone", "instance-gone")
    if options.case not in cases:
        raise SystemExit(f"Unknown case {options.case!r}; expected one of {', '.join(cases)}")
    require_options(options, "pipe")
    if options.case in ("document-gone", "instance-gone"):
        require_options(options, "instance", "document")
    result, passed = run_case(options.case, options)
    if not passed and result is not None and code_of(result) == "instanceGone" and options.case != "instance-gone":
        print("\nThe application connection was lost instead of answering; is the app still running?")
    return 0 if passed else 1


def run_case(case, options):
    """Run one guided case; returns (final result, expectation met)."""
    if case == "not-attached":
        client = PipeClient(options.pipe)
        error = client.connect()
        if error:
            show("not-attached", {}, error)
            return error, check("notAttached", error)
        try:
            request = envelope(options.instance or "unset", options.document or "unset", "call",
                               tool="get_pattern_context", arguments={})
            result = client.transact(request)
            show("call before any attach", request, result)
            return result, check("notAttached", result)
        except ConnectionLost:
            result = failure("instanceGone", "Application connection lost before answering.")
            show("connection lost", {}, result)
            return result, False
        finally:
            client.close()

    if case == "stale-document":
        document = options.stale_document or "document-999999"
        client = PipeClient(options.pipe)
        error = client.connect()
        if error:
            show("stale-document", {}, error)
            return error, check("documentGone", error)
        try:
            request = envelope(options.instance or "unset", document, "attach")
            result = client.transact(request)
            show(f"attach to explicit but absent document {document}", request, result)
            return result, check("documentGone", result)
        except ConnectionLost:
            result = failure("instanceGone", "Application connection lost before answering.")
            show("connection lost", {}, result)
            return result, False
        finally:
            client.close()

    if case == "wrong-instance":
        instance = options.wrong_instance or "instance-from-another-app-lifetime"
        client = PipeClient(options.pipe)
        error = client.connect()
        if error:
            show("wrong-instance", {}, error)
            return error, check("instanceGone", error)
        try:
            request = envelope(instance, options.document or "unset", "attach")
            result = client.transact(request)
            show(f"attach naming a different app instance {instance}", request, result)
            return result, check("instanceGone", result)
        except ConnectionLost:
            result = failure("instanceGone", "Application connection lost before answering.")
            show("connection lost", {}, result)
            return result, False
        finally:
            client.close()

    # The remaining cases need a live attachment first.
    require_options(options, "instance", "document")
    client = PipeClient(options.pipe)
    error = client.connect()
    if error:
        show(case, {}, error)
        return error, False
    try:
        attached, ok = attach(client, options.instance, options.document)
        show("attach", envelope(options.instance, options.document, "attach"), attached)
        if not ok:
            return attached, False

        if case == "owning-thread":
            request = envelope(options.instance, options.document, "call",
                               tool="get_pattern_context", arguments={}, direct=True)
            result = client.transact(request)
            show("direct model read attempt (never queued)", request, result)
            print("The broker tried to satisfy this off the owning thread and the owning-thread")
            print("guard rejected it; no document state was read or changed.")
            return result, check("owningThreadRequired", result)

        if case == "document-gone":
            print(f"\nGUIDED STEP: close the document {options.document} in the app now")
            input("then press Enter here to send a request addressed to it... ")
            request = envelope(options.instance, options.document, "call",
                               tool="get_pattern_context", arguments={})
            try:
                result = client.transact(request)
                show("call to the just-closed document", request, result)
                passed = check("documentGone", result)
            except ConnectionLost:
                result = failure("instanceGone", "Application connection lost before answering.")
                show("connection lost", {}, result)
                return result, False
            if options.drift_check:
                print("\nGUIDED STEP: open another document in the app (its ID will differ)")
                input("then press Enter to re-send the same stale request... ")
                try:
                    drifted = client.transact(request)
                    show("stale request after another document exists", request, drifted)
                    print("The old ID never drifts to the new document; the request stayed dead.")
                    passed = passed and check("documentGone", drifted)
                except ConnectionLost:
                    drifted = failure("instanceGone", "Application connection lost before answering.")
                    show("connection lost", {}, drifted)
                    return drifted, False
            return result, passed

        if case == "instance-gone":
            print("\nGUIDED STEP: exit the application now (File > Exit or close its window)")
            input("then press Enter here to send one more request... ")
            request = envelope(options.instance, options.document, "call",
                               tool="get_pattern_context", arguments={})
            try:
                result = client.transact(request)
                show("call after the application exited", request, result)
                return result, check("instanceGone", result)
            except ConnectionLost:
                result = failure("instanceGone", "Application connection lost; the instance is gone. "
                                                 "Relaunch and explicitly attach again.")
                show("connection lost", {}, result)
                return result, check("instanceGone", result)

    finally:
        client.close()
    raise SystemExit(f"Unhandled case {case!r}")


def demo_command(options):
    require_options(options, "pipe", "instance", "document")
    print("Issue 30 guided demo against the running app.")
    print("First a real read; then the failure taxonomy, gentlest first.")
    print("The document-gone and instance-gone steps ask you to act in the app.\n")

    client = PipeClient(options.pipe)
    error = client.connect()
    if error:
        show("connect", {}, error)
        return 1
    try:
        attached, ok = attach(client, options.instance, options.document)
        show("attach", envelope(options.instance, options.document, "attach"), attached)
        if not ok:
            return 0 if check("ok", attached) else 1
        result = call(client, options.instance, options.document)
        title = "get_pattern_context (real Score Context from the owning thread)"
        show(title, envelope(options.instance, options.document, "call",
                             tool="get_pattern_context", arguments={}), result)
        if not result.get("ok"):
            return 0 if check("ok", result) else 1
        print(json.dumps(result.get("context", result), ensure_ascii=False, indent=2))
        print("\nThe context above is the real document state, captured on the owning thread.")
    except ConnectionLost:
        result = failure("instanceGone", "Application connection lost; relaunch and explicitly attach again.")
        show("connection lost", {}, result)
        return 1
    finally:
        client.close()

    summary = [("get_pattern_context", result.get("ok"))]
    for case, expected in (("not-attached", "notAttached"), ("wrong-instance", "instanceGone"),
                           ("stale-document", "documentGone"), ("owning-thread", "owningThreadRequired")):
        print(f"\n--- case {case}: expect {expected} ---")
        _, passed = run_case(case, options)
        summary.append((case, passed))

    print("\n--- case document-gone: expect documentGone ---")
    _, passed = run_case("document-gone", options)
    summary.append(("document-gone", passed))
    print("\n--- case instance-gone: expect instanceGone ---")
    _, passed = run_case("instance-gone", options)
    summary.append(("instance-gone", passed))

    print("\n== demo summary ==")
    failures = 0
    for name, ok in summary:
        print(f"  {name}: {'PASS' if ok else 'FAIL'}")
        failures += 0 if ok else 1
    print(f"\n{len(summary) - failures}/{len(summary)} expectations met.")
    return 0 if failures == 0 else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    # Identity options live on each subcommand so they may follow it, e.g.
    # `probe.py context --pipe ... --instance ... --document ...`.
    identities = argparse.ArgumentParser(add_help=False)
    identities.add_argument("--pipe", help="exact current-user local pipe printed by the app")
    identities.add_argument("--instance", help="exact app lifetime ID printed by the app")
    identities.add_argument("--document", help="exact document lifetime ID printed by the app")
    sub = parser.add_subparsers(dest="command")
    sub.add_parser("attach", parents=[identities], help="attach and report the typed result")
    sub.add_parser("context", parents=[identities], help="attach, read the real Score Context, print it")
    case = sub.add_parser("case", parents=[identities], help="run one guided failure case")
    case.add_argument("case", help="not-attached | stale-document | owning-thread | wrong-instance | document-gone | instance-gone")
    case.add_argument("--stale-document", help="document ID to name in the stale-document case")
    case.add_argument("--wrong-instance", help="instance ID to name in the wrong-instance case")
    case.add_argument("--drift-check", action="store_true",
                      help="document-gone: re-send after another document opens to show no drift")
    demo = sub.add_parser("demo", parents=[identities], help="guided walk: real read, then the whole failure taxonomy")
    demo.add_argument("--drift-check", action="store_true",
                      help="document-gone: re-send after another document opens to show no drift")
    options = parser.parse_args()
    if not options.command:
        parser.print_help()
        return 2
    if options.command == "attach":
        require_options(options, "pipe", "instance", "document")
        client = PipeClient(options.pipe)
        error = client.connect()
        if error:
            show("connect", {}, error)
            return 1
        try:
            result, _ = attach(client, options.instance, options.document)
            show("attach", envelope(options.instance, options.document, "attach"), result)
            return 0 if check("ok", result) else 1
        finally:
            client.close()
    if options.command == "context":
        return context_command(options)
    if options.command == "case":
        return case_command(options)
    if options.command == "demo":
        return demo_command(options)
    return 2


if __name__ == "__main__":
    sys.exit(main())
