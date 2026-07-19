#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""MirageFS UT 用例索引检索工具。"""

from __future__ import annotations

import argparse
import ast
import json
import re
import sys
import unicodedata
from dataclasses import asdict, dataclass
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parents[2]
TEST_DIR = ROOT_DIR / "tests"
FRAMEWORK_H = TEST_DIR / "framework" / "test_framework.h"
MODULE_ORDER = [
    "config",
    "common",
    "lsa",
    "object",
    "fsc",
    "fops",
    "namei",
    "runtime",
    "msh",
]

ENUM_RE = re.compile(r"\b([A-Z][A-Z0-9_]+)\s*=\s*(0x[0-9a-fA-F]+|\d+)")


@dataclass
class UtCase:
    module: str
    component: str
    list_no: str
    case_no: str
    case_name: str
    scenario: str
    fault: str
    expected: str
    source_file: str
    source_line: int


def display_width(text: str) -> int:
    width = 0
    for ch in str(text):
        if unicodedata.combining(ch):
            continue
        width += 2 if unicodedata.east_asian_width(ch) in {"F", "W"} else 1
    return width


def clip(text: str, width: int) -> str:
    text = str(text)
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


def pad(text: str, width: int) -> str:
    text = clip(text, width)
    return text + " " * max(0, width - display_width(text))


def parse_int(text: str) -> int:
    return int(text, 0)


def parse_enums(text: str) -> dict[str, int]:
    values: dict[str, int] = {}
    for name, raw in ENUM_RE.findall(text):
        values[name] = parse_int(raw)
    return values


def find_matching_paren(text: str, open_idx: int) -> int:
    depth = 0
    in_string = False
    in_char = False
    escape = False
    for idx in range(open_idx, len(text)):
        ch = text[idx]
        if escape:
            escape = False
            continue
        if ch == "\\" and (in_string or in_char):
            escape = True
            continue
        if ch == '"' and not in_char:
            in_string = not in_string
            continue
        if ch == "'" and not in_string:
            in_char = not in_char
            continue
        if in_string or in_char:
            continue
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                return idx
    raise ValueError("unmatched paren")


def split_args(arg_text: str) -> list[str]:
    args: list[str] = []
    start = 0
    depth = 0
    in_string = False
    in_char = False
    escape = False
    for idx, ch in enumerate(arg_text):
        if escape:
            escape = False
            continue
        if ch == "\\" and (in_string or in_char):
            escape = True
            continue
        if ch == '"' and not in_char:
            in_string = not in_string
            continue
        if ch == "'" and not in_string:
            in_char = not in_char
            continue
        if in_string or in_char:
            continue
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        elif ch == "," and depth == 0:
            args.append(arg_text[start:idx].strip())
            start = idx + 1
    args.append(arg_text[start:].strip())
    return args


def iter_test_case_calls(text: str) -> list[tuple[int, int, list[str]]]:
    calls: list[tuple[int, int, list[str]]] = []
    pos = 0
    while True:
        start = text.find("TEST_CASE(", pos)
        if start < 0:
            break
        open_idx = start + len("TEST_CASE")
        close_idx = find_matching_paren(text, open_idx)
        args = split_args(text[open_idx + 1:close_idx])
        calls.append((start, close_idx + 1, args))
        pos = close_idx + 1
    return calls


def eval_symbol(raw: str, values: dict[str, int]) -> int:
    raw = raw.strip()
    if raw in values:
        return values[raw]
    return parse_int(raw.rstrip("Uu"))


def eval_no(raw: str, values: dict[str, int]) -> int:
    name, rest = raw.split("(", 1)
    inner = rest.rsplit(")", 1)[0]
    args = split_args(inner)
    if name.strip() == "UT_LIST_NO" and len(args) == 3:
        module = eval_symbol(args[0], values)
        component = eval_symbol(args[1], values)
        list_id = eval_symbol(args[2], values)
        item = 0
    elif name.strip() == "UT_CASE_NO" and len(args) == 4:
        module = eval_symbol(args[0], values)
        component = eval_symbol(args[1], values)
        list_id = eval_symbol(args[2], values)
        item = eval_symbol(args[3], values)
    else:
        return eval_symbol(raw, values)
    return ((module & 0xff) << 24) | ((component & 0xff) << 16) | \
           ((list_id & 0x0f) << 12) | (item & 0x0fff)


def eval_c_string(raw: str) -> str:
    try:
        return ast.literal_eval(raw)
    except Exception:
        parts = re.findall(r'"(?:\\.|[^"\\])*"', raw, flags=re.S)
        return "".join(ast.literal_eval(part) for part in parts)


def module_name_from_no(module_no: int, module_names: dict[int, str]) -> str:
    return module_names.get(module_no, f"unknown-{module_no:02x}")


def component_name(raw_component: int,
                   values: dict[str, int],
                   module: str) -> str:
    prefix = f"TEST_{module.upper()}_COMPONENT_"
    for name, value in values.items():
        if value == raw_component and name.startswith(prefix):
            return name[len(prefix):].lower()
    return f"component-{raw_component:02x}"


