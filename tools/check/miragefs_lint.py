#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MirageFS 轻量静态检查工具。

当前检查项：
- encoding.bom              : 禁止 UTF-8 BOM
- encoding.final_newline    : 文件末尾必须有换行
- whitespace.trailing       : 禁止行尾空白
- whitespace.blank_lines    : 连续空行不超过 2 行
- style.line_length         : 行宽超过阈值时告警
- function.param_count      : 函数参数数量超过阈值时告警
- io.forbidden              : src C 代码禁止直接 printf/fprintf/puts
- c.large_stack_array       : 固定大小栈数组超过阈值时告警
- naming.typedef_struct     : typedef struct 名称应以 _t 结尾
- naming.typedef_enum       : typedef enum 名称应以 _t 结尾
- naming.macro              : 项目宏名应使用全大写风格

设计目标：
1. 默认检查 src，方便作为日常源代码检查入口。
2. 默认只让硬错误导致失败，风格类问题输出 WARN。
3. 使用 --strict 时，WARN 也会让脚本返回非 0，便于逐步收紧规范。
4. 默认输出问题明细表和规则汇总表；使用 --detail-limit 可限制明细表行数。
5. 使用 --plain 时关闭 banner/进度条/表格，便于 CI 或日志收集。
"""

from __future__ import annotations

import argparse
import os
import re
import sys
import time
import unicodedata
from dataclasses import dataclass
from pathlib import Path

DEFAULT_PATHS = ("src",)
ALL_PATHS = ("src", "tools", "docs")
TEXT_SUFFIXES = {".c", ".h", ".md", ".py", ".mk"}
SKIP_DIRS = {".git", "output", "build", "__pycache__"}

RULE_LABELS = {
    "encoding.bom": "UTF-8 BOM",
    "encoding.final_newline": "末尾换行",
    "whitespace.trailing": "行尾空白",
    "whitespace.blank_lines": "连续空行",
    "style.line_length": "行宽",
    "function.param_count": "函数参数",
    "io.forbidden": "直接输出",
    "c.raw_return": "裸错误返回",
    "c.large_stack_array": "大栈数组",
    "naming.typedef_struct": "struct typedef",
    "naming.typedef_enum": "enum typedef",
    "naming.macro": "宏命名",
}

FUNC_HEAD_RE = re.compile(
    r"^([A-Za-z_][\w\s\*]*?\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*\((.*)$"
)
STACK_ARRAY_RE = re.compile(
    r"\b[A-Za-z_][\w\s\*]*\s+[A-Za-z_][\w]*\s*\[(\d+)\]"
)
FORBIDDEN_IO_RE = re.compile(r"\b(printf|fprintf|puts)\s*\(")
RETURN_RAW_ERR_RE = re.compile(r"\breturn\s+-1\s*;")
TYPEDEF_END_RE = re.compile(r"^}\s*([A-Za-z_][A-Za-z0-9_]*)\s*;")
MACRO_RE = re.compile(r"^#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)")
MACRO_NAME_RE = re.compile(r"^[A-Z][A-Z0-9_]*$")


@dataclass
class Finding:
    level: str
    rule: str
    path: Path
    line: int
    message: str


def is_text_file(path: Path) -> bool:
    if path.name == "Makefile":
        return True
    return path.suffix in TEXT_SUFFIXES


def is_output_allowed(path: Path) -> bool:
    allowed = {
        Path("src/app/main.c"),
        Path("src/common/log/fs_log.c"),
        Path("src/common/trace/fs_trace.h"),
        Path("src/common/utils/fs_utils.h"),
    }
    return path in allowed


def iter_files(paths: list[Path]) -> list[Path]:
    files: list[Path] = []
    for start in paths:
        if not start.exists():
            continue
        if start.is_file():
            if is_text_file(start):
                files.append(start)
            continue
        for path in start.rglob("*"):
            if any(part in SKIP_DIRS for part in path.parts):
                continue
            if path.is_file() and is_text_file(path):
                files.append(path)
    return sorted(files)


def count_params(signature: str) -> int:
    params = signature.strip()
    if params.endswith(")"):
        params = params[:-1]
    params = params.strip()
    if (not params) or params == "void":
        return 0
    return len([item for item in params.split(",") if item.strip()])


def collect_function_signature(lines: list[str], start_idx: int) -> tuple[str, int] | None:
    first = lines[start_idx].strip()
    if first.startswith(("if ", "for ", "while ", "switch ", "return ")):
        return None
    if "(" not in first:
        return None
    if first.startswith(("#", "typedef", "struct ", "enum ")):
        return None

    text = first
    idx = start_idx
    while ")" not in text and idx + 1 < len(lines):
        idx += 1
        text += " " + lines[idx].strip()
        if idx - start_idx > 12:
            return None

    tail = text.split(")", 1)[1].strip() if ")" in text else ""
    if tail.startswith((";", "=", ",")):
        return None
    if not (tail.startswith("{") or tail == ""):
        return None

    return text, idx


def add(findings: list[Finding], level: str, rule: str,
        path: Path, line: int, message: str) -> None:
    findings.append(Finding(level, rule, path, line, message))


def check_param_count(path: Path, lines: list[str], max_params: int) -> list[Finding]:
    findings: list[Finding] = []
    idx = 0
    while idx < len(lines):
        sig = collect_function_signature(lines, idx)
        if sig is None:
            idx += 1
            continue
        text, end_idx = sig
        match = FUNC_HEAD_RE.match(text)
        if match is not None:
            inside = text[text.find("(") + 1:text.rfind(")")]
            nr = count_params(inside)
            if nr > max_params:
                name = match.group(2)
                add(findings, "WARN", "function.param_count", path, idx + 1,
                    f"函数 {name} 参数数量为 {nr}，超过阈值 {max_params}，建议封装 ctx/args")
        idx = max(idx + 1, end_idx + 1)
    return findings


def check_typedef_names(path: Path, lines: list[str]) -> list[Finding]:
    findings: list[Finding] = []
    typedef_kind: str | None = None
    depth = 0

    for idx, line in enumerate(lines, start=1):
        stripped = line.strip()
        code_part = stripped.split("/*", 1)[0].split("//", 1)[0].strip()

        if typedef_kind is None:
            if code_part.startswith("typedef struct"):
                typedef_kind = "struct"
                depth = code_part.count("{") - code_part.count("}")
            elif code_part.startswith("typedef enum"):
                typedef_kind = "enum"
                depth = code_part.count("{") - code_part.count("}")
            continue

        depth += code_part.count("{") - code_part.count("}")
        if depth > 0:
            continue

        match = TYPEDEF_END_RE.match(code_part)
        if match is None:
            continue

        name = match.group(1)
        if not name.endswith("_t"):
            rule = ("naming.typedef_struct" if typedef_kind == "struct"
                    else "naming.typedef_enum")
            label = "struct" if typedef_kind == "struct" else "enum"
            add(findings, "WARN", rule, path, idx,
                f"typedef {label} 名称 {name} 建议以 _t 结尾")
        typedef_kind = None

    return findings


def check_macro_name(path: Path, idx: int, code_part: str,
                     findings: list[Finding]) -> None:
    match = MACRO_RE.match(code_part)
    if match is None:
        return

    name = match.group(1)
    # 头文件保护和特殊双下划线宏不在项目宏命名规则内。
    if name.startswith("__"):
        return
    if not MACRO_NAME_RE.match(name):
        add(findings, "WARN", "naming.macro", path, idx,
            f"宏 {name} 建议使用全大写下划线风格")


def check_file(path: Path, max_line_length: int,
               max_params: int, max_stack_array: int) -> list[Finding]:
    findings: list[Finding] = []
    data = path.read_bytes()

    if data.startswith(b"\xef\xbb\xbf"):
        add(findings, "ERROR", "encoding.bom", path, 1,
            "文件包含 UTF-8 BOM，请使用无 BOM UTF-8")

    if data and not data.endswith(b"\n"):
        add(findings, "ERROR", "encoding.final_newline", path, 1,
            "文件末尾缺少换行")

    try:
        text = data.decode("utf-8-sig")
    except UnicodeDecodeError as exc:
        add(findings, "ERROR", "encoding.bom", path, exc.start + 1,
            "文件不是合法 UTF-8")
        return findings

    lines = text.splitlines()
    empty_run = 0

    for idx, line in enumerate(lines, start=1):
        if line.rstrip(" \t") != line:
            add(findings, "ERROR", "whitespace.trailing", path, idx,
                "行尾存在多余空白")
        if "\t" in line:
            add(findings, "WARN", "whitespace.trailing", path, idx,
                "发现 tab，项目代码建议统一使用空格")
        if path.suffix != ".md" and len(line) > max_line_length:
            add(findings, "WARN", "style.line_length", path, idx,
                f"行宽 {len(line)} 超过阈值 {max_line_length}")

        if line.strip() == "":
            empty_run += 1
            if empty_run > 2:
                add(findings, "WARN", "whitespace.blank_lines", path, idx,
                    "连续空行超过 2 行")
        else:
            empty_run = 0

        if path.suffix in {".c", ".h"}:
            stripped = line.strip()
            code_part = stripped.split("/*", 1)[0].split("//", 1)[0].strip()
            if FORBIDDEN_IO_RE.search(code_part) and not is_output_allowed(path):
                add(findings, "WARN", "io.forbidden", path, idx,
                    "发现直接使用 printf/fprintf/puts，请确认是否应改为日志接口")
            if RETURN_RAW_ERR_RE.search(code_part):
                add(findings, "WARN", "c.raw_return", path, idx,
                    "疑似裸返回错误值，请确认是否应使用 fs_error_t")
            arr = STACK_ARRAY_RE.search(code_part)
            if arr is not None and int(arr.group(1)) > max_stack_array:
                add(findings, "WARN", "c.large_stack_array", path, idx,
                    f"栈数组超过 {max_stack_array} 字节，建议改为堆/内存池")
            check_macro_name(path, idx, code_part, findings)

    if path.suffix in {".c", ".h"}:
        findings.extend(check_typedef_names(path, lines))
        findings.extend(check_param_count(path, lines, max_params))

    return findings


def use_rich_output(plain: bool) -> bool:
    if plain:
        return False
    if os.environ.get("NO_COLOR"):
        return False
    return sys.stdout.isatty()


def print_banner(enabled: bool, paths: list[Path]) -> None:
    if not enabled:
        return
    targets = ", ".join(str(p) for p in paths)
    print("╭────────────────────────────────╮")
    print("│ MirageFS Lint · Source Radar   │")
    print("╰────────────────────────────────╯")
    print(f"扫描范围: {targets}")


def print_progress(enabled: bool, current: int, total: int, path: Path) -> None:
    if not enabled:
        return
    width = 28
    done = 0 if total == 0 else int(width * current / total)
    bar = "█" * done + "░" * (width - done)
    percent = 100 if total == 0 else int(100 * current / total)
    name = str(path)
    if len(name) > 42:
        name = "..." + name[-39:]
    print(f"\r[{bar}] {percent:3d}% {name:<42}", end="", flush=True)
    if current == total:
        print()


def summarize(findings: list[Finding]) -> dict[str, dict[str, int]]:
    summary = {rule: {"ERROR": 0, "WARN": 0} for rule in RULE_LABELS}
    for item in findings:
        summary.setdefault(item.rule, {"ERROR": 0, "WARN": 0})
        summary[item.rule][item.level] += 1
    return summary


def module_of(path: Path) -> str:
    parts = path.parts
    if len(parts) >= 2 and parts[0] == "src":
        return parts[1]
    if len(parts) >= 1 and parts[0] in {"docs", "tools"}:
        return parts[0]
    return "-"


def display_width(text: str) -> int:
    width = 0
    for ch in text:
        if unicodedata.combining(ch):
            continue
        width += 2 if unicodedata.east_asian_width(ch) in {"F", "W"} else 1
    return width


def clip(text: str, width: int) -> str:
    if display_width(text) <= width:
        return text
    out = ""
    used = 0
    limit = max(0, width - 3)
    for ch in text:
        ch_width = 2 if unicodedata.east_asian_width(ch) in {"F", "W"} else 1
        if used + ch_width > limit:
            break
        out += ch
        used += ch_width
    return out + "..."


def pad_cell(text: str, width: int, align: str = "left") -> str:
    text = clip(str(text), width)
    padding = max(0, width - display_width(text))
    if align == "right":
        return " " * padding + text
    return text + " " * padding


def print_detail_table(findings: list[Finding], plain: bool,
                       detail_limit: int) -> None:
    if plain or not findings:
        return

    shown = findings[:detail_limit]
    top = ("┌───────┬──────────────────────┬──────────┬"
           "────────────────────────────────┬──────┬"
           "──────────────────────────────┐")
    head = ("│ Level │ Rule                 │ Module   │"
            " File                           │ Line │"
            " Message                      │")
    sep = ("├───────┼──────────────────────┼──────────┼"
           "────────────────────────────────┼──────┼"
           "──────────────────────────────┤")
    bot = ("└───────┴──────────────────────┴──────────┴"
           "────────────────────────────────┴──────┴"
           "──────────────────────────────┘")

    print(top)
    print(head)
    print(sep)
    for item in shown:
        print(f"│ {pad_cell(item.level, 5)} │ {pad_cell(item.rule, 20)} │ "
              f"{pad_cell(module_of(item.path), 8)} │ "
              f"{pad_cell(str(item.path), 30)} │ "
              f"{pad_cell(item.line, 4, 'right')} │ "
              f"{pad_cell(item.message, 28)} │")
    print(bot)
    if len(findings) > detail_limit:
        print(f"问题明细仅显示前 {detail_limit} 条；完整列表见上方逐行输出。")


def print_summary_table(findings: list[Finding], plain: bool) -> None:
    summary = summarize(findings)
    rows = []
    for rule, counts in summary.items():
        if counts["ERROR"] == 0 and counts["WARN"] == 0:
            continue
        rows.append((RULE_LABELS.get(rule, rule), counts["ERROR"], counts["WARN"]))

    errors = sum(1 for item in findings if item.level == "ERROR")
    warnings = sum(1 for item in findings if item.level == "WARN")

    if plain:
        print(f"检查完成：ERROR={errors} WARN={warnings}")
        return

    print("┌──────────────────────┬───────┬──────┐")
    print("│ Rule                 │ Error │ Warn │")
    print("├──────────────────────┼───────┼──────┤")
    if not rows:
        print("│ clean                │     0 │    0 │")
    for label, err_nr, warn_nr in rows:
        print(f"│ {pad_cell(label, 20)} │ "
              f"{pad_cell(err_nr, 5, 'right')} │ "
              f"{pad_cell(warn_nr, 4, 'right')} │")
    print("└──────────────────────┴───────┴──────┘")
    print(f"检查完成：ERROR={errors} WARN={warnings}")


def main() -> int:
    parser = argparse.ArgumentParser(description="MirageFS lightweight lint")
    parser.add_argument("paths", nargs="*", default=None,
                        help="待检查路径，默认检查 src")
    parser.add_argument("--strict", action="store_true",
                        help="将 WARN 也视为失败")
    parser.add_argument("--all", action="store_true",
                        help="检查 src、tools、docs 全量路径")
    parser.add_argument("--plain", action="store_true",
                        help="关闭 banner、进度条和汇总表格")
    parser.add_argument("--max-line-length", type=int, default=120,
                        help="行宽告警阈值，默认 120")
    parser.add_argument("--max-params", type=int, default=6,
                        help="函数参数数量告警阈值，默认 6")
    parser.add_argument("--max-stack-array", type=int, default=4096,
                        help="栈数组大小告警阈值，默认 4096")
    parser.add_argument("--detail-limit", type=int, default=80,
                        help="表格中最多显示的问题明细数量，默认 80")
    args = parser.parse_args()

    raw_paths = list(ALL_PATHS) if args.all else (args.paths or list(DEFAULT_PATHS))
    paths = [Path(p) for p in raw_paths]
    files = iter_files(paths)
    rich = use_rich_output(args.plain)

    print_banner(rich, paths)
    findings: list[Finding] = []
    start = time.time()
    for idx, path in enumerate(files, start=1):
        print_progress(rich, idx, len(files), path)
        findings.extend(check_file(path, args.max_line_length,
                                   args.max_params,
                                   args.max_stack_array))

    for item in findings:
        print(f"{item.level}: {item.path}:{item.line}: [{item.rule}] {item.message}")

    if rich:
        elapsed = time.time() - start
        print(f"扫描文件：{len(files)}，耗时：{elapsed:.2f}s")
    print_detail_table(findings, args.plain, args.detail_limit)
    print_summary_table(findings, args.plain)

    errors = [item for item in findings if item.level == "ERROR"]
    warnings = [item for item in findings if item.level == "WARN"]
    if errors or (args.strict and warnings):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
