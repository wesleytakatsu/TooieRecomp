#!/usr/bin/env python3
"""Build local .deb/AppImage installers from the existing audited Linux package."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import sys

ROOT = Path(__file__).resolve().parent.parent
APP = "tooie-recomp"
TOOL_SHA = "ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0"
RUNTIME_SHA = "617aa77314d57c8b8e4fae9c3f0b35af7b30d5529cb69e98f64f92510ed98a46"
DESKTOP = """[Desktop Entry]
Type=Application
Name=TooieRecompiled
Comment=Banjo-Tooie native recompilation (bring your own ROM)
Exec=tooie-recomp
Icon=tooie-recomp
Terminal=false
Categories=Game;
"""


def run(*args, **kwargs):
    return subprocess.run([str(a) for a in args], check=True, **kwargs)


def output(*args, **kwargs):
    return run(*args, capture_output=True, text=True, **kwargs).stdout.strip()


def sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def write(path, text, mode=0o644):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
    path.chmod(mode)


def checksum(path):
    write(Path(str(path) + ".sha256"), f"{sha(path)}  {path.name}\n")
    print(f"installer: {path}", flush=True)


def appimage_tools():
    cache = Path.home() / ".cache/tooie-appimage-tools"
    return [(Path(os.environ.get("TOOIE_APPIMAGETOOL", cache / "appimagetool-x86_64.AppImage")).resolve(), TOOL_SHA),
            (Path(os.environ.get("TOOIE_APPIMAGE_RUNTIME", cache / "runtime-x86_64")).resolve(), RUNTIME_SHA)]


def check_requirements(format):
    """Read-only checks, limited to the formats requested; report all missing inputs."""
    missing = []
    commands = "git tar gzip sha256sum readelf ldd dpkg dpkg-query strings find sort xargs install cp awk sed grep cut head wc du stat readlink basename dirname mkdir rm chmod tee mktemp date clang cmake".split()
    if format in ["all", "deb", "appimage"]:
        commands += ["desktop-file-validate"]
    if format in ["all", "deb"]:
        commands += ["dpkg-shlibdeps", "dpkg-deb"]
    if format in ["all", "appimage"]:
        commands += ["glib-compile-schemas"]
    for command in commands:
        if not shutil.which(command):
            missing.append(f"{command} (veja os pacotes apt em tools/BUILDING.md)")
    if shutil.which("cmake"):
        version = re.search(r"cmake version (\d+)\.(\d+)", output("cmake", "--version"))
        if not version or tuple(map(int, version.groups())) < (3, 21):
            missing.append("CMake >= 3.21 necessário (presets da branch)")
    if not any(shutil.which(c) for c in ["ldconfig", "/sbin/ldconfig", "/usr/sbin/ldconfig"]):
        missing.append("ldconfig (apt: libc-bin)")
    if sys.platform != "linux" or (shutil.which("dpkg") and output("dpkg", "--print-architecture") != "amd64"):
        missing.append("host Linux x86_64/amd64 necessário")
    release = os.environ.get("TOOIE_RELEASE_VERSION", "")
    if release and (not re.fullmatch(r"[A-Za-z0-9.-]+", release) or release.startswith(("v", "V"))):
        missing.append("TOOIE_RELEASE_VERSION inválida: use letras, números, pontos e hífens, sem v inicial")
    elif release and format in ["all", "deb"] and shutil.which("dpkg"):
        if subprocess.run(["dpkg", "--validate-version", release], capture_output=True).returncode:
            missing.append("TOOIE_RELEASE_VERSION não é uma versão Debian válida (exemplo: 0.1.0)")
    if format in ["all", "appimage"]:
        for path, expected in appimage_tools():
            if not path.is_file() or sha(path) != expected:
                missing.append(f"{path}: ausente ou SHA-256 incorreto; download explícito em tools/BUILDING.md")
        tool = appimage_tools()[0][0]
        if tool.is_file() and not os.access(tool, os.X_OK):
            missing.append(f"permissão de execução: chmod +x '{tool}'")
        if not list(Path("/usr/share/glib-2.0/schemas").glob("org.gtk.Settings.*.gschema.xml")):
            missing.append("schemas GTK 3 (apt: libgtk-3-common)")
        for path in [Path("/usr/share/doc/libgtk-3-common/copyright"), ROOT / "tools/appimage-runtime.LICENSE"]:
            if not path.is_file():
                missing.append(f"aviso de licença ausente: {path}")
        if not Path("/usr/share/common-licenses").is_dir():
            missing.append("/usr/share/common-licenses (apt: base-files)")
    for var, default in [("TOOIE_BUILD_DIR", "build-linux"), ("TOOIE_DIST_DIR", "dist")]:
        path = Path(os.environ.get(var, ROOT / default)).resolve()
        while not path.exists():
            path = path.parent
        if not path.is_dir() or not os.access(path, os.W_OK | os.X_OK):
            missing.append(f"{var}: destino não gravável ({path})")
        elif shutil.disk_usage(path).free < 2 * 1024**3:
            missing.append(f"{var}: menos de 2 GiB livres em {path}")
    for item in missing:
        print(f"MISS/BAD {item}", file=sys.stderr)
    if missing:
        print("Pacotes usuais: sudo apt install binutils libc-bin coreutils findutils gawk grep sed tar gzip "
              "dpkg-dev desktop-file-utils libglib2.0-bin", file=sys.stderr)
    else:
        print(f"PACKAGE PREFLIGHT OK ({format})", flush=True)
    return not missing


def desktop_files(tree):
    desktop = tree / f"usr/share/applications/{APP}.desktop"
    write(desktop, DESKTOP)
    run("desktop-file-validate", desktop)
    icon = tree / f"usr/share/icons/hicolor/256x256/apps/{APP}.png"
    icon.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(ROOT / "icons/app.png", icon)


def deb(stage, work, out, version):
    tree = work / "deb"
    shutil.copytree(stage, tree / f"usr/lib/{APP}")
    desktop_files(tree)
    (tree / "usr/bin").mkdir(parents=True)
    (tree / f"usr/bin/{APP}").symlink_to(f"../lib/{APP}/run-tooie.sh")
    # dpkg-shlibdeps uses the host's distro symbol database for minimum versions.
    write(work / "debian/control", f"Source: {APP}\n\nPackage: {APP}\nArchitecture: amd64\n")
    deps = output("dpkg-shlibdeps", "-O", "-l/usr/lib/x86_64-linux-gnu",
                  f"-e{stage / 'TooieRecompiled'}", cwd=work)
    if not deps.startswith("shlibs:Depends="):
        raise RuntimeError(f"unexpected dpkg-shlibdeps output: {deps}")
    deps = deps[len("shlibs:Depends="):] + ", libvulkan1, bash"
    size = sum(p.stat().st_size for p in tree.rglob("*") if p.is_file()) // 1024 + 1
    write(tree / "DEBIAN/control", f"""Package: {APP}
