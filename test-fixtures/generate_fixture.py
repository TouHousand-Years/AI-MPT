#!/usr/bin/env python3
"""Generate the OpenMPT-for-AI pattern-collaboration MPTM fixture.

Writes ``ai-collab-fixture.mptm``: a purpose-built, musically credible test
module for the first vertical slice (issue 20 / ticket 28).

  * fixed meter and tempo (4/4, 4 rows per beat, 16 rows per bar,
    125 BPM classic tempo mode, speed 6, no tempo changes anywhere)
  * pre-existing piano, harmony and bass sounds (embedded 16-bit samples,
    no missing-sample warnings, no placeholder noise)
  * two resettable 8-bar starting states, each a target Pattern that appears
    exactly once in the Order list:
      Pattern 0 "AI State A - Melody":  melody + bass, harmony voices empty
      Pattern 1 "AI State B - Harmony": harmony + bass, melody voice empty
  * a song message documenting meter, tempo, channels and how to reset

The file layout mirrors what OpenMPT's own ``CSoundFile::SaveIT`` writes for
an MPTM module (see openmpt-original_ref/soundlib/Load_it.cpp), including the
"STPM" song-extension block and the trailing Ssb("mptm") extension with the
serialized sequence. The generator is deterministic; running it again
reproduces the pristine fixture byte-for-byte (that is the documented reset).

Usage:  python generate_fixture.py [output.mptm]
"""

import math
import struct
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Format constants (see openmpt-original_ref/soundlib)
# ---------------------------------------------------------------------------

# Tracker note numbers: NOTE_MIN = 1 (C-0), NOTE_MIDDLEC = 61 (C-5) == MIDI 60.
C5 = 61
NOTE_KEYOFF = 0xFF

# MPTM "made with" / "compatible with" version fields.
VER_MPT_FILE = 0x891   # verMptFileVer (cwtv for MPTM)
CMWT = 0x888           # cmwt for MPTM
OPENMPT_VERSION = 0x01210019  # 1.33.00.25, this snapshot's Version::Current()

# Default MPT playback behaviour (CSoundFile::GetDefaultPlaybackBehaviour),
# expressed as PlayBehaviour enum indices from soundlib/Snd_defs.h.
MPT_PLAY_BEHAVIOUR_BITS = [
    7,    # kPeriodsAreHertz
    9,    # kPerChannelGlobalVolSlide
    10,   # kPanOverride
    13,   # kITArpeggio
    15,   # kITPortaMemoryShare
    16,   # kITPatternLoopTargetReset
    17,   # kITFT2PatternLoop
    18,   # kITPingPongNoReset
    20,   # kITClearOldNoteAfterCut
    21,   # kITVibratoTremoloPanbrello
    24,   # kITMultiSampleBehaviour
    25,   # kITPortaTargetReached
    26,   # kITPatternLoopBreak
    28,   # kITSwingBehaviour
    30,   # kITSCxStopsSample
    31,   # kITEnvelopePositionHandling
    33,   # kITPingPongMode
    34,   # kITRealNoteMapping
    39,   # kITPortaNoNote
    41,   # kITVolColMemory
    44,   # kITFirstTickHandling
    45,   # kITSampleAndHoldPanbrello
    46,   # kITClearPortaTarget
    47,   # kITPanbrelloHold
    48,   # kITPanningReset
    50,   # kITInstrWithNoteOff
    99,   # kOPLFlexibleNoteOff
    102,  # kITDoNotOverrideChannelPan
    104,  # kITDCTBehaviour
    105,  # kOPLwithNNA
    115,  # kITPitchPanSeparation
]
K_MAX_PLAY_BEHAVIOURS = 138

# ---------------------------------------------------------------------------
# Musical layout
# ---------------------------------------------------------------------------

SAMPLE_RATE = 44100
ROWS_PER_BEAT = 4
ROWS_PER_BAR = 16          # 4/4
NUM_BARS = 8
NUM_ROWS = ROWS_PER_BAR * NUM_BARS   # 128 = 8 bars
TEMPO_BPM = 125            # classic tempo mode
SPEED = 6

