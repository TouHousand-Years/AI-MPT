"""Probe client tests against the scripted endpoint double; no music model."""
import json
import subprocess
import sys
import unittest

from test_sidecar import EndpointDouble

CONTEXT = {"ok": True, "context": {"pattern": 0, "rows": 128, "cells": []}}
ATTACH_OK = {"ok": True}


def run_probe(*args, stdin_text=""):
    return subprocess.run([sys.executable, "probe.py", *args], input=stdin_text,
                          text=True, encoding="utf-8", capture_output=True, timeout=10, cwd=str(__import__("pathlib").Path(__file__).parent))


class ProbeAttachTests(unittest.TestCase):
    def test_attach_reports_ok(self):
        with EndpointDouble([ATTACH_OK]) as endpoint:
            process = run_probe("attach", "--pipe", endpoint.path, "--instance", "i", "--document", "d")
        self.assertEqual(process.returncode, 0, process.stderr)
        self.assertIn("expected ok: PASS", process.stdout)

    def test_attach_reports_typed_failure(self):
        error = {"ok": False, "error": {"layer": "attachment", "code": "documentGone", "reason": "x"}}
        with EndpointDouble([error]) as endpoint:
            process = run_probe("attach", "--pipe", endpoint.path, "--instance", "i", "--document", "old")
        self.assertEqual(process.returncode, 1)
        self.assertIn("expected ok: FAIL (got documentGone)", process.stdout)

    def test_context_prints_score_context(self):
        with EndpointDouble([ATTACH_OK, CONTEXT]) as endpoint:
            process = run_probe("context", "--pipe", endpoint.path, "--instance", "i", "--document", "d")
        self.assertEqual(process.returncode, 0, process.stderr)
        self.assertIn("owning thread", process.stdout)
        printed, _ = json.JSONDecoder().raw_decode(process.stdout.rsplit("result:  ", 1)[1].lstrip())
        self.assertEqual(printed["context"]["rows"], 128)

    def test_missing_identities_are_refused(self):
        process = run_probe("context", "--pipe", r"\\.\pipe\openmpt-ai-test-x")
        self.assertNotEqual(process.returncode, 0)
        self.assertIn("Missing explicit identities", process.stderr)


class ProbeGuidedCaseTests(unittest.TestCase):
    def run_case(self, case, double_results, *extra, stdin_text=""):
        with EndpointDouble(double_results) as endpoint:
            process = run_probe("case", case, *extra, "--pipe", endpoint.path,
                                "--instance", "i", "--document", "d", stdin_text=stdin_text)
        return process

    def test_not_attached(self):
        error = {"ok": False, "error": {"layer": "attachment", "code": "notAttached", "reason": "x"}}
        process = self.run_case("not-attached", [error])
        self.assertEqual(process.returncode, 0, process.stderr)
        self.assertIn("expected notAttached: PASS", process.stdout)

    def test_stale_document(self):
        error = {"ok": False, "error": {"layer": "attachment", "code": "documentGone", "reason": "x"}}
        process = self.run_case("stale-document", [error], "--stale-document", "document-3")
        self.assertEqual(process.returncode, 0, process.stderr)
        self.assertIn("expected documentGone: PASS", process.stdout)
        self.assertIn("document-3", process.stdout)

    def test_wrong_instance(self):
        error = {"ok": False, "error": {"layer": "attachment", "code": "instanceGone", "reason": "x"}}
        process = self.run_case("wrong-instance", [error], "--wrong-instance", "other-app")
        self.assertEqual(process.returncode, 0, process.stderr)
        self.assertIn("expected instanceGone: PASS", process.stdout)

    def test_owning_thread(self):
        error = {"ok": False, "error": {"layer": "dispatch", "code": "owningThreadRequired", "reason": "x"}}
        process = self.run_case("owning-thread", [ATTACH_OK, error])
        self.assertEqual(process.returncode, 0, process.stderr)
        self.assertIn("expected owningThreadRequired: PASS", process.stdout)

    def test_document_gone_with_drift_check(self):
        gone = {"ok": False, "error": {"layer": "attachment", "code": "documentGone", "reason": "x"}}
        process = self.run_case("document-gone", [ATTACH_OK, gone, gone], "--drift-check",
                                stdin_text="\n\n")
        self.assertEqual(process.returncode, 0, process.stderr)
        self.assertEqual(process.stdout.count("expected documentGone: PASS"), 2)

    def test_instance_gone_maps_lost_connection(self):
        with EndpointDouble([ATTACH_OK, None]) as endpoint:
            process = run_probe("case", "instance-gone", "--pipe", endpoint.path,
                                "--instance", "i", "--document", "d", stdin_text="\n")
        self.assertEqual(process.returncode, 0, process.stderr)
        self.assertIn("expected instanceGone: PASS", process.stdout)

    def test_case_with_wrong_expectation_fails(self):
        wrong = {"ok": False, "error": {"layer": "dispatch", "code": "owningThreadRequired", "reason": "x"}}
        process = self.run_case("not-attached", [wrong])
        self.assertEqual(process.returncode, 1)
        self.assertIn("expected notAttached: FAIL (got owningThreadRequired)", process.stdout)

    def test_unknown_case_is_refused(self):
        process = run_probe("case", "nonsense")
        self.assertNotEqual(process.returncode, 0)
        self.assertIn("Unknown case", process.stderr)


if __name__ == "__main__":
    unittest.main()