Version: {version}
Section: games
Priority: optional
Architecture: amd64
Maintainer: Wesley <wesley_takatsu@yahoo.com.br>
Installed-Size: {size}
Depends: {deps}
Homepage: https://github.com/wesleytakatsu/TooieRecomp
Description: TooieRecompiled native recompilation
 Native Banjo-Tooie recompilation. Requires your own verified USA ROM and
 a Vulkan- or OpenGL-capable GPU/driver. No ROM, saves or user configuration
 included.
 Linked-code notices and asset provenance are in /usr/lib/{APP}/licenses
 and THIRD-PARTY-MANIFEST.tsv.
""")
    target = out / f"{stage.parent.name}.deb"
    run("dpkg-deb", "--root-owner-group", "--build", tree, target)
    # Check the real archive, including its symlink and payload checksums.
    extracted = work / "deb-check"
    run("dpkg-deb", "--extract", target, extracted)
    run(ROOT / "tools/verify_package.sh", extracted / f"usr/lib/{APP}")
    if (extracted / f"usr/bin/{APP}").resolve() != extracted / f"usr/lib/{APP}/run-tooie.sh":
        raise RuntimeError("broken Debian launcher symlink")
    checksum(target)


def appimage(stage, work, out, name):
    pinned_tools = appimage_tools()
    tool, runtime = [item[0] for item in pinned_tools]
    for path, expected in pinned_tools:
        if not path.is_file() or sha(path) != expected:
            raise RuntimeError(f"missing or unverified AppImage tool: {path}; see tools/BUILDING.md")
    tree = work / "TooieRecompiled.AppDir"
    shutil.copytree(stage, tree / f"usr/lib/{APP}")
    desktop_files(tree)
    (tree / f"{APP}.desktop").symlink_to(f"usr/share/applications/{APP}.desktop")
    (tree / f"{APP}.png").symlink_to(f"usr/share/icons/hicolor/256x256/apps/{APP}.png")
    (tree / ".DirIcon").symlink_to(f"{APP}.png")
    (tree / "usr/bin").mkdir(parents=True)
    (tree / f"usr/bin/{APP}").symlink_to("../../AppRun")
    write(tree / "AppRun", """#!/bin/sh
