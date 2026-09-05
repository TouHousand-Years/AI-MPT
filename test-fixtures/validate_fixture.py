#!/usr/bin/env python3
"""Validate the AI collaboration MPTM fixture against ticket 28's acceptance
criteria, by parsing the module file independently of the generator.

Checks performed on the module structure:

  * MPTM identity (cwtv 0x891 / cmwt 0x888), song name and song message
  * Order list contains each target Pattern exactly once
  * Pattern 0 (state A): melody on ch 1, bass on ch 4, harmony (ch 2/3) empty
  * Pattern 1 (state B): harmony on ch 2/3, bass on ch 4, melody (ch 1) empty
  * 128-row patterns (8 bars), 4/4 signature in the header (4 rows/beat,
    16 rows/measure), no effect commands at all (fixed meter and tempo)
  * three instruments bound to three non-empty 16-bit samples (no missing
    sample references; every note references its instrument)
  * documented reset path: song message names both states and the reset

Checks performed on playback (if openmpt123 is found): renders the module
and asserts both halves contain audio.

Usage:  python validate_fixture.py [fixture.mptm]
Exit code 0 = all checks passed.
"""

import math
import struct
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate_fixture as gen  # noqa: E402

OPENMPT123_CANDIDATES = [
    REPO / "openmpt-original_ref" / "bin" / "debug" / "vs2022-win10-static" / "amd64" / "openmpt123.exe",
]

checks_passed = 0


def check(condition, description, detail=""):
    global checks_passed
    if condition:
        checks_passed += 1
        print(f"  ok  {description}")
    else:
        print(f" FAIL {description} {detail}")
        raise AssertionError(description)


# ---------------------------------------------------------------------------
# MPTM parsing
# ---------------------------------------------------------------------------

class PatternCell:
    __slots__ = ("note", "instr", "vol", "cmd", "param")

    def __init__(self):
        self.note = 0
        self.instr = 0
        self.vol = None
        self.cmd = 0
        self.param = 0


def parse_pattern(data, pos):
    packed_len, num_rows = struct.unpack_from("<HH", data, pos)
    pos += 8  # u16 packed size, u16 rows, u16 unused x2
    packed = data[pos:pos + packed_len]
    rows = [[PatternCell() for _ in range(64)] for _ in range(num_rows)]
    p, row, ch, mask = 0, 0, 0, 0
    last = {}
    while row < num_rows and p < len(packed):
        b = packed[p]; p += 1
        if b == 0:
            row += 1
            continue
        ch = (b & 0x7F) - 1
        if b & 0x80:
            mask = packed[p]; p += 1
        cell = rows[row][ch]
        prev = last.setdefault(ch, PatternCell())
        if mask & 0x10: cell.note = prev.note
        if mask & 0x20: cell.instr = prev.instr
        if mask & 0x40: cell.vol = prev.vol
        if mask & 0x80: cell.cmd, cell.param = prev.cmd, prev.param
        if mask & 0x01:
            v = packed[p]; p += 1
            cell.note = {0xFF: 255, 0xFE: 254}.get(v, v + 1)  # 1-based, 255=keyoff
        if mask & 0x02:
            cell.instr = packed[p]; p += 1
        if mask & 0x04:
            v = packed[p]; p += 1
            if v <= 64: cell.vol = v
        if mask & 0x08:
            cell.cmd = packed[p]; cell.param = packed[p + 1]; p += 2
        last[ch] = cell
    return num_rows, rows, pos + packed_len


