#!/usr/bin/env python3
"""STR fixtures for vlc3_test (M7).

Pulls the first video frame out of data/CDData/thq.str:
  port/tests/str_frame1.bin        the frame's BS data (concatenated 2016-byte
                                   sector payloads), prefixed by a small header
  port/tests/str_frame1_golden.rgb ffmpeg's RGB24 decode of that frame
                                   (320x240x3) - the tolerance oracle

The .str files are raw 2336-byte XA sectors: [8B subheader][2328B data],
video sectors have submode bit2 clear and a 32-byte STR header at data+0.

Run from the repo root:  py port/tests/make_str_fixture.py
Requires ffmpeg on PATH (same convention as make_xa_fixture.py).
"""
import os
import struct
import subprocess
import sys

SECTOR = 2336
STR_PATH = os.path.join("data", "CDData", "thq.str")
OUT_BS = os.path.join("port", "tests", "str_frame1.bin")
OUT_RGB = os.path.join("port", "tests", "str_frame1_golden.rgb")


def main():
    with open(STR_PATH, "rb") as f:
        raw = f.read()
    nsec = len(raw) // SECTOR
    assert len(raw) % SECTOR == 0, "thq.str is not a whole number of 2336B sectors"

    chunks = {}
    n_sectors_in_frame = frame_size = width = height = None
    for i in range(nsec):
        sec = raw[i * SECTOR:(i + 1) * SECTOR]
        submode = sec[2]
        if submode & 0x04:
            continue  # audio
        data = sec[8:]
        st_id, st_type, sec_off, sec_cnt = struct.unpack_from("<HHHH", data, 0)
        frame_no, fsize = struct.unpack_from("<II", data, 8)
        if st_id != 0x0160 or st_type != 0x8001:
            continue
        if frame_no != 1:
            if frame_no > 1:
                break
            continue
        w, h = struct.unpack_from("<HH", data, 0x10)
        n_sectors_in_frame, frame_size, width, height = sec_cnt, fsize, w, h
        chunks[sec_off] = data[0x20:0x20 + 0x7E0]

    assert n_sectors_in_frame and len(chunks) == n_sectors_in_frame, (
        "frame 1: got %d of %s chunks" % (len(chunks), n_sectors_in_frame))
    bs = b"".join(chunks[i] for i in range(n_sectors_in_frame))

    ver, = struct.unpack_from("<H", bs, 6)
    decl, = struct.unpack_from("<H", bs, 0)
    print("frame 1: %dx%d, %d sectors, frameSize %d, BS version %d, "
          "declared MDEC words %d" % (width, height, n_sectors_in_frame,
                                      frame_size, ver, decl))

    #  fixture header: magic, width, height, frameSize, payload bytes
    with open(OUT_BS, "wb") as f:
        f.write(b"STF1")
        f.write(struct.pack("<HHII", width, height, frame_size, len(bs)))
        f.write(bs)

    #  ffmpeg's psxstr demuxer wants 2352-byte sectors; wrap the raw 2336-byte
    #  sectors as RIFF CDXA the way make_xa_fixture.py does.
    def sector2352(raw2336, idx):
        sync = b"\x00" + b"\xff" * 10 + b"\x00"
        mm, rest = divmod(150 + idx, 75 * 60)
        ss, ff = divmod(rest, 75)
        bcd = lambda v: ((v // 10) << 4) | (v % 10)
        return sync + bytes([bcd(mm), bcd(ss), bcd(ff), 2]) + raw2336

    payload = b"".join(sector2352(raw[i * SECTOR:(i + 1) * SECTOR], i)
                       for i in range(nsec))
    fmt = b"fmt " + struct.pack("<I", 16) + b"\x00" * 16
    dat = b"data" + struct.pack("<I", len(payload)) + payload
    riff = b"RIFF" + struct.pack("<I", 4 + len(fmt) + len(dat)) + b"CDXA" + fmt + dat
    xa_path = os.path.join(os.environ["TEMP"], "str_fixture.xa")
    with open(xa_path, "wb") as f:
        f.write(riff)

    subprocess.run(
        ["ffmpeg", "-y", "-loglevel", "error", "-i", xa_path,
         "-frames:v", "1", "-f", "rawvideo", "-pix_fmt", "rgb24", OUT_RGB],
        check=True)
    os.remove(xa_path)
    sz = os.path.getsize(OUT_RGB)
    assert sz == width * height * 3, "golden is %d bytes, want %d" % (
        sz, width * height * 3)
    print("wrote %s (%d bytes) and %s (%d bytes)" % (
        OUT_BS, os.path.getsize(OUT_BS), OUT_RGB, sz))


if __name__ == "__main__":
    main()
