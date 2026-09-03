#!/usr/bin/env python3
"""Give packaged client downloads explicit BKW version/platform names."""
import pathlib
import re
import sys

root = pathlib.Path(__file__).resolve().parents[1]
version = re.search(r'#define BKW_VERSION "([br][0-9.]+)"', (root / "src/engine/shared/bkw_version.h").read_text())[1]
folder = pathlib.Path(sys.argv[1])
for file in list(folder.iterdir()):
    if file.name.endswith(".tar.xz"):
        suffix = "linux_x86_64.tar.xz"
    elif file.suffix == ".zip":
        suffix = "win64.zip"
    elif file.suffix == ".dmg":
        suffix = "macos.dmg"
    elif file.suffix == ".apk":
        suffix = "android.apk"
    else:
        continue
    file.rename(folder / f"bkw-{version}-{suffix}")
