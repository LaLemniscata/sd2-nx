#!/usr/bin/env python3
"""Check every engine and il2cpp address in source/ against your copy of the game.

Usage: tools/check_engine.py    (run from the repository root)

Reads game/lib/libunity.so and game/lib/libil2cpp.so. Nothing is patched or
written. Exits non-zero on the first class of mismatch it finds, so a build
with a stale address is never sent to the console.
"""
import re
import struct
import sys

failures = []


def expect(ok, label, detail=""):
    print("%-4s %s %s" % ("ok" if ok else "FAIL", label, detail))
    if not ok:
        failures.append(label)


class Elf:
    def __init__(self, path):
        self.data = open(path, "rb").read()
        phoff = struct.unpack_from("<Q", self.data, 32)[0]
        phsize, phnum = struct.unpack_from("<HH", self.data, 54)
        self.segs = [struct.unpack_from("<IIQQQQQQ", self.data, phoff + i * phsize) for i in range(phnum)]
        text = next(s for s in self.segs if s[0] == 1 and s[1] & 1)
        self.text_off, self.text_va, self.text_size = text[2], text[3], text[5]
        self.words = struct.unpack_from("<%dI" % (self.text_size // 4), self.data, self.text_off)

    def word(self, va):
        i = (va - self.text_va) // 4
        return self.words[i] if 0 <= i < len(self.words) else None

    def in_bss(self, va):
        return any(s[0] == 1 and s[1] & 2 and s[3] + s[5] <= va < s[3] + s[6] for s in self.segs)


def defines(text):
    return {n: int(v, 16) for n, v in re.findall(r"^#define (\w+)\s+(0x[0-9a-fA-F]+)u?\b", text, re.M)}


def check_engine():
    unity = Elf("game/lib/libunity.so")
    src = open("source/sd2_offsets.h", errors="replace").read()
    d = defines(src)

    for fp_name, rva, guard in (("FP_TM_UPDATE", "GAME_RVA_TM_UPDATE", "GUARD_TM_UPDATE"),
                                ("FP_CHOREO_GET", "GAME_RVA_CHOREO_GET", "GUARD_CHOREO_GET"),
                                ("FP_AUDIO_OUTTYPE", "GAME_RVA_AUDIO_OUTTYPE", "GUARD_AUDIO_OUTTYPE"),
                                ("FP_FMOD_BUFGEOM", "GAME_RVA_FMOD_BUFGEOM", "GUARD_FMOD_BUFGEOM")):
        body = re.search(r"%s\[\] = \{(.*?)\};" % fp_name, src, re.S).group(1)
        fp = [(int(p, 16), int(m, 16)) for p, m in re.findall(r"\{ (0x[0-9a-f]+)u?, (0x[0-9a-f]+)u? \}", body)]
        first_p, first_m = fp[0]
        hits = [i for i, w in enumerate(unity.words)
                if w & first_m == first_p
                and i + len(fp) <= len(unity.words)
                and all(unity.words[i + j] & m == p for j, (p, m) in enumerate(fp))]
        expect(len(hits) == 1, fp_name + " unique", "hits=%d" % len(hits))
        if hits:
            expect(unity.text_va + hits[0] * 4 == d[rva], rva + " == fingerprint", hex(d[rva]))
        expect(unity.word(d[rva]) == d[guard], guard, hex(d[guard]))

    tm = d["GAME_RVA_TM_UPDATE"]
    expect(unity.word(tm + d["TM_BODY_DELTA"]) & 0xffc003ff == 0xd10003ff, "TM_BODY_DELTA opens a frame")
    # ldr x8,[x0,#frame] / ldr w9,[x0,#render] / ldrb w10,[x0,#pause]
    expect(unity.word(tm) == 0xf9400008 | (d["TM_FIELD_FRAMECOUNT"] // 8) << 10, "TM_FIELD_FRAMECOUNT")
    expect(unity.word(tm + 4) == 0xb9400009 | (d["TM_FIELD_RENDERCOUNT"] // 4) << 10, "TM_FIELD_RENDERCOUNT")
    expect(unity.word(tm + 8) == 0x3940000a | d["TM_FIELD_PAUSE"] << 10, "TM_FIELD_PAUSE")

    run = d["GAME_RVA_FMOD_BUFGEOM"]
    expect(unity.word(run - 8) == d["GUARD_FMOD_PERIOD_LDR"], "GUARD_FMOD_PERIOD_LDR")
    expect(unity.word(run - 4) == d["GUARD_FMOD_PERIOD_CBZ"], "GUARD_FMOD_PERIOD_CBZ")
    expect(unity.word(run + d["FMOD_BUFGEOM_BRANCH_OFF"]) == d["GUARD_FMOD_BUFGEOM_BLS"], "GUARD_FMOD_BUFGEOM_BLS")
    expect(unity.word(run + d["FMOD_BUFCOUNT_OFF"]) == d["GUARD_FMOD_BUFCOUNT"], "GUARD_FMOD_BUFCOUNT")

    expect(unity.word(d["GAME_RVA_WaitVSync"]) == d["GUARD_WaitVSync"], "GUARD_WaitVSync")
    ldr_va = d["GAME_RVA_WaitVSync_ldr"]
    ldr = unity.word(ldr_va)
    expect(ldr == d["GUARD_WaitVSync_ldr"], "GUARD_WaitVSync_ldr")
    adrp = unity.word(ldr_va - 4)
    page = (((adrp >> 29) & 3) | ((adrp >> 5) & 0x7ffff) << 2) << 12
    counter = (ldr_va & ~0xfff) + page + ((ldr >> 10) & 0xfff) * 8
    expect(counter == d["GAME_RVA_VSYNC_COUNTER"], "GAME_RVA_VSYNC_COUNTER decodes", hex(counter))
    expect(unity.in_bss(counter), "vsync counter is in .bss")

    region = open("source/sd2_region_patch.h", errors="replace").read()
    sites = re.findall(r"\{ (0x[0-9a-f]+), (0x[0-9a-f]+), (0x[0-9a-f]+) \}", region)
    expect(len(sites) == 21, "region table has 21 sites", str(len(sites)))
    bad = [o for o, f, _ in sites if unity.word(int(o, 16)) != int(f, 16)]
    expect(not bad, "region table from-words", str(bad))


def check_il2cpp():
    il2cpp = Elf("game/lib/libil2cpp.so")
    stp, str_ = 0xa9bf4ffe, 0xf81e0ffe
    rows = {
        "source/sd2_il2cpp.h": {"Input_GetKey": str_, "Input_GetKeyDown": str_, "Input_GetKeyUp": str_,
                                "Input_GetAxis": 0x14000001, "Input_GetAxisRaw": 0x14000001,
                                "Input_GetButtonDown": 0x14000001, "Backend_RequestAd": 0xa9bc67fe,
                                "Application_get_internetReachability": 0xa9bf4ffe, "Input_get_mouseScrollDelta": 0xd10083ff},
        "source/sd2_touchhook.h": {"Input_GetMouseButton": str_, "Input_GetMouseButtonDown": str_,
                                   "Input_GetMouseButtonUp": str_, "Input_get_mousePosition": 0xd10083ff,
                                   "Input_get_mousePresent": stp, "Input_get_touchSupported": stp,
                                   "Input_get_touchCount": stp},
        "source/sd2_screen.h": {"Screen_get_width": stp, "Screen_get_height": stp,
                                "Display_get_systemWidth": 0xd100c3ff, "Display_get_systemHeight": 0xd100c3ff},
        "source/sd2_time.h": {"Time_get_time": stp, "Time_get_deltaTime": stp, "Time_get_unscaledTime": stp,
                              "Time_get_unscaledDeltaTime": stp, "Time_get_smoothDeltaTime": stp,
                              "Time_get_timeScale": stp, "Time_get_frameCount": stp,
                              "Time_get_realtimeSinceStartup": stp},
    }
    shim = open("source/libc_shim.c", errors="replace").read()
    guards = re.findall(r'\{ (0x[0-9a-f]+), (0x[0-9a-f]+), "(GC_[^"]+)"\s*\}', shim)
    expect(len(guards) == 8, "GC bridge has 8 guards", str(len(guards)))
    for off, word, what in guards:
        expect(il2cpp.word(int(off, 16)) == int(word, 16), "GC guard " + what, off)
    d = defines(shim)
    expect(d["GC_RESTART_SIG_OFF"] - 4 == d["GC_SUSPEND_SIG_OFF"] == d["GC_START_ACK_OFF"] + 4,
           "GC ints are contiguous")
    expect(il2cpp.in_bss(d["GC_ACK_SEM_OFF"]), "GC ack semaphore is in .bss", hex(d["GC_ACK_SEM_OFF"]))

    for path, wanted in rows.items():
        d = defines(open(path, errors="replace").read())
        for name, guard in wanted.items():
            rva = d.get("RVA_" + name)
            expect(rva is not None and il2cpp.word(rva) == guard, "RVA_" + name,
                   "%s word=%s" % (hex(rva or 0), hex(il2cpp.word(rva) or 0) if rva else "-"))


def main():
    check_engine()
    check_il2cpp()
    if failures:
        print("\n%d FAILED: %s" % (len(failures), ", ".join(failures)))
        return 1
    print("\nall checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
