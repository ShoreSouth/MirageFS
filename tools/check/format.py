#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MirageFS clang-format wrapper.

默认检查 src 和 tests 下的 C/H 文件。使用 --fix 原地格式化。
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

DEFAULT_PATHS = ("src", "tests")
C_SUFFIXES = {".c", ".h"}
SKIP_DIRS = {".git", "output", "build", "__pycache__"}


def is_c_file(path: Path) -> bool:
    return path.is_file() and path.suffix in C_SUFFIXES


def iter_files(paths: list[Path]) -> list[Path]:
    files: list[Path] = []
    for start in paths:
        if not start.exists():
            continue
        if is_c_file(start):
            files.append(start)
            continue
        if not start.is_dir():
            continue
        for path in start.rglob("*"):
            if any(part in SKIP_DIRS for part in path.parts):
                continue
            if is_c_file(path):
                files.append(path)
    return sorted(files)


def main() -> int:
    parser = argparse.ArgumentParser(description="MirageFS clang-format wrapper")
    parser.add_argument("paths", nargs="*",
                        help="待检查路径，默认检查 src tests")
    parser.add_argument("--fix", action="store_true",
                        help="原地格式化文件")
    parser.add_argument("--clang-format", default="clang-format",
                        help="clang-format 命令名或路径")
    args = parser.parse_args()

    clang_format = shutil.which(args.clang_format)
    if clang_format is None:
        print("ERROR: clang-format 未安装或不在 PATH 中", file=sys.stderr)
        print("安装示例：sudo apt-get update && "
              "sudo apt-get install -y clang-format", file=sys.stderr)
        return 127

    raw_paths = args.paths or list(DEFAULT_PATHS)
    files = iter_files([Path(path) for path in raw_paths])
    if not files:
        print("clang-format: 没有找到 C/H 文件")
        return 0

    if args.fix:
        cmd = [clang_format, "-i", *[str(path) for path in files]]
    else:
        cmd = [
            clang_format,
            "--dry-run",
            "-Werror",
            *[str(path) for path in files],
        ]

    result = subprocess.run(cmd, check=False)
    if result.returncode == 0:
        mode = "格式化完成" if args.fix else "格式检查通过"
        print(f"clang-format: {mode}，文件数={len(files)}")
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