def load_cases() -> list[UtCase]:
    framework_values = parse_enums(FRAMEWORK_H.read_text(encoding="utf-8-sig"))
    module_names = {
        value: name.removeprefix("UT_MOD_").lower()
        for name, value in framework_values.items()
        if name.startswith("UT_MOD_")
    }
    cases: list[UtCase] = []

    for module in MODULE_ORDER:
        path = TEST_DIR / module / f"test_{module}.c"
        if not path.exists():
            continue
        text = path.read_text(encoding="utf-8-sig")
        values = {**framework_values, **parse_enums(text)}
        for start, _end, args in iter_test_case_calls(text):
            if len(args) != 6:
                raise ValueError(f"{path}: TEST_CASE 参数数量应为 6，实际为 {len(args)}")
            list_no = eval_no(args[0], values)
            case_no = eval_no(args[1], values)
            case_module = module_name_from_no((case_no >> 24) & 0xff,
                                             module_names)
            component = component_name((case_no >> 16) & 0xff,
                                       values,
                                       case_module)
            source_line = text.count("\n", 0, start) + 1
            cases.append(UtCase(
                module=case_module,
                component=component,
                list_no=f"0x{list_no:08x}",
                case_no=f"0x{case_no:08x}",
                case_name=args[2].strip(),
                scenario=eval_c_string(args[3]),
                fault=eval_c_string(args[4]),
                expected=eval_c_string(args[5]),
                source_file=str(path.relative_to(ROOT_DIR)),
                source_line=source_line,
            ))
    return cases


def parse_query_no(raw: str | None) -> int | None:
    if not raw:
        return None
    try:
        return parse_int(raw)
    except ValueError:
        return None


def apply_filters(cases: list[UtCase], args: argparse.Namespace) -> list[UtCase]:
    query_no = parse_query_no(args.query)
    module = args.module
    list_no = args.list_no
    case_no = args.case_no
    name = args.name

    if args.query:
        if query_no is not None:
            if (query_no & 0x0fff) == 0:
                list_no = f"0x{query_no:08x}"
            else:
                case_no = f"0x{query_no:08x}"
        elif args.query in MODULE_ORDER:
            module = args.query
        else:
            name = args.query

    rows = cases
    if module:
        rows = [row for row in rows if row.module == module]
    if list_no:
        target = f"0x{parse_int(list_no):08x}"
        rows = [row for row in rows if row.list_no == target]
    if case_no:
        target = f"0x{parse_int(case_no):08x}"
        rows = [row for row in rows if row.case_no == target]
    if name:
        rows = [row for row in rows if name in row.case_name]
    return rows


def print_table(rows: list[UtCase], plain: bool) -> None:
    if plain:
        for row in rows:
            print("\t".join([
                row.module,
                row.component,
                row.list_no,
                row.case_no,
                row.case_name,
                row.scenario,
                row.fault,
                row.expected,
                f"{row.source_file}:{row.source_line}",
            ]))
        return

    headers = ["module", "component", "list_no", "case_no", "case", "scenario"]
    widths = [8, 12, 10, 10, 42, 24]
    print(" ".join(pad(h, w) for h, w in zip(headers, widths)))
    print(" ".join("-" * w for w in widths))
    for row in rows:
        print(" ".join([
            pad(row.module, widths[0]),
            pad(row.component, widths[1]),
            pad(row.list_no, widths[2]),
            pad(row.case_no, widths[3]),
            pad(row.case_name, widths[4]),
            pad(row.scenario, widths[5]),
        ]))
    print(f"\n共 {len(rows)} 个 UT case")


def validate_cases(cases: list[UtCase]) -> list[str]:
    errors: list[str] = []
    seen_cases: dict[str, UtCase] = {}
    for row in cases:
        case_value = parse_int(row.case_no)
        list_value = parse_int(row.list_no)
        if (list_value & 0x0fff) != 0:
            errors.append(f"{row.case_name}: list_no 低 12 位必须为 0")
        if (case_value & 0x0fff) == 0:
            errors.append(f"{row.case_name}: case_no 低 12 位不能为 0")
        if (case_value & 0xfffff000) != list_value:
            errors.append(f"{row.case_name}: case_no 与 list_no 不属于同一用例集")
        if row.case_no in seen_cases:
            prev = seen_cases[row.case_no]
            errors.append(f"{row.case_no}: 与 {prev.case_name} 编号重复")
        seen_cases[row.case_no] = row
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(
        description="检索 MirageFS UT 用例编号、描述和源码位置")
    parser.add_argument("query", nargs="?",
                        help="模块名、list_no、case_no 或 case 名片段")
    parser.add_argument("--module", choices=MODULE_ORDER,
                        help="只显示指定模块")
    parser.add_argument("--list", dest="list_no",
                        help="只显示指定 list_no，例如 0x06031000")
    parser.add_argument("--case", dest="case_no",
                        help="只显示指定 case_no，例如 0x06031001")
    parser.add_argument("--name", help="按 case 函数名片段过滤")
    parser.add_argument("--json", action="store_true",
                        help="输出 JSON，便于后续工具消费")
    parser.add_argument("--plain", action="store_true",
                        help="输出 tab 分隔文本")
    parser.add_argument("--check", action="store_true",
                        help="校验编号唯一性和 list/case 对应关系")
    args = parser.parse_args()

    cases = load_cases()
    errors = validate_cases(cases)
    if args.check:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        if errors:
            return 1
        print(f"UT 编号检查通过：{len(cases)} 个 case")
        return 0

    rows = apply_filters(cases, args)
    if args.json:
        print(json.dumps([asdict(row) for row in rows],
                         ensure_ascii=False,
                         indent=2))
    else:
        print_table(rows, args.plain)
    return 0


if __name__ == "__main__":
    sys.exit(main())
