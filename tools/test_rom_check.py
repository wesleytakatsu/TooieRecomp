#!/usr/bin/env python3
"""Unit tests for tools/rom_check.py, tools/toolchain_lock.py and tools/tooie_paths.py.

Run: python tools/test_rom_check.py -v
"""
from __future__ import annotations

import contextlib
import hashlib
import io
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import rom_check  # noqa: E402
import toolchain_lock  # noqa: E402
import tooie_paths  # noqa: E402
from tooie_paths import BUILD, TOOIE, rom_path  # noqa: E402

FAKE_DATA = b"0123456789abcdef"
FAKE_OTHER = b"fedcba9876543210"


def _digest(data: bytes, name: str) -> str:
    return hashlib.new(name, data).hexdigest()


def _fake_lock() -> dict[str, str]:
    return {
        "ROM_BASENAME": "fake.z64",
        "ROM_SIZE": str(len(FAKE_DATA)),
        "ROM_MD5": _digest(FAKE_DATA, "md5"),
        "ROM_SHA1": _digest(FAKE_DATA, "sha1"),
        "DECOMPRESSED_ROM_SIZE": str(len(FAKE_DATA)),
        "DECOMPRESSED_ROM_SHA1": _digest(FAKE_DATA, "sha1"),
    }


def _run(argv: list[str]) -> tuple[int, str, str]:
    out, err = io.StringIO(), io.StringIO()
    with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
        code = rom_check.main(argv)
    return code, out.getvalue(), err.getvalue()


@contextlib.contextmanager
def _env(name: str, value: str | None):
    saved = os.environ.get(name)
    if value is None:
        os.environ.pop(name, None)
    else:
        os.environ[name] = value
    try:
        yield
    finally:
        if saved is None:
            os.environ.pop(name, None)
        else:
            os.environ[name] = saved


class ToolchainLockTests(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)

    def _lock(self, text: str) -> Path:
        path = Path(self._tmp.name) / "synthetic.lock"
        path.write_text(text, encoding="utf-8")
        return path

    def _load_value(self, line: str) -> str:
        return toolchain_lock.load(self._lock(line + "\n"))["K"]

    def test_real_lock_parses(self):
        lock = toolchain_lock.load()
        self.assertEqual(lock["ROM_BASENAME"], "Banjo-Tooie (USA).z64")
        self.assertEqual(lock["ROM_SIZE"], "33554432")
        self.assertEqual(lock["ROM_MD5"], "40e98faa24ac3ebe1d25cb5e5ddf49e4")
        self.assertEqual(lock["ROM_SHA1"], "af1a89e12b638b8d82cc4c085c8e01d4cba03fb3")
        self.assertEqual(lock["DECOMPRESSED_ROM_SIZE"], "35098224")
        self.assertEqual(lock["DECOMPRESSED_ROM_SHA1"], "af479771ff5e91ce3732f3d84146614ab71d8d79")

    def test_real_lock_trailing_comment_stripped(self):
        self.assertNotIn("#", toolchain_lock.load()["ROM_BASENAME"])

    def test_double_quoted_value_keeps_spaces(self):
        self.assertEqual(self._load_value('K="a b c"'), "a b c")

    def test_single_quoted_value_keeps_backslash(self):
        self.assertEqual(self._load_value(r"K='a\b'"), r"a\b")

    def test_double_quoted_backslash_escapes(self):
        self.assertEqual(self._load_value(r'K="a\"b"'), 'a"b')

    def test_bare_value(self):
        self.assertEqual(self._load_value("K=val"), "val")

    def test_empty_value(self):
        self.assertEqual(self._load_value("K="), "")

    def test_bare_value_trailing_comment(self):
        self.assertEqual(self._load_value("K=val # c"), "val")

    def test_quoted_value_trailing_comment(self):
        self.assertEqual(self._load_value('K="val" # c'), "val")

    def test_hash_glued_to_token_is_kept(self):
        self.assertEqual(self._load_value("K=val#notcomment"), "val#notcomment")

    def test_blank_and_comment_lines_skipped(self):
        lock = toolchain_lock.load(self._lock("# header\n\n   \n  # indented comment\nK=1\n"))
        self.assertEqual(lock, {"K": "1"})

    def test_leading_whitespace_on_entry_stripped(self):
        self.assertEqual(self._load_value("   K=1"), "1")

    def test_duplicate_key_last_wins(self):
        lock = toolchain_lock.load(self._lock("K=1\nK=2\n"))
        self.assertEqual(lock["K"], "2")

    def test_malformed_line_reports_line_number(self):
        path = self._lock("A=1\n\nNOEQUALS\n")
        with self.assertRaises(ValueError) as ctx:
            toolchain_lock.load(path)
        self.assertIn("toolchain.lock:3:", str(ctx.exception))
        self.assertIn("malformed line", str(ctx.exception))

    def test_lowercase_key_is_malformed(self):
        with self.assertRaisesRegex(ValueError, "toolchain.lock:1: malformed line"):
            toolchain_lock.load(self._lock("k=1\n"))

    def test_unclosed_quote_raises(self):
        with self.assertRaisesRegex(ValueError, "toolchain.lock:1: unclosed quote"):
            toolchain_lock.load(self._lock('K="abc\n'))

    def test_trailing_junk_after_quoted_raises(self):
        with self.assertRaisesRegex(ValueError, "trailing junk"):
            toolchain_lock.load(self._lock('K="abc" x\n'))

    def test_unquoted_inner_whitespace_raises(self):
        with self.assertRaisesRegex(ValueError, "malformed unquoted value"):
            toolchain_lock.load(self._lock("K=a b\n"))