# Channels
CH_MELODY = 0
CH_HARMONY1 = 1
CH_HARMONY2 = 2
CH_BASS = 3
NUM_CHANNELS = 4
CHANNEL_NAMES = ["Melody", "Harmony 1", "Harmony 2", "Bass"]
CHANNEL_PAN = {  # IT header pan byte, 0..64 (= nPan/4); bit 0x80 would mute
    CH_MELODY: 32,    # center
    CH_HARMONY1: 24,  # slightly left
    CH_HARMONY2: 40,  # slightly right
    CH_BASS: 32,      # center
}

# Instruments / samples (1-based as used inside the module)
INS_PIANO, INS_PAD, INS_BASS = 1, 2, 3

# Chord progression: C  Am  F  G, twice. (Bar, chord root/fifth for bass.)
PROGRESSION = ["C", "Am", "F", "G", "C", "Am", "F", "G"]
BASS_ROOT = {"C": 37, "Am": 34, "F": 30, "G": 32}  # C-3, A-2, F-2, G-2

# State B harmony voicing (chord tone above / below), whole-bar pad notes.
HARMONY_VOICE1 = {"C": 53, "Am": 49, "F": 49, "G": 48}  # E-4, C-4, C-4, B-3
HARMONY_VOICE2 = {"C": 49, "Am": 45, "F": 45, "G": 44}  # C-4, A-3, A-3, G-3

# State A melody (piano), one phrase per bar: half note + two quarters.
# Tracker note numbers: C-5 = 61, so E-5 = 65, G-5 = 68, etc.
MELODY = [
    # bar 1 (C):  E-5 half, G-5, E-5
    [(0, 65), (8, 68), (12, 65)],
    # bar 2 (Am): C-5 half, B-4, A-4
    [(16, 61), (24, 60), (28, 58)],
    # bar 3 (F):  A-4 half, G-4, F-4
    [(32, 58), (40, 56), (44, 54)],
    # bar 4 (G):  G-4 half, A-4, B-4
    [(48, 56), (56, 58), (60, 60)],
    # bar 5 (C):  C-5 half, D-5, E-5
    [(64, 61), (72, 63), (76, 65)],
    # bar 6 (Am): E-5 half, D-5, C-5
    [(80, 65), (88, 63), (92, 61)],
    # bar 7 (F):  F-5 half, E-5, D-5
    [(96, 66), (104, 65), (108, 63)],
    # bar 8 (G):  D-5 half, B-4, G-4
    [(112, 63), (120, 60), (124, 56)],
]

SONG_NAME = "AI Collab Fixture"
ARTIST = "OpenMPT for AI"
PATTERN_NAME_A = "AI State A - Melody"
PATTERN_NAME_B = "AI State B - Harmony"

SONG_MESSAGE = "\r\n".join([
    "OpenMPT for AI - Pattern collaboration fixture",
    "==============================================",
    "",
    "Purpose",
    "-------",
    "Purpose-built test module for the first vertical slice",
    "(melody <-> harmony collaboration with a local AI agent over",
    "MCP). It is a test fixture, not a finished song.",
    "",
    "Fixed timing (do not change)",
    "----------------------------",
    "Meter    : 4/4 (4 rows per beat, 16 rows per bar)",
    "Tempo    : 125 BPM, classic tempo mode, speed 6",
    "           There are no tempo or speed changes anywhere.",
    "Patterns : 128 rows = 8 bars each.",
    "",
    "Instruments and channels",
    "------------------------",
    "Channel 1 (Melody)    : instrument 1 \"E-Piano\"",
    "Channel 2 (Harmony 1) : instrument 2 \"Warm Pad\"",
    "Channel 3 (Harmony 2) : instrument 2 \"Warm Pad\"",
    "Channel 4 (Bass)      : instrument 3 \"Round Bass\"",
    "",
    "Starting states",
    "---------------",
    "Each target Pattern appears exactly once in the order list.",
    "",
    "Pattern 0 \"AI State A - Melody\":",
    "  Melody (ch 1) and bass (ch 4) are written; the harmony",
    "  voices (ch 2, ch 3) are empty. Use this state for the",
    "  melody -> harmony collaboration test.",
    "",
    "Pattern 1 \"AI State B - Harmony\":",
    "  Harmony (ch 2, ch 3) and bass (ch 4) are written; the",
    "  melody voice (ch 1) is empty. Use this state for the",
    "  harmony -> melody collaboration test.",
    "",
    "How to reset a state",
    "--------------------",
    "AI edits land in the working copy in memory. To reset:",
    "1. Close the module without saving (or undo by hand).",
    "2. Reopen this pristine file; both patterns are back in",
    "   their original states.",
    "Keep this file pristine: use Save As for every working",
    "copy and never save over this file.",
    "",
]) + "\r\n"

