# Minimal pkg-config for the handheld cross build (Windows or Linux host): reads .pc files from the sysroot. Only what CMake's FindPkgConfig calls.
import os
import re
import sys

SYSROOT = (os.environ.get("HANDHELD_SYSROOT") or os.environ["XU10_SYSROOT"]).replace("\\", "/")
DIRS = ["usr/lib/aarch64-linux-gnu/pkgconfig", "usr/lib/pkgconfig", "usr/share/pkgconfig"]


def load(name):
    for d in DIRS:
        path = os.path.join(SYSROOT, d, name + ".pc")
        if os.path.exists(path):
            variables, fields = {}, {}
            for line in open(path, encoding="utf-8", errors="replace"):
                line = line.strip()
                m = re.match(r"^([A-Za-z0-9_.]+)=(.*)$", line)
                if m:
                    variables[m.group(1)] = expand(m.group(2), variables)
                    continue
                m = re.match(r"^([A-Za-z.]+):\s*(.*)$", line)
                if m:
                    fields[m.group(1)] = expand(m.group(2), variables)
            return fields
    return None


def expand(text, variables):
    return re.sub(r"\$\{([^}]+)\}", lambda m: variables.get(m.group(1), ""), text)


def collect(name, key, seen):
    if name in seen:
        return []
    seen.add(name)
    pc = load(name)
    if pc is None:
        return []
    out = pc.get(key, "").split()
    for dep in re.split(r"[,\s]+", pc.get("Requires", "")):
        if dep and not re.match(r"^[<>=0-9.]+$", dep):
            out += collect(dep, key, seen)
    return out


def sysrooted(flag):
    for prefix in ("-I", "-L"):
        if flag.startswith(prefix + "/"):
            return prefix + SYSROOT + flag[2:]
    return flag


def main():
    args = sys.argv[1:]
    if "--version" in args:
        print("0.29.2")
        return 0
    opts = [a for a in args if a.startswith("--")]
    names = [a for a in args if not a.startswith("-") and not re.match(r"^[<>=0-9.]+$", a)]
    missing = [n for n in names if load(n) is None]
    if missing:
        print("Package " + missing[0] + " was not found", file=sys.stderr)
        return 1
    out = []
    for name in names:
        if "--modversion" in opts:
            print(load(name).get("Version", "0"))
            continue
        cflags = [sysrooted(f) for f in collect(name, "Cflags", set())]
        libs = [sysrooted(f) for f in collect(name, "Libs", set())]
        if "--cflags-only-I" in opts:
            out += [f for f in cflags if f.startswith("-I")]
        if "--cflags-only-other" in opts:
            out += [f for f in cflags if not f.startswith("-I")]
        if "--libs-only-L" in opts:
            out += [f for f in libs if f.startswith("-L")]
        if "--libs-only-l" in opts:
            out += [f for f in libs if f.startswith("-l")]
        if "--libs-only-other" in opts:
            out += [f for f in libs if not f.startswith(("-l", "-L"))]
    if out:
        print(" ".join(out))
    return 0


sys.exit(main())
