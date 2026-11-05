#!/usr/bin/env python3
"""Proves the handheld work leaves desktop builds alone.

Every modified source file, with TK64_MOBILE undefined (the desktop default), must resolve to the exact text of the base
commit. The only symbol resolved is TK64_MOBILE; other preprocessor lines stay as they are. Files listed in REVIEWED are
build scripts whose desktop paths were read by hand and are printed so the diff stays visible.

    python tools/handheld/check_desktop_unchanged.py [--base <commit>]

The default base is where this branch left origin/main, or HEAD in a clone without that ref. Once the handheld work is on the
branch being checked, pass the commit before it: comparing a commit with itself proves nothing.
Exit code 0: desktop text identical. Exit code 1: a file differs.
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE_SUFFIXES = {'.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hlsl', '.hlsli', '.inl'}
REVIEWED = {'CMakeLists.txt', 'lib/rt64/CMakeLists.txt'}
# Sources only the handheld build compiles: lib/rt64/CMakeLists.txt selects them when TK64_MOBILE is on.
MOBILE_ONLY = {'lib/rt64/src/contrib/plume/plume_gles_mobile.cpp', 'lib/rt64/src/tk64/tk64_depth_ram.h'}

IF_RE = re.compile(r'^\s*#\s*(if|ifdef|ifndef)\b(.*)$')
ELSE_RE = re.compile(r'^\s*#\s*(else|elif)\b')
ENDIF_RE = re.compile(r'^\s*#\s*endif\b')


def git(*args):
    return subprocess.run(['git', '-C', str(ROOT), *args], capture_output=True, check=True).stdout


def resolve_desktop(text):
    """Drops what is inside '#ifdef TK64_MOBILE', keeps its '#else' part, and leaves every other directive alone."""
    out = []
    stack = []  # One entry per open #if: 'mobile' (in the TK64_MOBILE part), 'desktop' (in its #else), or None (other).
    for line in text.splitlines(keepends=True):
        match = IF_RE.match(line)
        if match:
            is_mobile = match.group(1) == 'ifdef' and match.group(2).strip() == 'TK64_MOBILE'
            if 'TK64_MOBILE' in match.group(2) and not is_mobile:
                raise ValueError(f'unsupported condition: {line.strip()}')
            hidden = 'mobile' in stack
            stack.append('mobile' if is_mobile else None)
            if not is_mobile and not hidden:
                out.append(line)
            continue
        if ELSE_RE.match(line) and stack:
            if stack[-1] == 'mobile':
                stack[-1] = 'desktop'
                continue
            if stack[-1] == 'desktop':
                raise ValueError('#elif or a second #else on TK64_MOBILE')
        elif ENDIF_RE.match(line) and stack:
            top = stack.pop()
            if top is None and 'mobile' not in stack:
                out.append(line)
            continue
        if 'mobile' not in stack:
            out.append(line)
    if stack:
        raise ValueError('unterminated #if')
    return ''.join(out)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--base')
    args = parser.parse_args()
    if args.base is None:
        try:
            args.base = git('merge-base', 'HEAD', 'origin/main').decode().strip()
        except subprocess.CalledProcessError:
            args.base = 'HEAD'

    scope = ['--', 'lib', 'src', 'include', 'CMakeLists.txt']
    changed = git('diff', '--name-only', '--diff-filter=a', args.base, *scope).decode().split()
    # New since the base, committed or not: only the handheld build may compile them.
    added = git('diff', '--name-only', '--diff-filter=A', args.base, *scope).decode().split()
    added += git('ls-files', '--others', '--exclude-standard', '--', 'lib', 'src', 'include').decode().split()
    failures = 0
    identical = 0
    for path in changed:
        if path in REVIEWED:
            print(f'reviewed  {path}')
            continue
        if Path(path).suffix.lower() not in SOURCE_SUFFIXES:
            print(f'FAIL      {path}: not a source file this check understands')
            failures += 1
            continue
        if not (ROOT / path).exists():
            print(f'FAIL      {path}: deleted since {args.base}')
            failures += 1
            continue
        current = (ROOT / path).read_bytes().decode('utf-8')
        base = git('show', f'{args.base}:{path}').decode('utf-8')
        try:
            resolved = resolve_desktop(current)
        except ValueError as error:
            print(f'FAIL      {path}: {error}')
            failures += 1
            continue
        if resolved == base:
            identical += 1
            print(f'identical {path}')
        else:
            failures += 1
            print(f'FAIL      {path}: differs from {args.base} with TK64_MOBILE undefined')
            for number, (a, b) in enumerate(zip(resolved.splitlines(), base.splitlines()), 1):
                if a != b:
                    print(f'          first difference at line {number}:\n          now  {a[:120]}\n          base {b[:120]}')
                    break
    for path in sorted(set(added)):
        if path in MOBILE_ONLY:
            print(f'mobile    {path}')
        else:
            print(f'FAIL      {path}: new file outside the handheld list')
            failures += 1
    print(f'{identical} files identical to {args.base} on desktop, {failures} failures')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
