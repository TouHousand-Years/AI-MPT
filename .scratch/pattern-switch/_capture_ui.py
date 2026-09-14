import ctypes
from ctypes import wintypes
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import sys
import tempfile
import time
from PIL import ImageGrab

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "sidecar"))
from test_native_integration import NativeIntegrationTests
import probe

test = NativeIntegrationTests()
user = ctypes.WinDLL("user32", use_last_error=True)
user.GetAncestor.argtypes = [wintypes.HWND, wintypes.UINT]
user.GetAncestor.restype = wintypes.HWND
user.ShowWindow.argtypes = [wintypes.HWND, ctypes.c_int]
user.SetWindowPos.argtypes = [wintypes.HWND, wintypes.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, wintypes.UINT]
with tempfile.TemporaryDirectory(prefix="openmpt-ui-check-") as directory:
    report, stop = Path(directory) / "endpoint.json", Path(directory) / "stop"
    app, _, endpoint = test.start_app(report, stop, 0)
    client = probe.PipeClient(endpoint["pipe"])
    preference = None
    pool = ThreadPoolExecutor(max_workers=1)
    try:
        preference = test.checked_control(app, "Always allow Pattern switching")
        if preference:
            test.click_control(app, "Always allow Pattern switching")
        tab = test.control(app, "SysTabControl32", class_name=True)
        root = user.GetAncestor(tab, 2)
        user.ShowWindow(root, 9)
        user.SetWindowPos(root, None, 40, 40, 1280, 900, 0x0004)
        test.activate_page(app, 49001)
        client.connect()
        client.transact(probe.envelope(endpoint["instance"], endpoint["document"], "attach"))
        future = pool.submit(client.transact, probe.envelope(endpoint["instance"], endpoint["document"], "call", tool="switch_pattern", arguments={"pattern": 1}))
        time.sleep(0.6)
        for width, height in ((1280, 900), (900, 700)):
            user.SetWindowPos(root, None, 40, 40, width, height, 0x0004)
            time.sleep(0.3)
            output = ROOT / ".scratch/pattern-switch" / f"approval-{width}.png"
            ImageGrab.grab(window=root).save(output)
            print(output)
        test.click_control(app, "Reject Pattern switch")
        print(json.dumps(future.result(timeout=5)))
    finally:
        if preference is not None and test.checked_control(app, "Always allow Pattern switching") != preference:
            test.click_control(app, "Always allow Pattern switching")
        test.stop_app(app, stop)
        pool.shutdown(wait=True)
        client.close()