# ---------------------------------------------------------------------------
# Sample synthesis (mono 16-bit PCM)
# ---------------------------------------------------------------------------

def _synthesize(freq, partials, taus, length_s, attack_s, sustain_drop=0.0):
    """Additive synthesis with per-partial exponential decay.

    partials: list of (frequency_multiplier, amplitude)
    taus:     matching decay time constants in seconds
    """
    n = int(SAMPLE_RATE * length_s)
    out = [0.0] * n
    attack_n = max(1, int(attack_s * SAMPLE_RATE))
    for (mult, amp), tau in zip(partials, taus):
        w = 2.0 * math.pi * freq * mult / SAMPLE_RATE
        for i in range(n):
            t = i / SAMPLE_RATE
            env = amp * math.exp(-t / tau)
            if sustain_drop:
                env *= max(0.0, 1.0 - sustain_drop * t / length_s)
            out[i] += env * math.sin(w * i)
    # soft attack ramp
    for i in range(attack_n):
        ramp = 0.5 - 0.5 * math.cos(math.pi * i / attack_n)
        out[i] *= ramp
    # final 10 ms fade to zero so the sample cannot click
    fade_n = min(int(0.010 * SAMPLE_RATE), n)
    for i in range(fade_n):
        out[n - 1 - i] *= i / fade_n
    peak = max(abs(v) for v in out) or 1.0
    return [int(max(-32768, min(32767, round(v / peak * 32767 * 0.75)))) for v in out]


def synth_piano():
    """Electric-piano-like decaying tone at middle C (261.63 Hz)."""
    f0 = 440.0 * 2 ** ((60 - 69) / 12)  # MIDI 60 = middle C = tracker C-5
    partials = [(n, 1.0 / n ** 1.6) for n in range(1, 9)]
    taus = [1.5 / (1 + 0.85 * (n - 1)) for n in range(1, 9)]
    return _synthesize(f0, partials, taus, 3.2, 0.006)


def synth_pad():
    """Soft sustained pad at middle C; slow attack, gentle baked-in decay."""
    f0 = 440.0 * 2 ** ((60 - 69) / 12)
    partials, taus = [], []
    for detune in (1.0, 1.0025):
        for n, amp in ((1, 1.0), (2, 0.45), (3, 0.18), (4, 0.08)):
            partials.append((n * detune, amp * 0.5))
            taus.append(6.0)  # nearly no decay; sustain_drop does the work
    return _synthesize(f0, partials, taus, 4.5, 0.15, sustain_drop=0.25)


def synth_bass():
    """Round decaying bass tone at C2 (two octaves below middle C)."""
    f0 = 440.0 * 2 ** ((36 - 69) / 12)  # MIDI 36 = tracker C-3
    partials = [(n, 1.0 / n ** 1.1) for n in range(1, 6)]
    taus = [1.2 / (1 + 0.7 * (n - 1)) for n in range(1, 6)]
    return _synthesize(f0, partials, taus, 2.4, 0.004)


# ---------------------------------------------------------------------------
# IT / MPTM structure writers
# ---------------------------------------------------------------------------

def _pad(buf: bytes, size: int, fill=b"\x00") -> bytes:
    assert len(buf) <= size
    return buf + fill * (size - len(buf))


def _name_field(text: str, size: int, space_pad: bool = False) -> bytes:
    raw = text.encode("ascii")
    assert len(raw) < size, text
    if space_pad:
        return raw + b" " * (size - len(raw))
    return _pad(raw, size)