set -eu
APPDIR=$(CDPATH= cd -- "$(dirname -- "$(readlink -f -- "$0")")" && pwd)
export LD_LIBRARY_PATH="$APPDIR/usr/lib/tooie-runtime${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export GSETTINGS_SCHEMA_DIR="$APPDIR/usr/share/glib-2.0/schemas"
cd "$APPDIR/usr/lib/tooie-recomp"
exec ./TooieRecompiled "$@"
""", 0o755)
    libs = tree / "usr/lib/tooie-runtime"
    libs.mkdir(parents=True)
    notices = tree / "usr/share/doc/tooie-runtime"
    notices.mkdir(parents=True)
    rows = ["soname\tsha256\tpackage\tversion\tdistro_path\tpolicy"]
    # Leave glibc and graphics-driver interfaces to the host. Everything else
    # comes from the distro-owned dependency closure, never /usr/local shadows.
    host = re.compile(r"^(ld-linux|lib(c|m|pthread|dl|rt|resolv|util|nss_[^.]+)\.so|lib(GL|EGL|GLX|GLdispatch|OpenGL|vulkan|drm|gbm)[^/]*\.so)")
    for line in (stage / "DEPENDENCIES.txt").read_text().splitlines():
        if line.startswith("#") or not line.strip():
            continue
        soname, package, version, source, *_ = line.split("\t")
        path = Path(source)
        policy = "host" if host.match(soname) else "bundled"
        if policy == "bundled":
            if package == "UNOWNED" or not path.is_file():
                raise RuntimeError(f"cannot bundle unowned/missing library: {line}")
            shutil.copyfile(path, libs / soname)
            copyright_file = Path("/usr/share/doc") / package / "copyright"
            if not copyright_file.is_file():
                raise RuntimeError(f"missing copyright notice for {package}")
            shutil.copyfile(copyright_file, notices / f"{package}.copyright")
        rows.append(f"{soname}\t{sha(path)}\t{package}\t{version}\t{source}\t{policy}")
    schemas = tree / "usr/share/glib-2.0/schemas"
    schemas.mkdir(parents=True)
    for schema in Path("/usr/share/glib-2.0/schemas").glob("org.gtk.Settings.*.gschema.xml"):
        shutil.copyfile(schema, schemas / schema.name)
    if not list(schemas.glob("*.xml")):
        raise RuntimeError("GTK 3 schemas missing; install libgtk-3-common")
    run("glib-compile-schemas", "--strict", schemas)
    shutil.copyfile("/usr/share/doc/libgtk-3-common/copyright", notices / "libgtk-3-common.copyright")
    shutil.copytree("/usr/share/common-licenses", tree / "usr/share/common-licenses")
    shutil.copyfile(ROOT / "tools/appimage-runtime.LICENSE", notices / "AppImage-runtime.LICENSE")
    write(tree / "BUNDLED-DEPENDENCIES.tsv", "\n".join(rows) + "\n")
    write(tree / "APPIMAGE-TOOLS.txt", f"appimagetool 1.9.1 sha256 {TOOL_SHA}\n"
          f"type2-runtime fedfad51acc2860b2916a1eef32ba4a500ff31ef sha256 {RUNTIME_SHA}\n"
          "https://github.com/AppImage/appimagetool\nhttps://github.com/AppImage/type2-runtime\n"
          "Runtime/third-party notices: usr/share/doc/tooie-runtime/AppImage-runtime.LICENSE.\n"
          "Requires host glibc and a working Vulkan driver. Not a universal Linux binary.\n")
    write(tree / "MANIFEST.sha256", "".join(
        f"{sha(p)}  {p.relative_to(tree)}\n" for p in sorted(tree.rglob("*"))
        if p.is_file() and not p.is_symlink() and p != tree / "MANIFEST.sha256"))
    target = out / f"{name}.AppImage"
    run(tool, "--appimage-extract-and-run", "--no-appstream", "--runtime-file", runtime,
        "--mksquashfs-opt", "-processors", "--mksquashfs-opt", "2", tree, target,
        env={**os.environ, "ARCH": "x86_64"})
    # Verify the compressed artifact, rather than only its input directory.
    check = work / "appimage-check"
    check.mkdir()
    run(target, "--appimage-extract", cwd=check, stdout=subprocess.DEVNULL)
    extracted = check / "squashfs-root"
    run("sha256sum", "--check", "--quiet", "MANIFEST.sha256", cwd=extracted)
    run(ROOT / "tools/verify_package.sh", extracted / f"usr/lib/{APP}")
    checksum(target)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("format", choices=["all", "tar", "deb", "appimage"], default="all", nargs="?")
    parser.add_argument("--check", action="store_true", help="check packaging prerequisites without writing/building")
    args = parser.parse_args()
    if not check_requirements(args.format):
        raise SystemExit(1)
    if args.check:
        return
    out = Path(os.environ.get("TOOIE_DIST_DIR", ROOT / "dist")).resolve()
    out.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".linux-installers-", dir=out) as temp:
        work = Path(temp)
        pointer = work / "stage-path.txt"
        run(ROOT / "tools/package_linux.sh", env={**os.environ, "TOOIE_DIST_DIR": str(out),
                                                 "TOOIE_PACKAGE_STAGE_ONLY": "0" if args.format in ["all", "tar"] else "1",
                                                 "TOOIE_PACKAGE_STAGE_FILE": str(pointer)})
        stage = Path(pointer.read_text().strip())
        name = stage.parent.name
        run(ROOT / "tools/verify_package.sh", stage)
        if args.format in ["all", "tar"]:
            run(ROOT / "tools/verify_package.sh", out / f"{name}.tar.gz")
        if args.format == "tar":
            return
        release = os.environ.get("TOOIE_RELEASE_VERSION")
        date = output("git", "show", "-s", "--format=%cd", "--date=format:%Y%m%d", "HEAD", cwd=ROOT)
        commit = output("git", "rev-parse", "--short", "HEAD", cwd=ROOT)
        version = release or f"0~git{date}.{commit}+local"
        if args.format in ["all", "deb"]:
            deb(stage, work, out, version)
        if args.format in ["all", "appimage"]:
            appimage(stage, work, out, name)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"package: {error}", file=sys.stderr)
        raise SystemExit(1)
