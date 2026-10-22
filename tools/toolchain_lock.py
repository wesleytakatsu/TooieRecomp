#!/usr/bin/env python3
"""Parse tools/toolchain.lock (shell-sourceable KEY=value) into a string dict."""
from __future__ import annotations

import re
from pathlib import Path

_LOCK = Path(__file__).resolve().parent / "toolchain.lock"
_ENTRY = re.compile(r"^([A-Z_][A-Z0-9_]*)=(.*)$")


def load(path: str | Path | None = None) -> dict[str, str]:
    """Return every KEY in the lock file. Duplicate keys: last one wins."""
    lock = Path(path) if path is not None else _LOCK
    out: dict[str, str] = {}
    text = lock.read_text(encoding="utf-8")
    for lineno, line in enumerate(text.splitlines(), 1):
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        match = _ENTRY.match(stripped)
        if not match:
            raise ValueError(f"toolchain.lock:{lineno}: malformed line: {stripped}")
        out[match.group(1)] = _value(match.group(2), lineno)
    return out


def _value(raw: str, lineno: int) -> str:
    # Membership on a str is a substring test, so "" matches. Quote chars only.
    if raw[:1] in ('"', "'"):
        return _quoted(raw, lineno)
    # Unquoted: drop a trailing ` # comment`. `#` glued to the token stays.
    comment = re.search(r"\s+#", raw)
    body = (raw[: comment.start()] if comment else raw).rstrip()
    if body != body.strip() or any(ch.isspace() for ch in body):
        raise ValueError(f"toolchain.lock:{lineno}: malformed unquoted value")
    return body


def _quoted(raw: str, lineno: int) -> str:
    quote = raw[0]
    chars: list[str] = []
    i = 1
    while i < len(raw):
        ch = raw[i]
        if ch == quote:
            tail = raw[i + 1 :].lstrip()
            if tail and not tail.startswith("#"):
                raise ValueError(f"toolchain.lock:{lineno}: trailing junk after quoted value")
            return "".join(chars)
        # Double quotes only: a backslash escapes the next character.
        if quote == '"' and ch == "\\" and i + 1 < len(raw):
            chars.append(raw[i + 1])
            i += 2
            continue
        chars.append(ch)
        i += 1
    raise ValueError(f"toolchain.lock:{lineno}: unclosed quote")