def build_header(song_name, num_orders, num_ins, num_smp, num_pat,
                 msg_len, msg_off, globalvol=128, mv=48, speed=SPEED,
                 tempo=TEMPO_BPM):
    flags = 0x01   # useStereoPlayback (always set by SaveIT)
    flags |= 0x04  # instrumentMode
    flags |= 0x08  # linearSlides (MPTM default)
    flags |= 0x40  # useMIDIPitchController (always set by SaveIT)
    flags |= 0x1000  # extendedFilterRange (MPTM default)
    special = 0x01 | 0x02 | 0x04  # song message | edit history | highlights

    h = b"IMPM"
    h += _name_field(song_name, 26, space_pad=True)
    h += bytes([ROWS_PER_BEAT, ROWS_PER_BAR])       # highlight minor/major
    h += struct.pack("<4H", num_orders, num_ins, num_smp, num_pat)
    h += struct.pack("<4H", VER_MPT_FILE, CMWT, flags, special)
    h += bytes([globalvol, mv, speed, tempo, 128, 0])  # gvol mv speed tempo sep pwd
    h += struct.pack("<HI", msg_len, msg_off)       # message length / offset
    h += struct.pack("<I", 0)                       # reserved
    h += bytes([CHANNEL_PAN.get(i, 0xA0 if i >= NUM_CHANNELS else 0x20)
                for i in range(64)])                # channel panning
    h += bytes([64] * 64)                           # channel volume
    assert len(h) == 192
    return h


def build_instrument(name, sample_no, fadeout, volenv=None):
    """ITInstrument (554 bytes). volenv: (flags, [(value, tick), ...])."""
    env = bytearray()
    if volenv is None:
        env += bytes([0, 0, 0, 0, 0, 0])            # disabled, 0 nodes
        env += b"\x00" * 75                          # 25 unused nodes
        env += b"\x00"                               # reserved
    else:
        env_flags, nodes = volenv
        assert 1 <= len(nodes) <= 25
        env += bytes([env_flags, len(nodes), 0, 0, 1, 1])  # sustain = node 1
        for value, tick in nodes:
            env += struct.pack("<bH", value, tick)
        env += b"\x00" * (75 - 3 * len(nodes))
        env += b"\x00"
    assert len(env) == 82
    disabled_env = b"\x00" * 82

    ins = b"IMPI"
    ins += b"\x00" * 13                              # DOS filename
    ins += bytes([0, 0, 0])                          # nna=cut, dct, dca
    ins += struct.pack("<H", fadeout)
    ins += struct.pack("<bB", 0, 0)                  # pps, ppc
    ins += bytes([128, 0x80])                        # gbv, dfp (ignore panning)
    ins += bytes([0, 0])                             # vol swing, pan swing
    ins += struct.pack("<H", 0)                      # trkvers
    ins += bytes([1, 0])                             # nos, reserved
    ins += _name_field(name, 26, space_pad=True)
    ins += bytes([0, 0, 0, 0])                       # ifc, ifr, mch, mpr
    ins += b"\x00" * 2                               # mbank
    ins += b"".join(bytes([note, sample_no]) for note in range(120))  # keyboard
    ins += bytes(env)                              # volume envelope
    ins += disabled_env                            # panning envelope (off)
    ins += disabled_env                            # pitch envelope (off!) - an
    # enabled pitch envelope would bend the instrument's pitch
    ins += b"\x00" * 4                               # dummy (no XTPM extension)
    assert len(ins) == 554
    return ins


def build_sample_header(name, pcm, c5_speed):
    """ITSample header (80 bytes). Sample pointer patched by the caller."""
    smp = b"IMPS"
    smp += b"\x00" * 13                              # DOS filename
    smp += bytes([64, 0x03, 64])                     # gvl, flags(present|16bit), vol
    smp += _name_field(name, 26, space_pad=True)
    smp += bytes([0x01, 0x00])                       # cvt=signed, dfp
    smp += struct.pack("<6I", len(pcm), 0, 0, c5_speed, 0, 0)
    smp += struct.pack("<I", 0)                      # sample pointer (patched)
    smp += bytes([0, 0, 0, 0])                       # autovibrato
    assert len(smp) == 80
    return smp


