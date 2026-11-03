#!/usr/bin/env python3
"""Rewrite the RVA_<Class>_<method> defines in source/ from an Il2CppDumper dump.

Usage: tools/gen_il2cpp_rvas.py [--check]    (run from the repository root)

Reads dumps/il2cpp/dump.cs, which is generated from your own copy of the game.
Exits non-zero if a define has no matching method, so a stale address cannot
survive silently. --check reports without writing.
"""
import re
import sys

HEADERS = ["source/sd2_il2cpp.h", "source/sd2_time.h", "source/sd2_screen.h",
           "source/sd2_touchhook.h"]
CLASSES = ("Input", "Screen", "Display", "Time", "Backend", "Application")


def read_methods(path):
    lines = open(path, errors="replace").read().split("\n")
    table = {}
    for cls in CLASSES:
        start = next(i for i, line in enumerate(lines)
                     if re.match(r"public (sealed |static )?class %s (:.*)?// TypeDefIndex" % cls, line))
        end = start
        while not lines[end].startswith("}"):
            end += 1
        for i in range(start, end):
            m = re.search(r"RVA: (0x[0-9A-F]+)", lines[i])
            if not m:
                continue
            sig = lines[i + 1].strip()
            name = sig.split("(")[0].split()[-1]
            table.setdefault((cls, name), []).append((int(m.group(1), 16), sig))
    return table


def main():
    check = "--check" in sys.argv
    table = read_methods("dumps/il2cpp/dump.cs")
    pattern = re.compile(r"^(#define RVA_(%s)_(\w+)\s+)0x[0-9a-fA-F]+u" % "|".join(CLASSES), re.M)
    missing = []
    for path in HEADERS:
        text = open(path, newline="").read()

        def replace(m):
            found = table.get((m.group(2), m.group(3)))
            if not found:
                missing.append("%s_%s" % (m.group(2), m.group(3)))
                return m.group(0)
            if len(found) > 1:
                print("ambiguous %s_%s: %s" % (m.group(2), m.group(3), [s for _, s in found]))
            print("%-44s 0x%07x  %s" % (m.group(2) + "." + m.group(3), found[0][0], found[0][1]))
            return "%s0x%07xu" % (m.group(1), found[0][0])

        new = pattern.sub(replace, text)
        if not check and new != text:
            open(path, "w", newline="").write(new)
    if missing:
        print("NOT IN THIS BUILD:", ", ".join(sorted(set(missing))))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