def parse_module(path):
    data = path.read_bytes()
    m = {}
    h = data[:192]
    check(h[:4] == b"IMPM", "IT/MPTM magic 'IMPM'")
    m["cwtv"], m["cmwt"] = struct.unpack_from("<2H", h, 40)
    m["flags"], m["special"] = struct.unpack_from("<2H", h, 44)
    m["ordnum"], m["insnum"], m["smpnum"], m["patnum"] = struct.unpack_from("<4H", h, 32)
    m["globalvol"], m["mv"], m["speed"], m["tempo"] = h[48], h[49], h[50], h[51]
    m["rows_per_beat"], m["rows_per_measure"] = h[30], h[31]
    m["msglength"], m["msgoffset"] = struct.unpack_from("<HI", h, 54)
    m["songname"] = h[4:30].rstrip(b" \x00").decode("ascii")
    m["chnpan"] = list(h[64:128])

    pos = 192
    m["orders"] = list(data[pos:pos + m["ordnum"]]); pos += m["ordnum"]
    inspos = struct.unpack_from("<%dI" % m["insnum"], data, pos); pos += 4 * m["insnum"]
    smppos = struct.unpack_from("<%dI" % m["smpnum"], data, pos); pos += 4 * m["smpnum"]
    patpos = struct.unpack_from("<%dI" % m["patnum"], data, pos); pos += 4 * m["patnum"]

    # extensions between parapointers and instruments: history, PNAM, CNAM, message
    q = pos
    if m["special"] & 0x02:  # embedEditHistory: u16 count + 8-byte entries
        nflt, = struct.unpack_from("<H", data, q)
        q += 2 + nflt * 8

    def read_names_chunk(q, magic, entry_size):
        if data[q:q + 4] != magic:
            return None, q
        size, = struct.unpack_from("<I", data, q + 4)
        raw = data[q + 8:q + 8 + size]
        names = [raw[i:i + entry_size].split(b"\x00")[0].decode("ascii")
                 for i in range(0, size, entry_size)]
        return names, q + 8 + size

    m["pattern_names"], q = read_names_chunk(q, b"PNAM", 32)
    m["channel_names"], q = read_names_chunk(q, b"CNAM", 20)
    m["message"] = data[m["msgoffset"]:m["msgoffset"] + m["msglength"]].split(b"\x00")[0].decode("ascii")

    # instruments
    m["instruments"] = []
    for p in inspos:
        check(data[p:p + 4] == b"IMPI", "instrument magic 'IMPI'")
        ins = {"name": data[p + 32:p + 58].rstrip(b" \x00").decode("ascii"),
               "keyboard": data[p + 64:p + 64 + 240]}
        m["instruments"].append(ins)

    # samples (headers + data)
    m["samples"] = []
    for i, p in enumerate(smppos):
        check(data[p:p + 4] == b"IMPS", "sample magic 'IMPS'")
        smp = {"name": data[p + 20:p + 46].rstrip(b" \x00").decode("ascii")}
        flags = data[p + 18]
        smp["16bit"] = bool(flags & 0x02)
        smp["length"], = struct.unpack_from("<I", data, p + 48)
        smp["c5speed"], = struct.unpack_from("<I", data, p + 60)
        ptr, = struct.unpack_from("<I", data, p + 72)
        raw = data[ptr:ptr + smp["length"] * (2 if smp["16bit"] else 1)]
        smp["pcm"] = struct.unpack("<%dh" % smp["length"], raw) if smp["16bit"] else raw
        m["samples"].append(smp)

    # patterns
    m["patterns"] = []
    for p in patpos:
        num_rows, rows, _ = parse_pattern(data, p)
        m["patterns"].append(rows)
    m["num_rows"] = [len(rows) for rows in m["patterns"]]

    # trailing MPTM extension: u32 offset -> Ssb("mptm") -> STPM block before it
    mpt_start, = struct.unpack_from("<I", data, len(data) - 4)
    m["mpt_start"] = mpt_start
    m["ssb_magic_ok"] = data[mpt_start:mpt_start + 3] == b"228"
    stpm = data.rfind(b"STPM", 0, mpt_start)
    m["has_stpm"] = stpm > 0
    return m


def cells_of(rows, ch):
    """Non-empty (row, note, instr, vol, cmd, param) tuples of one channel."""
    out = []
    for row, cells in enumerate(rows):
        c = cells[ch]
        if c.note or c.instr or c.vol is not None or c.cmd:
            out.append((row, c.note, c.instr, c.vol, c.cmd, c.param))
    return out


# ---------------------------------------------------------------------------
# Structure checks
# ---------------------------------------------------------------------------

