# Builds an aarch64 sysroot from official Debian 12 packages. Files are only extracted, nothing is executed.
# Usage: python -I fetch_sysroot.py <dest> [extra packages...]
import hashlib, io, lzma, os, posixpath, shutil, sys, tarfile, urllib.request

MIRROR = "https://deb.debian.org/debian"
PACKAGES = """
gcc-12-base libc6 libc6-dev libgcc-s1 libgcc-12-dev libstdc++6 libstdc++-12-dev linux-libc-dev libatomic1
libsdl2-2.0-0 libsdl2-dev libfreetype6 libfreetype-dev libpng16-16 libbrotli1 zlib1g zlib1g-dev
libx11-6 libx11-dev libxrandr2 libxrandr-dev libxrender1 libxrender-dev libxext6 libxext-dev x11proto-dev
libxcb1 libxcb1-dev libxau6 libxau-dev libxdmcp6 libxdmcp-dev libbsd0 libmd0
libdbus-1-3 libdbus-1-dev libwayland-client0 libwayland-egl1 libwayland-dev
libegl1 libegl-dev libgles2 libgles-dev libgl1 libgl-dev libglx-dev libglvnd0 libgbm1 libgbm-dev libdrm2 libdrm-dev
""".split()


def fetch(url):
    with urllib.request.urlopen(url, timeout=120) as r:
        return r.read()


def parse_index(text):
    index = {}
    for stanza in text.split("\n\n"):
        fields = {}
        for line in stanza.split("\n"):
            if line and not line[0].isspace() and ":" in line:
                key, value = line.split(":", 1)
                fields[key] = value.strip()
        if "Package" in fields:
            index[fields["Package"]] = fields
    return index


def data_tar(deb):
    if deb[:8] != b"!<arch>\n":
        raise ValueError("not an ar archive")
    pos = 8
    while pos < len(deb):
        name = deb[pos:pos + 16].decode().strip().rstrip("/")
        size = int(deb[pos + 48:pos + 58].decode().strip())
        body = deb[pos + 60:pos + 60 + size]
        if name.startswith("data.tar"):
            return name, body
        pos += 60 + size + (size & 1)
    raise ValueError("no data.tar")


def extract(deb, dest, links):
    name, body = data_tar(deb)
    if name.endswith(".zst"):
        raise ValueError("zstd data.tar not supported")
    with tarfile.open(fileobj=io.BytesIO(body), mode="r:*") as tar:
        for m in tar.getmembers():
            rel = posixpath.normpath(m.name)
            if rel.startswith("..") or rel.startswith("/") or rel.startswith(("usr/share/doc", "usr/share/man", "usr/share/lintian")):
                continue
            out = os.path.join(dest, *rel.split("/"))
            if m.isdir():
                os.makedirs(out, exist_ok=True)
            elif m.isfile():
                os.makedirs(os.path.dirname(out), exist_ok=True)
                with open(out, "wb") as f:
                    shutil.copyfileobj(tar.extractfile(m), f)
            elif m.issym():
                links.append((rel, m.linkname))


def resolve_links(dest, links):
    # Windows without symlink rights: replace each link with a copy of its target.
    pending = links
    for _ in range(8):
        retry = []
        for rel, target in pending:
            if target.startswith("/"):
                src_rel = target.lstrip("/")
            else:
                src_rel = posixpath.normpath(posixpath.join(posixpath.dirname(rel), target))
            src = os.path.join(dest, *src_rel.split("/"))
            out = os.path.join(dest, *rel.split("/"))
            if os.path.isfile(src):
                os.makedirs(os.path.dirname(out), exist_ok=True)
                shutil.copyfile(src, out)
            elif os.path.isdir(src):
                if not os.path.exists(out):
                    shutil.copytree(src, out)
            else:
                retry.append((rel, target))
        pending = retry
        if not pending:
            break
    return pending


def main():
    dest = sys.argv[1]
    wanted = PACKAGES + sys.argv[2:]
    cache = dest + "-debs"
    os.makedirs(cache, exist_ok=True)
    os.makedirs(dest, exist_ok=True)
    index = {}
    for suite, comp in (("bookworm", "main"),):
        raw = fetch(f"{MIRROR}/dists/{suite}/{comp}/binary-arm64/Packages.xz")
        index.update(parse_index(lzma.decompress(raw).decode("utf-8", "replace")))
    links = []
    total = 0
    for pkg in wanted:
        info = index.get(pkg)
        if info is None:
            print("MISSING", pkg)
            continue
        path = os.path.join(cache, os.path.basename(info["Filename"]))
        if os.path.exists(path):
            with open(path, "rb") as f:
                deb = f.read()
        else:
            deb = fetch(f"{MIRROR}/{info['Filename']}")
        if hashlib.sha256(deb).hexdigest() != info["SHA256"]:
            raise SystemExit(f"SHA256 mismatch: {pkg}")
        with open(path, "wb") as f:
            f.write(deb)
        total += len(deb)
        extract(deb, dest, links)
        print(f"ok {pkg} {info['Version']} {len(deb) // 1024} KiB")
    left = resolve_links(dest, links)
    print(f"total {total // (1024 * 1024)} MiB, {len(links)} links, unresolved {len(left)}")
    for rel, target in left:
        print("  unresolved", rel, "->", target)


main()