def encode_pattern(cells, num_rows):
    """IT-packed pattern data. cells: {(row, ch): (note, instr, vol)}.

    note: tracker note number (1..120) or NOTE_KEYOFF; instr: 1-based;
    vol: volume-column value 0..64. None means "field not present".
    Always writes an explicit channel mask (bit 0x80), which is a valid
    (if unoptimized) encoding of the IT pattern format.
    """
    data = bytearray()
    for row in range(num_rows):
        for ch in range(NUM_CHANNELS):
            cell = cells.get((row, ch))
            if cell is None:
                continue
            note, instr, vol = cell
            mask = 0
            payload = bytearray()
            if note is not None:
                mask |= 0x01
                payload.append(NOTE_KEYOFF if note == NOTE_KEYOFF else note - 1)
            if instr is not None:
                mask |= 0x02
                payload.append(instr)
            if vol is not None:
                mask |= 0x04
                payload.append(vol)
            data.append((ch + 1) | 0x80)
            data.append(mask)
            data += payload
        data.append(0)  # end of row
    return struct.pack("<HHHH", len(data), num_rows, 0, 0) + bytes(data)


# ---------------------------------------------------------------------------
# MPTM extension blocks
# ---------------------------------------------------------------------------

def _adaptive16(value):
    assert value < 0x4000
    if value < 0x40:
        return struct.pack("<B", (value << 2) | 0x00)
    return struct.pack("<H", (value << 2) | 0x01)


def _adaptive32(value):
    assert value < 0x40000000
    if value < 0x40:
        return struct.pack("<B", (value << 2) | 0x00)
    if value < 0x4000:
        return struct.pack("<H", (value << 2) | 0x01)
    return struct.pack("<I", (value << 2) | 0x02)


def _adaptive64(value, fixed_size=0):
    if fixed_size == 2:
        assert value < 0x4000
        return struct.pack("<H", (value << 2) | 0x01)
    if fixed_size == 8:
        assert value < 0x4000000000000000
        return struct.pack("<Q", (value << 2) | 0x03)
    return _adaptive32(value) if value < 0x40000000 else struct.pack("<Q", (value << 2) | 0x03)


class SsbWriter:
    """Byte-exact replica of srlztn::SsbWrite as configured by SaveIT
    (variable-size entry ids, start pos + size in map, version field)."""

    def __init__(self, obj_id: bytes, version: int):
        self._buf = bytearray()
        self._buf += b"228"
        self._buf += bytes([len(obj_id)]) + obj_id
        self._buf += bytes([0x1F])            # header: idbytes=3, startpos, size, version
        self._buf += _adaptive32(2)           # header data size
        self._buf += b"\x00"                  # HeaderId_FlagByte
        self._buf += b"\x01"                  # flags: variable-size ids
        self._buf += _adaptive64(version)
        self._buf += b"\x01"                  # id byte count marker (IdSizeVariable)
        self._entrycount_pos = len(self._buf)
        self._buf += b"\x00\x00"              # entry count placeholder
        self._mappos_pos = len(self._buf)
        self._buf += b"\x00" * 8              # map rpos placeholder
        self._items = 0
        self._map = bytearray()

    def write_item(self, item_id: bytes, data: bytes):
        rpos = len(self._buf)
        self._buf += data
        self._map += _adaptive16(len(item_id)) + item_id
        self._map += _adaptive64(rpos) + _adaptive64(len(data))
        self._items += 1

    def finish(self) -> bytes:
        map_rpos = len(self._buf)
        self._buf += self._map
        self._buf[self._entrycount_pos:self._entrycount_pos + 2] = _adaptive64(self._items, 2)
        self._buf[self._mappos_pos:self._mappos_pos + 8] = _adaptive64(map_rpos, 8)
        return bytes(self._buf)


def _item_string(text: str) -> bytes:
    """srlztn::WriteItemString: u32 ((len<<4)|12) followed by the bytes."""
    raw = text.encode("utf-8")
    return struct.pack("<I", (len(raw) << 4) | 12) + raw