def check_structure(m):
    print("Module identity")
    check(m["cwtv"] == 0x891 and m["cmwt"] == 0x888, "cwtv 0x891 / cmwt 0x888 (MPTM)")
    check(m["songname"] == gen.SONG_NAME, "song name", m["songname"])
    check(m["ordnum"] == 2 and m["insnum"] == 3 and m["smpnum"] == 3 and m["patnum"] == 2,
          "2 orders, 3 instruments, 3 samples, 2 patterns")
    check(m["ssb_magic_ok"], "tail pointer leads to the Ssb '228' MPTM extension block")

    print("Fixed meter and tempo")
    check(m["rows_per_beat"] == 4, "header rows-per-beat = 4", str(m["rows_per_beat"]))
    check(m["rows_per_measure"] == 16, "header rows-per-measure = 16", str(m["rows_per_measure"]))
    check(m["speed"] == gen.SPEED and m["tempo"] == gen.TEMPO_BPM,
          "speed 6 / tempo 125 in header")
    msg = m["message"]
    check("4/4" in msg and "125 BPM" in msg, "song message states meter and tempo")
    check("reset" in msg.lower(), "song message documents the reset path")

    print("Order list")
    check(m["orders"] == [0, 1], "order list is [0, 1]", str(m["orders"]))
    check(m["orders"].count(0) == 1 and m["orders"].count(1) == 1,
          "each target Pattern appears exactly once")
    check(m["pattern_names"] == [gen.PATTERN_NAME_A, gen.PATTERN_NAME_B],
          "pattern names state the two states", str(m.get("pattern_names")))

    print("Patterns")
    all_cells = [cells_of(m["patterns"][pat], ch) for pat in (0, 1) for ch in range(4)]
    used_cmds = {c[4] for cells in all_cells for c in cells}
    check(used_cmds == {0}, "no effect commands anywhere (fixed tempo, no jumps)",
          str(used_cmds))
    for pat in (0, 1):
        check(m["num_rows"][pat] == gen.NUM_ROWS,
              f"pattern {pat} has {gen.NUM_ROWS} rows (8 bars)", str(m["num_rows"][pat]))

    print("State A (pattern 0): melody + bass, harmony empty")
    melody = cells_of(m["patterns"][0], 0)
    harmony1 = cells_of(m["patterns"][0], 1)
    harmony2 = cells_of(m["patterns"][0], 2)
    bass = cells_of(m["patterns"][0], 3)
    check(len(melody) > 0, "melody channel has notes", str(len(melody)))
    check(all(c[2] == gen.INS_PIANO for c in melody), "melody uses the piano instrument")
    check(all(54 <= c[1] <= 76 for c in melody), "melody notes in expected register")
    check(len(harmony1) == 0, "harmony voice 1 is empty")
    check(len(harmony2) == 0, "harmony voice 2 is empty")
    check(len(bass) > 0, "bass channel has notes", str(len(bass)))
    check(all(c[2] == gen.INS_BASS for c in bass), "bass uses the bass instrument")
    check({c[0] % 16 for c in bass} <= {0, 8}, "bass hits beats 1 and 3")

    print("State B (pattern 1): harmony + bass, melody empty")
    melody = cells_of(m["patterns"][1], 0)
    harmony1 = cells_of(m["patterns"][1], 1)
    harmony2 = cells_of(m["patterns"][1], 2)
    bass = cells_of(m["patterns"][1], 3)
    check(len(melody) == 0, "melody channel is empty")
    check(len(harmony1) > 0 and len(harmony2) > 0, "both harmony voices have notes")
    check(all(c[2] == gen.INS_PAD for c in harmony1 + harmony2 if c[2] != 0),
          "harmony uses the pad instrument")
    offs1 = [c for c in harmony1 if c[1] == 255]
    offs2 = [c for c in harmony2 if c[1] == 255]
    check(len(offs1) == gen.NUM_BARS and len(offs2) == gen.NUM_BARS,
          "every harmony bar ends with a note-off")
    check(len(bass) > 0, "bass channel has notes")

    print("Instruments and samples")
    for i, ins in enumerate(m["instruments"]):
        kb = {ins["keyboard"][j * 2 + 1] for j in range(120)}
        check(kb == {i + 1}, f"instrument {i + 1} '{ins['name']}' maps all notes to sample {i + 1}")
    for i, smp in enumerate(m["samples"]):
        check(smp["length"] > 0 and len(smp["pcm"]) == smp["length"],
              f"sample {i + 1} '{smp['name']}' has {smp['length']} frames of data")
        check(smp["16bit"], f"sample {i + 1} is 16-bit")
        peak = max(abs(v) for v in smp["pcm"])
        check(2000 < peak <= 32767, f"sample {i + 1} has audible non-clipping content (peak {peak})")
    # every note in the module must reference an existing instrument
    for pat in (0, 1):
        for ch in range(4):
            for (row, note, instr, vol, cmd, param) in cells_of(m["patterns"][pat], ch):
                if note not in (0, 255, 254):
                    check(1 <= instr <= 3, f"note at row {row} references instrument {instr}")

    print("Channel layout")
    check(m["channel_names"] == gen.CHANNEL_NAMES, "channel names",
          str(m.get("channel_names")))
    for ch in range(4):
        check(not (m["chnpan"][ch] & 0x80), f"channel {ch + 1} is not muted")


