#!/usr/bin/env python3
"""Stereo XA fixture for xa_test (M7).

Takes the FIRST TWO audio sectors of data/CDData/thq.str (STR movie audio:
coding 0x01 = stereo 37.8kHz 4-bit), saves them raw, and decodes them with
ffmpeg's adpcm_xa (bit-exact oracle, as in make_xa_fixture.py) to
interleaved s16le stereo.  Both decoders start from zero history at the
first sector, so the golden matches a fresh-stream decode.

Run from the repo root:  py port/tests/make_xa_stereo_fixture.py
Writes port/tests/xa_stereo_sectors.bin and xa_stereo_golden.pcm.
"""
import os
import struct
import subprocess

STR_PATH = os.path.join("data", "CDData", "thq.str")
SEC = 2336

raw = open(STR_PATH, "rb").read()
nsec = len(raw) // SEC
sectors = []
for i in range(nsec):
    s = raw[i * SEC:(i + 1) * SEC]
    if s[2] & 0x04:            # audio submode
        assert s[3] == 0x01, "expected coding 0x01 (stereo 37.8k 4-bit)"
        sectors.append(s)
        if len(sectors) == 2:
            break
assert len(sectors) == 2

with open("port/tests/xa_stereo_sectors.bin", "wb") as f:
    for s in sectors:
        f.write(s)


#  A tiny CDXA wrap fails ffmpeg's probe, so wrap the WHOLE movie (as
#  make_str_fixture.py does) and take just the first two audio packets -
#  the decoder starts those from zero history either way.
def sector2352(raw2336, idx):
    sync = b"\x00" + b"\xff" * 10 + b"\x00"
    mm, rest = divmod(150 + idx, 75 * 60)
    ss, ff = divmod(rest, 75)
    bcd = lambda v: ((v // 10) << 4) | (v % 10)
    return sync + bytes([bcd(mm), bcd(ss), bcd(ff), 2]) + raw2336


payload = b"".join(sector2352(raw[i * SEC:(i + 1) * SEC], i)
                   for i in range(nsec))
fmt = b"fmt " + struct.pack("<I", 16) + b"\x00" * 16
dat = b"data" + struct.pack("<I", len(payload)) + payload
riff = b"RIFF" + struct.pack("<I", 4 + len(fmt) + len(dat)) + b"CDXA" + fmt + dat
xa_path = os.path.join(os.environ["TEMP"], "xa_stereo_fixture.xa")
open(xa_path, "wb").write(riff)

subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", xa_path,
                "-map", "0:a:0", "-frames:a", "2",
                "-f", "s16le", "-acodec", "pcm_s16le",
                "port/tests/xa_stereo_golden.pcm"], check=True)
os.remove(xa_path)
got = os.path.getsize("port/tests/xa_stereo_golden.pcm")
print("golden pcm bytes:", got, "(expect", 2 * 2016 * 2 * 2, ")")
assert got == 2 * 2016 * 2 * 2