class RomCheckSyntheticTests(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.dir = Path(self._tmp.name)
        patcher = mock.patch.object(rom_check, "load", return_value=_fake_lock())
        patcher.start()
        self.addCleanup(patcher.stop)

    def _write(self, name: str, data: bytes) -> str:
        path = self.dir / name
        path.write_bytes(data)
        return str(path)

    def test_raw_missing_file(self):
        code, _, err = _run([str(self.dir / "nope.z64")])
        self.assertEqual(code, 1)
        self.assertIn("rom_check: FAIL: not a file", err)

    def test_raw_wrong_size(self):
        code, _, err = _run([self._write("short.z64", FAKE_DATA[:-1])])
        self.assertEqual(code, 1)
        self.assertIn("rom_check: FAIL: size 15 != 16", err)

    def test_raw_ok(self):
        code, out, err = _run([self._write("good.z64", FAKE_DATA)])
        self.assertEqual(code, 0)
        self.assertEqual(err, "")
        self.assertIn(f"rom_check: OK 16 bytes md5 {_digest(FAKE_DATA, 'md5')}", out)

    def test_raw_wrong_content_fails_md5_first(self):
        code, _, err = _run([self._write("bad.z64", FAKE_OTHER)])
        self.assertEqual(code, 1)
        self.assertIn("rom_check: FAIL: MD5 ", err)

    def test_raw_wrong_sha1_only(self):
        lock = _fake_lock()
        lock["ROM_SHA1"] = "0" * 40
        with mock.patch.object(rom_check, "load", return_value=lock):
            code, _, err = _run([self._write("good2.z64", FAKE_DATA)])
        self.assertEqual(code, 1)
        self.assertIn(f"rom_check: FAIL: SHA1 {_digest(FAKE_DATA, 'sha1')} != {'0' * 40}", err)

    def test_decompressed_missing_file(self):
        code, _, err = _run(["--decompressed", str(self.dir / "nope.z64")])
        self.assertEqual(code, 1)
        self.assertIn("rom_check: FAIL: not a file", err)

    def test_decompressed_wrong_size(self):
        code, _, err = _run(["--decompressed", self._write("d_short.z64", FAKE_DATA + b"x")])
        self.assertEqual(code, 1)
        self.assertIn("rom_check: FAIL: size 17 != 16", err)

    def test_decompressed_wrong_content_same_size(self):
        code, _, err = _run(["--decompressed", self._write("d_bad.z64", FAKE_OTHER)])
        self.assertEqual(code, 1)
        self.assertRegex(err, r"rom_check: FAIL: SHA1 [0-9a-f]{40} != [0-9a-f]{40}")

    def test_decompressed_ok(self):
        code, out, err = _run(["--decompressed", self._write("d_good.z64", FAKE_DATA)])
        self.assertEqual(code, 0)
        self.assertEqual(err, "")
        self.assertIn(f"rom_check: OK decompressed 16 bytes sha1 {_digest(FAKE_DATA, 'sha1')}", out)

    def test_decompressed_without_file_is_usage_error(self):
        code, _, err = _run(["--decompressed"])
        self.assertEqual(code, 2)
        self.assertIn("--decompressed needs a file", err)

    def test_decompressed_empty_file_arg_is_usage_error(self):
        code, _, err = _run(["--decompressed", ""])
        self.assertEqual(code, 2)
        self.assertIn("--decompressed needs a file", err)

    def test_missing_lock_key_is_usage_error(self):
        lock = _fake_lock()
        del lock["ROM_SIZE"]
        with mock.patch.object(rom_check, "load", return_value=lock):
            code, _, err = _run([self._write("good3.z64", FAKE_DATA)])
        self.assertEqual(code, 2)
        self.assertIn("missing ROM_SIZE", err)

    def test_check_raw_raises_rom_check_error(self):
        with self.assertRaises(rom_check.RomCheckError):
            rom_check.check_raw(str(self.dir / "absent.z64"))

    def test_main_default_uses_rom_path_env(self):
        good = self._write("env_rom.z64", FAKE_DATA)
        with _env("TOOIE_ROM", good):
            code, out, _ = _run([])
        self.assertEqual(code, 0)
        self.assertIn("rom_check: OK 16 bytes", out)


_REAL_ROM = rom_path()
_REAL_DECOMP = BUILD / "extract" / "decompressed.us.z64"


class RomCheckRealTests(unittest.TestCase):
    @unittest.skipUnless(_REAL_ROM.is_file(), f"real ROM not present at {_REAL_ROM}")
    def test_real_rom_ok(self):
        code, out, err = _run([])
        self.assertEqual(code, 0, err)
        self.assertIn("rom_check: OK 33554432 bytes", out)

    @unittest.skipUnless(_REAL_DECOMP.is_file(), f"decompressed image not present at {_REAL_DECOMP}")
    def test_real_decompressed_ok(self):
        code, out, err = _run(["--decompressed", str(_REAL_DECOMP)])
        self.assertEqual(code, 0, err)
        self.assertIn("rom_check: OK decompressed 35098224 bytes", out)


class TooiePathsTests(unittest.TestCase):
    def test_rom_path_honours_tooie_rom(self):
        target = str(Path(tempfile.gettempdir()) / "custom-rom.z64")
        with _env("TOOIE_ROM", target):
            self.assertEqual(rom_path(), Path(target))

    def test_rom_path_default_is_repo_basename(self):
        with _env("TOOIE_ROM", None):
            self.assertEqual(rom_path(), TOOIE / "Banjo-Tooie (USA).z64")

    def test_rom_path_empty_env_falls_back(self):
        with _env("TOOIE_ROM", ""):
            self.assertEqual(rom_path(), TOOIE / "Banjo-Tooie (USA).z64")


if __name__ == "__main__":
    unittest.main()
