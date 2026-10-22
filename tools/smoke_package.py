#!/usr/bin/env python3
"""Launch smoke for a staged or extracted TooieRecompiled package.

Port of tools/smoke_package.sh. Passes when the process is still alive after
SECS seconds (default 15), then signals only that PID (graceful, then kill).

On Linux the process gets an empty environment plus a temporary HOME/XDG tree
(and optional TOOIE_SMOKE_LD_LIBRARY_PATH). On Windows, SHGetKnownFolderPath
ignores environment variables, so config lands in the real
%LOCALAPPDATA%\\TooieRecompiled.

  python tools/smoke_package.py <package dir> [secs]
"""
from __future__ import annotations

import os
import subprocess
import sys
import tempfile
import time


def terminate_only(proc: subprocess.Popen) -> None:
    """Signal this PID only: graceful first, then a hard kill."""
    if proc.poll() is not None:
        return
    if os.name == "nt":
        subprocess.run(
            ["taskkill", "/PID", str(proc.pid)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
    else:
        try:
            proc.terminate()
        except OSError:
            return
    try:
        proc.wait(timeout=10)
        return
    except subprocess.TimeoutExpired:
        pass
    if proc.poll() is not None:
        return
    if os.name == "nt":
        subprocess.run(
            ["taskkill", "/F", "/PID", str(proc.pid)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
    try:
        proc.kill()
    except OSError:
        pass
    try:
        proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        pass


def linux_env(home: str, cfg: str, data: str, cache: str) -> dict:
    env = {
        "PATH": os.environ.get("PATH", ""),
        "DISPLAY": os.environ.get("DISPLAY", ""),
        "XAUTHORITY": os.environ.get("XAUTHORITY", ""),
        "WAYLAND_DISPLAY": os.environ.get("WAYLAND_DISPLAY", ""),
        "XDG_RUNTIME_DIR": os.environ.get("XDG_RUNTIME_DIR", ""),
        "HOME": home,
        "XDG_CONFIG_HOME": cfg,
        "XDG_DATA_HOME": data,
        "XDG_CACHE_HOME": cache,
    }
    library_path = os.environ.get("TOOIE_SMOKE_LD_LIBRARY_PATH")
    if library_path:
        env["LD_LIBRARY_PATH"] = library_path
    return env


def print_mapped_libz(pid: int) -> None:
    libs = []
    try:
        with open("/proc/%d/maps" % pid, "r", encoding="utf-8", errors="replace") as handle:
            for line in handle:
                if "libz.so" not in line:
                    continue
                parts = line.split()
                if len(parts) >= 6 and parts[5] not in libs:
                    libs.append(parts[5])
    except OSError:
        pass
    print("mapped libz: " + " ".join(libs), flush=True)


def linux_cmdline(pid: int) -> str:
    with open("/proc/%d/cmdline" % pid, "rb") as handle:
        return handle.read().replace(b"\x00", b" ").decode("utf-8", "replace")


def print_isolated_files(paths: tuple) -> None:
    print("files created in isolated cfg/data:", flush=True)
    shown = 0
    for base in paths:
        for dirpath, _dirnames, filenames in os.walk(base):
            for name in filenames:
                print(os.path.join(dirpath, name), flush=True)
                shown += 1
                if shown >= 20:
                    return


def tail_file(path: str, count: int = 5) -> None:
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            lines = handle.read().splitlines()
    except OSError:
        return
    for line in lines[-count:]:
        print(line, flush=True)


def main(argv: list) -> int:
    # argv is the argument list without the program name (sys.argv[1:]).
    if len(argv) < 1 or len(argv) > 2:
        sys.stderr.write("usage: tools/smoke_package.py <package dir> [secs]\n")
        return 2
    pkg = os.path.abspath(argv[0])
    if not os.path.isdir(pkg):
        print("SMOKE FAIL: not a directory: %s" % pkg, flush=True)
        return 1
    secs = 15
    if len(argv) == 2:
        try:
            secs = int(argv[1])
        except ValueError:
            sys.stderr.write("SMOKE FAIL: secs must be an integer\n")
            return 2
        if secs < 0:
            sys.stderr.write("SMOKE FAIL: secs must be >= 0\n")
            return 2
    exe_name = "TooieRecompiled.exe" if os.name == "nt" else "TooieRecompiled"
    exe = os.path.join(pkg, exe_name)
    if not os.path.isfile(exe):
        print("SMOKE FAIL: missing %s" % exe, flush=True)
        return 1

    tmp = tempfile.mkdtemp(prefix="tooie-smoke-")
    stdout_path = os.path.join(tmp, "stdout.log")
    stderr_path = os.path.join(tmp, "stderr.log")
    env = None
    isolated = ()
    if os.name == "nt":
        print("NOTE: not isolated on Windows", flush=True)
        print(
            "NOTE: config is stored under the real %LOCALAPPDATA%\\TooieRecompiled "
            "(SHGetKnownFolderPath ignores environment variables).",
            flush=True,
        )
    else:
        home = os.path.join(tmp, "home")
        cfg = os.path.join(tmp, "cfg")
        data = os.path.join(tmp, "data")
        cache = os.path.join(tmp, "cache")
        for directory in (home, cfg, data, cache):
            os.makedirs(directory)
        env = linux_env(home, cfg, data, cache)
        isolated = (cfg, data, home)

    print("logs: %s %s" % (stdout_path, stderr_path), flush=True)
    out_handle = open(stdout_path, "wb")
    err_handle = open(stderr_path, "wb")
    proc = None
    allow_kill = True
    try:
        proc = subprocess.Popen([exe], cwd=pkg, env=env, stdout=out_handle, stderr=err_handle)
        time.sleep(secs)
        if proc.poll() is not None:
            print(
                "SMOKE FAIL: exited early rc=%s (logs: %s %s)"
                % (proc.returncode, stdout_path, stderr_path),
                flush=True,
            )
            tail_file(stderr_path)
            return 1
        if os.name != "nt":
            print_mapped_libz(proc.pid)
            command = linux_cmdline(proc.pid)
            if not command.startswith(exe):
                allow_kill = False
                print(
                    "SMOKE: refusing to signal unexpected pid %s (%s)"
                    % (proc.pid, command.strip()),
                    flush=True,
                )
                return 2
        terminate_only(proc)
        print(
            "SMOKE PASS: alive %ss, terminated own pid %s (logs: %s %s)"
            % (secs, proc.pid, stdout_path, stderr_path),
            flush=True,
        )
        if isolated:
            print_isolated_files(isolated)
        return 0
    finally:
        out_handle.close()
        err_handle.close()
        if proc is not None and allow_kill and proc.poll() is None:
            terminate_only(proc)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
