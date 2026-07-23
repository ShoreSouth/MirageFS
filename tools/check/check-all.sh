#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT_DIR"

python3 tools/check/miragefs_lint.py --all --plain
python3 tools/check/format.py
tools/test/list-ut.py --check