# ---------------------------------------------------------------------------
# Playback checks (openmpt123 render)
# ---------------------------------------------------------------------------

def goertzel(vals, rate, freq):
    w = 2.0 * math.pi * freq / rate
    coeff = 2.0 * math.cos(w)
    s1 = s2 = 0.0
    for v in vals:
        s0 = v + coeff * s1 - s2
        s2, s1 = s1, s0
    return math.sqrt(s1 * s1 + s2 * s2 - coeff * s1 * s2) / len(vals)


def check_playback(fixture_path):
    exe = next((c for c in OPENMPT123_CANDIDATES if c.exists()), None)
    if exe is None:
        print("Playback: openmpt123 not found, skipped")
        return
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        # openmpt123 --render writes the WAV next to the input file
        local = tmp / "fixture.mptm"
        local.write_bytes(fixture_path.read_bytes())
        subprocess.run([str(exe), "--render", "--output-type", "wav", "--no-float",
                        "--force", "--quiet", "--", str(local)], check=True, cwd=tmp)
        rendered = next(tmp.glob("*.wav"))
        with wave.open(str(rendered), "rb") as w:
            rate, ch, n = w.getframerate(), w.getnchannels(), w.getnframes()
            raw = w.readframes(n)
            sampwidth = w.getsampwidth()
    if sampwidth == 2:
        vals = [v / 32768.0 for v in struct.unpack("<%dh" % (len(raw) // 2), raw)]
    else:
        vals = struct.unpack("<%df" % (len(raw) // 4), raw)
    # downmix to mono: analyzing the interleaved stereo stream directly would
    # shift spectral energy (each frame duplicates the signal in time)
    if ch > 1:
        vals = [sum(vals[i:i + ch]) / ch for i in range(0, len(vals), ch)]
        ch = 1

    print("Playback")
    expect = gen.NUM_ROWS * 2 / gen.ROWS_PER_BEAT * 60.0 / gen.TEMPO_BPM  # 2 patterns = 30.72s
    check(abs(n / rate - expect) < 0.2,
          f"rendered duration {n / rate:.2f}s ~= {expect:.2f}s")

    def segment(t0, t1):
        return [v for i, v in enumerate(vals) if t0 <= (i // ch) / rate < t1]

    def rms(seg):
        return math.sqrt(sum(v * v for v in seg) / len(seg))

    half = expect / 2
    a, b = segment(0, half), segment(half, expect)
    check(rms(a) > 0.01, f"state A half is audible (rms {rms(a):.4f})")
    check(rms(b) > 0.01, f"state B half is audible (rms {rms(b):.4f})")

    # tuning spot checks (Goertzel over the first bar of each state)
    e5 = 440.0 * 2 ** ((64 - 69) / 12)   # melody E-5, bar 1 of state A
    c2 = 440.0 * 2 ** ((36 - 69) / 12)   # bass C-3, bar 1 of state A
    e3 = 440.0 * 2 ** ((52 - 69) / 12)   # harmony E-4, bar 1 of state B
    bar = gen.ROWS_PER_BAR / gen.ROWS_PER_BEAT * 60.0 / gen.TEMPO_BPM  # one 4/4 bar = 1.92s
    seg_a, seg_b = segment(0, bar), segment(half, half + bar)
    check(goertzel(seg_a, rate, e5) > 0.02, "state A bar 1 contains tuned melody E (329.6 Hz)")
    check(goertzel(seg_a, rate, c2) > 0.02, "state A bar 1 contains tuned bass C (65.4 Hz)")
    check(goertzel(seg_b, rate, e3) > 0.01, "state B bar 1 contains tuned harmony E (164.8 Hz)")


# ---------------------------------------------------------------------------

def main():
    fixture = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent / "ai-collab-fixture.mptm"
    print(f"Validating {fixture}")
    m = parse_module(fixture)
    check_structure(m)
    print("Determinism")
    check(gen.build_module() == fixture.read_bytes(),
          "generator reproduces the fixture byte-for-byte (documented reset)")
    check_playback(fixture)
    print(f"\nAll {checks_passed} checks passed.")


if __name__ == "__main__":
    main()
