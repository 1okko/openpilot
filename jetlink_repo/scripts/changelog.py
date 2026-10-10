#!/usr/bin/env python3
"""
Copyright (c) 2026-, Zeph Leggett.

This file is part of jetlink and is licensed under the MIT License.
See the LICENSE file in the root directory for more details.

Release notes from CHANGELOG.md, for the GitHub release and the Mac app's
update feed.

Each release is a `Jetlink vX.Y.Z` line underlined with `=`, newest first.

    # the release's notes: its section without the heading
    python3 scripts/changelog.py notes v0.8.1

    # the update window's notes: this release and the nine before it, each
    # under its heading, which the app cuts at the release it has installed
    python3 scripts/changelog.py history v0.8.1

A tag with no section prints nothing and exits 0: the release then gets
GitHub's generated notes, and the feed a link to the release page.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

HEADING = re.compile(r'^Jetlink (v[0-9]\S*)\s*$')
UNDERLINE = re.compile(r'^=+$')
CHANGELOG = Path(__file__).resolve().parents[1] / 'CHANGELOG.md'


def sections(text: str) -> list[tuple[str, str]]:
  """(tag, notes) for every release, newest first. The notes drop the
  underline and the blank lines around them, and keep the ones inside."""
  found: list[tuple[str, list[str]]] = []
  for line in text.splitlines():
    match = HEADING.match(line)
    if match:
      found.append((match.group(1), []))
    elif found and not UNDERLINE.match(line):
      found[-1][1].append(line)
  return [(tag, '\n'.join(lines).strip('\n')) for tag, lines in found]


def notes(text: str, tag: str) -> str:
  """The section for `tag`, or '' when there is none."""
  return next((body for name, body in sections(text) if name == tag), '')


def history(text: str, tag: str, count: int = 10) -> str:
  """Markdown of `tag`'s section and up to `count - 1` older ones, each under a
  `## Jetlink vX.Y.Z` heading. '' when `tag` has no section."""
  releases = sections(text)
  start = next((i for i, (name, _) in enumerate(releases) if name == tag), None)
  if start is None:
    return ''
  return '\n\n'.join(f'## Jetlink {name}\n\n{body}' for name, body in releases[start:start + count])


def main(argv: list[str] | None = None) -> int:
  parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
  parser.add_argument('mode', choices=['notes', 'history'])
  parser.add_argument('tag', help='the release tag, like v0.8.1')
  args = parser.parse_args(argv)
  text = CHANGELOG.read_text(encoding='utf-8')
  out = notes(text, args.tag) if args.mode == 'notes' else history(text, args.tag)
  if out:
    sys.stdout.write(out + '\n')
  return 0


if __name__ == '__main__':
  sys.exit(main())