def build_sequences_extension(order, tempo_raw, speed):
    """The Ssb("mptm") block SaveIT appends: one "mptSeqC" item holding the
    default sequence (WriteModSequences -> WriteModSequence)."""
    seq = SsbWriter(b"mptSeq", OPENMPT_VERSION)
    seq.write_item(b"u", b"\x01")                      # useUTF8
    seq.write_item(b"n", _item_string(""))             # sequence name (default: empty)
    seq.write_item(b"l", struct.pack("<H", len(order)))
    seq.write_item(b"a", b"".join(struct.pack("<H", p) for p in order))
    seq.write_item(b"t", struct.pack("<I", tempo_raw))
    seq.write_item(b"s", struct.pack("<I", speed))
    seq_data = seq.finish()

    seqs = SsbWriter(b"mptSeqC", OPENMPT_VERSION)
    seqs.write_item(b"n", b"\x01")                     # one sequence
    seqs.write_item(b"c", b"\x00")                     # current sequence
    seqs.write_item(b"\x00", seq_data)                 # sequence 0
    seqs_data = seqs.finish()

    top = SsbWriter(b"mptm", OPENMPT_VERSION)
    top.write_item(b"mptSeqC", seqs_data)
    return top.finish()


def build_song_extensions(num_channels):
    """The "STPM" song-extension block (CSoundFile::SaveExtendedSongProperties
    for a fresh MPTM document in classic tempo mode).

    Chunk codes appear byte-reversed on disk (the loader reads them as
    little-endian uint32 of the big-endian magic), e.g. "C..." -> "...C".
    """
    out = bytearray(b"STPM")

    def chunk(code, payload):
        out.extend(code)
        out.extend(struct.pack("<H", len(payload)))
        out.extend(payload)

    chunk(b"...C", struct.pack("<H", num_channels))    # C... channel count
    chunk(b".MMP", struct.pack("<i", 3))               # PMM. MixLevels v1_17RC3 (MPTM default)
    chunk(b".VWC", struct.pack("<I", OPENMPT_VERSION))  # CWV. created with
    chunk(b"VWSL", struct.pack("<I", OPENMPT_VERSION))  # LSWV last saved with
    chunk(b"VTSV", struct.pack("<i", 48))              # VSTV VSTi volume
    bits = bytearray((K_MAX_PLAY_BEHAVIOURS + 7) // 8)
    for bit in MPT_PLAY_BEHAVIOUR_BITS:
        bits[bit // 8] |= 1 << (bit % 8)
    chunk(b".FSM", bytes(bits))                        # MSF. playback behaviour
    chunk(b"AUTH", ARTIST.encode("utf-8"))             # MagicLE: stored as-is
    return bytes(out)


# ---------------------------------------------------------------------------
# Musical content
# ---------------------------------------------------------------------------

def _add_bass_line(cells):
    """Two half-note roots per bar (beats 1 and 3) on the bass channel."""
    for bar, chord in enumerate(PROGRESSION):
        root = BASS_ROOT[chord]
        base = bar * ROWS_PER_BAR
        cells[(base, CH_BASS)] = (root, INS_BASS, 60)
        cells[(base + 8, CH_BASS)] = (root, INS_BASS, 56)


def build_state_a_cells():
    """Pattern 0: melody (piano) + bass; harmony channels empty."""
    cells = {}
    for bar_notes in MELODY:
        for row, note in bar_notes:
            is_half_note = (row % ROWS_PER_BAR) < 8
            cells[(row, CH_MELODY)] = (note, INS_PIANO, 64 if is_half_note else 56)
    _add_bass_line(cells)
    return cells


def build_state_b_cells():
    """Pattern 1: harmony (pads) + bass; melody channel empty."""
    cells = {}
    for bar, chord in enumerate(PROGRESSION):
        base = bar * ROWS_PER_BAR
        cells[(base, CH_HARMONY1)] = (HARMONY_VOICE1[chord], INS_PAD, 56)
        cells[(base + ROWS_PER_BAR - 1, CH_HARMONY1)] = (NOTE_KEYOFF, None, None)
        cells[(base, CH_HARMONY2)] = (HARMONY_VOICE2[chord], INS_PAD, 48)
        cells[(base + ROWS_PER_BAR - 1, CH_HARMONY2)] = (NOTE_KEYOFF, None, None)
    _add_bass_line(cells)
    return cells


# ---------------------------------------------------------------------------
# Module assembly (mirrors CSoundFile::SaveIT for MOD_TYPE_MPT)
# ---------------------------------------------------------------------------

def build_module():
    piano = synth_piano()
    pad = synth_pad()
    bass = synth_bass()
    samples = [
        ("E-Piano", piano, SAMPLE_RATE),          # waveform is middle C
        ("Warm Pad", pad, SAMPLE_RATE),           # waveform is middle C
        ("Round Bass", bass, SAMPLE_RATE * 4),    # waveform is C2 -> C5Speed x4
    ]
    patterns = [encode_pattern(build_state_a_cells(), NUM_ROWS),
                encode_pattern(build_state_b_cells(), NUM_ROWS)]
    instruments = [
        build_instrument("E-Piano", 1, fadeout=96),
        build_instrument("Warm Pad", 2, fadeout=192,
                         volenv=(0x05, [(0, 0), (64, 8), (0, 48)])),  # on + sustain; (value, tick)
        build_instrument("Round Bass", 3, fadeout=96),
    ]

    num_orders = 2          # each target pattern exactly once
    ins_ptrs = [0] * 3
    smp_ptrs = [0] * 3
    pat_ptrs = [0, 0]

    # --- layout pass -------------------------------------------------------
    dw_hdr_pos = 192 + num_orders
    ptr_bytes = (3 + 3 + 2) * 4
    history_size = 2          # u16 entry count = 0
    pnam_size = 8 + 2 * 32
    cnam_size = 8 + NUM_CHANNELS * 20
    dw_extra = history_size + pnam_size + cnam_size
    msg_offset = dw_hdr_pos + dw_extra + ptr_bytes
    msg = SONG_MESSAGE.encode("ascii")
    msg_len = len(msg) + 1    # SaveIT writes length + 1 (trailing NUL)

    pos = msg_offset + msg_len
    for i in range(3):
        ins_ptrs[i] = pos
        pos += 554
    for i in range(3):
        smp_ptrs[i] = pos
        pos += 80
    for i in range(2):
        pat_ptrs[i] = pos
        pos += len(patterns[i])
    sample_data_ptrs = []
    for i, (_, pcm, _) in enumerate(samples):
        sample_data_ptrs.append(pos)
        pos += len(pcm) * 2
    stpm_pos = pos
    stpm = build_song_extensions(NUM_CHANNELS)
    pos += len(stpm)
    ssb_pos = pos
    ssb = build_sequences_extension([0, 1], TEMPO_BPM * 10000, SPEED)
    pos += len(ssb)
    pos += 4  # trailing u32 mptStartPos

    # --- emit --------------------------------------------------------------
    out = bytearray()
    out += build_header(SONG_NAME, num_orders, 3, 3, 2, msg_len, msg_offset)
    out += bytes([0, 1])                                   # order list
    out += struct.pack("<3I", *ins_ptrs)
    out += struct.pack("<3I", *smp_ptrs)
    out += struct.pack("<2I", *pat_ptrs)
    out += struct.pack("<H", 0)                            # empty edit history
    out += b"PNAM" + struct.pack("<I", 64)
    out += _name_field(PATTERN_NAME_A, 32) + _name_field(PATTERN_NAME_B, 32)
    out += b"CNAM" + struct.pack("<I", NUM_CHANNELS * 20)
    for name in CHANNEL_NAMES:
        out += _name_field(name, 20)
    out += msg + b"\x00"
    assert len(out) == msg_offset + msg_len, (len(out), msg_offset + msg_len)

    for ins in instruments:
        out += ins
    for i, (name, pcm, c5_speed) in enumerate(samples):
        hdr = bytearray(build_sample_header(name, pcm, c5_speed))
        hdr[72:76] = struct.pack("<I", sample_data_ptrs[i])  # samplepointer field
        out += hdr
    for pat in patterns:
        out += pat
    for _, pcm, _ in samples:
        out += struct.pack("<%dh" % len(pcm), *pcm)
    assert len(out) == stpm_pos
    out += stpm
    assert len(out) == ssb_pos
    out += ssb
    out += struct.pack("<I", ssb_pos)
    return bytes(out)


def main():
    out_path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent / "ai-collab-fixture.mptm"
    data = build_module()
    out_path.write_bytes(data)
    print(f"wrote {out_path} ({len(data)} bytes)")


if __name__ == "__main__":
    main()
