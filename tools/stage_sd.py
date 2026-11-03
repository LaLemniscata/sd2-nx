#!/usr/bin/env python3
"""Build the SD card folder from your own Google Play copy of the game.

Usage: tools/stage_sd.py [output_dir]    (run from the repository root)

Reads game/apk/*.apk and sd2_nx.nro. Default output: game/sd/switch/sd2_nx.
Stops if the APKs are not the supported version. Copy the output to
sdmc:/switch/sd2_nx/ on the console.
"""
import hashlib
import os
import shutil
import sys
import zipfile

SUPPORTED = {
    "game/apk/base.apk":
        "4f79947311560099b70cee6fab95cd9f08d6c6014fae4f822b33155bda25f0dc",
    "game/apk/split_config.arm64_v8a.apk":
        "bc3ece4fd9fb6445942ede7ade0a036f36174cf9a8ae32197acc776a2b7dd151",
}
LIBS = ("libmain.so", "libunity.so", "libil2cpp.so")
ASSET_PREFIXES = ("assets/bin/Data/", "assets/not_atlassed", "assets/audio",
                  "assets/StreamingAssets", "assets/UnityServicesProjectConfiguration.json")


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "game/sd/switch/sd2_nx"
    for path, want in SUPPORTED.items():
        if not os.path.exists(path):
            print("missing", path)
            return 1
        if sha256(path) != want:
            print("%s is not Soda Dungeon 2 1.2.21 (140). Unsupported version." % path)
            return 1
    if not os.path.exists("sd2_nx.nro"):
        print("missing sd2_nx.nro: run make first")
        return 1

    os.makedirs(out, exist_ok=True)
    shutil.copy2("sd2_nx.nro", out)
    split = zipfile.ZipFile("game/apk/split_config.arm64_v8a.apk")
    for lib in LIBS:
        with open(os.path.join(out, lib), "wb") as f:
            f.write(split.read("lib/arm64-v8a/" + lib))

    base = zipfile.ZipFile("game/apk/base.apk")
    count = total = 0
    for info in base.infolist():
        if info.is_dir() or not info.filename.startswith(ASSET_PREFIXES):
            continue
        dest = os.path.join(out, info.filename)
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        if not (os.path.exists(dest) and os.path.getsize(dest) == info.file_size):
            with base.open(info) as src, open(dest, "wb") as dst:
                shutil.copyfileobj(src, dst)
        count += 1
        total += info.file_size
    print("staged %s: %d asset files, %d MB" % (out, count, total // (1 << 20)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
