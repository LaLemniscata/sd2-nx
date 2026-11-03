#!/usr/bin/env python3
"""Rewrite source/sd2_entrypoints.h offsets from libunity.so's RegisterNatives tables.

Usage: tools/extract_entrypoints.py [--check]    (run from the repository root)

Walks R_AARCH64_RELATIVE relocations looking for JNINativeMethod triples
{ name, signature, function }. Reports defines with no native in this build.
"""
import re
import struct
import sys

R_AARCH64_RELATIVE = 1027


def load(path):
    data = open(path, "rb").read()
    phoff = struct.unpack_from("<Q", data, 32)[0]
    phsize, phnum = struct.unpack_from("<HH", data, 54)
    loads, dynamic = [], None
    for i in range(phnum):
        p_type, _, off, vaddr, _, filesz, _, _ = struct.unpack_from("<IIQQQQQQ", data, phoff + i * phsize)
        if p_type == 1:
            loads.append((vaddr, off, filesz))
        elif p_type == 2:
            dynamic = (off, filesz)
    return data, loads, dynamic


def to_file(loads, vaddr):
    for start, off, size in loads:
        if start <= vaddr < start + size:
            return off + vaddr - start
    return None


def relocations(data, loads, dynamic):
    tags = {}
    for i in range(dynamic[1] // 16):
        tag, value = struct.unpack_from("<QQ", data, dynamic[0] + i * 16)
        tags[tag] = value
    base = to_file(loads, tags[7])          # DT_RELA
    relocs = {}
    for i in range(tags[8] // 24):          # DT_RELASZ
        offset, info, addend = struct.unpack_from("<QQq", data, base + i * 24)
        if info & 0xffffffff == R_AARCH64_RELATIVE:
            relocs[offset] = addend
    return relocs


def cstring(data, loads, vaddr):
    off = to_file(loads, vaddr)
    if off is None:
        return None
    end = data.find(b"\0", off, off + 512)
    if end < 0:
        return None
    try:
        return data[off:end].decode("ascii")
    except UnicodeDecodeError:
        return None


def main():
    check = "--check" in sys.argv
    data, loads, dynamic = load("game/lib/libunity.so")
    relocs = relocations(data, loads, dynamic)
    natives = {}
    for offset, addend in relocs.items():
        if offset + 8 not in relocs or offset + 16 not in relocs:
            continue
        name = cstring(data, loads, addend)
        sig = cstring(data, loads, relocs[offset + 8])
        if name and sig and re.fullmatch(r"\w+", name) and sig.startswith("("):
            natives[name] = (relocs[offset + 16], sig)
    print("natives found:", len(natives))

    path = "source/sd2_entrypoints.h"
    text = open(path, newline="").read()
    missing = []

    def replace(m):
        found = natives.get(m.group(2))
        if not found:
            missing.append(m.group(2))
            return m.group(0)
        return "%s0x%08x /* %s */" % (m.group(1), found[0], found[1])

    new = re.sub(r"^(#define OFF_(\w+)\s+)0x[0-9a-fA-F]+ /\*.*?\*/", replace, text, flags=re.M)
    if not check and new != text:
        open(path, "w", newline="").write(new)
    print("defines with no native in this build:", missing)
    print("nativeUnityPlayerSetRunning" in natives, "<- nativeUnityPlayerSetRunning present")
    return 0


if __name__ == "__main__":
    sys.exit(main())
