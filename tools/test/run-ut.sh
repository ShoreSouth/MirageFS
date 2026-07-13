#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MODULES=(config common lsa object fsc fops namei runtime msh)

C_BOLD="\033[1m"
C_DIM="\033[2m"
C_GREEN="\033[32m"
C_RED="\033[31m"
C_YELLOW="\033[33m"
C_BLUE="\033[34m"
C_MAGENTA="\033[35m"
C_RESET="\033[0m"

usage() {
    cat <<USAGE
MirageFS UT runner

用法:
  tools/test/run-ut.sh                 # 运行全部 UT
  tools/test/run-ut.sh all             # 运行全部 UT
  tools/test/run-ut.sh <module>        # 运行单模块 UT，例如 common
  tools/test/run-ut.sh <module> <case> # 运行单 case
  tools/test/run-ut.sh list            # 列出模块
  tools/test/run-ut.sh list <module>   # 列出模块下 case
  tools/test/run-ut.sh coverage        # 生成 lcov/genhtml 覆盖率报告并汇总
  tools/test/run-ut.sh coverage-check [min]

模块:
  ${MODULES[*]}

环境变量:
  NO_ANIM=1       关闭启动动画
  COVERAGE_MIN=90 覆盖率阈值，默认由 Makefile 使用 0
USAGE
}

has_module() {
    local target="$1"
    local m
    for m in "${MODULES[@]}"; do
        [[ "$m" == "$target" ]] && return 0
    done
    return 1
}

spark_banner() {
    [[ "${NO_ANIM:-0}" == "1" ]] && return 0
    [[ -t 1 ]] || return 0

    local mode="$1"
    local frames=(
"      /\\
   __/  \__    Mirage UT
  /__ __ __\\   charging $mode
     /__\\"
"      \\/
   __\\  /__    Mirage UT
  /__ __ __\\   checking $mode
     \\__/"
"      /\\
   __/  \__    Mirage UT
  /__ __ __\\   go $mode
     /__\\"
    )
    local frame
    for frame in "${frames[@]}"; do
        printf "\033[2J\033[H${C_MAGENTA}%s${C_RESET}\n" "$frame"
        sleep 0.08
    done
    printf "\033[2J\033[H"
}

list_modules() {
    printf "${C_BOLD}UT modules${C_RESET}\n"
    local m
    for m in "${MODULES[@]}"; do
        printf "  ${C_GREEN}%-8s${C_RESET} tests/%s\n" "$m" "$m"
    done
}

list_cases() {
    local module="$1"
    if ! has_module "$module"; then
        printf "unknown module: %s\n" "$module" >&2
        return 1
    fi

    printf "${C_BOLD}%s cases${C_RESET}\n" "$module"
    grep -Rho 'static int test_[a-zA-Z0-9_]*(void)' "$ROOT_DIR/tests/$module" 2>/dev/null \
        | sed -E 's/static int (test_[a-zA-Z0-9_]*)\(void\)/  \1/' \
        | sort
}

print_test_summary() {
    local log_file="$1"
    printf "\n${C_BOLD}UT 结果汇总${C_RESET}\n"
    printf "+------------+--------+--------+--------+\n"
    printf "| %-10s | %6s | %6s | %6s |\n" "module" "passed" "total" "status"
    printf "+------------+--------+--------+--------+\n"
    awk '
        /^\[suite\] [^:]+: [0-9]+\/[0-9]+ passed/ {
            module=$2; gsub(":", "", module);
            split($3, nums, "/");
            passed=nums[1]; total=nums[2];
            status=(passed == total ? "PASS" : "FAIL");
            printf "| %-10s | %6s | %6s | %6s |\n", module, passed, total, status;
            seen=1;
        }
        END { if (!seen) printf "| %-10s | %6s | %6s | %6s |\n", "-", "-", "-", "NOLOG"; }
    ' "$log_file"
    printf "+------------+--------+--------+--------+\n"
}

run_with_summary() {
    local log_file
    log_file="$(mktemp /tmp/miragefs-ut.XXXXXX.log)"
    set +e
    "$@" 2>&1 | tee "$log_file"
    local rc=${PIPESTATUS[0]}
    set -e
    print_test_summary "$log_file"
    rm -f "$log_file"
    return "$rc"
}

coverage_field() {
    local info_file="$1"
    local field="$2"
    lcov --summary "$info_file" 2>/dev/null \
        | awk -v field="$field" '$1 ~ field { gsub("%", "", $2); print $2; exit }'
}

print_coverage_summary() {
    local info_file="$ROOT_DIR/output/coverage/coverage.filtered.info"
    [[ -f "$info_file" ]] || return 0

    printf "\n${C_BOLD}覆盖率汇总${C_RESET}\n"
    printf "+------------+----------+----------+\n"
    printf "| %-10s | %8s | %8s |\n" "module" "lines" "funcs"
    printf "+------------+----------+----------+\n"

    local total_lines total_funcs
    total_lines="$(coverage_field "$info_file" lines)"
    total_funcs="$(coverage_field "$info_file" functions)"
    printf "| %-10s | %7s%% | %7s%% |\n" "TOTAL" "${total_lines:-0.0}" "${total_funcs:-0.0}"

    local module tmp lines funcs
    for module in "${MODULES[@]}"; do
        tmp="$(mktemp /tmp/miragefs-cov-${module}.XXXXXX.info)"
        if lcov --extract "$info_file" "$ROOT_DIR/src/$module/*" --output-file "$tmp" >/dev/null 2>&1; then
            lines="$(coverage_field "$tmp" lines)"
            funcs="$(coverage_field "$tmp" functions)"
            printf "| %-10s | %7s%% | %7s%% |\n" "$module" "${lines:-0.0}" "${funcs:-0.0}"
        else
            printf "| %-10s | %8s | %8s |\n" "$module" "n/a" "n/a"
        fi
        rm -f "$tmp"
    done
    printf "+------------+----------+----------+\n"
    printf "HTML: %s\n" "$ROOT_DIR/output/coverage/html/index.html"
}

run_all() {
    spark_banner "all"
    printf "${C_BLUE}==>${C_RESET} 运行全部 UT\n"
    run_with_summary make -C "$ROOT_DIR" test
}

run_module() {
    local module="$1"
    if ! has_module "$module"; then
        printf "unknown module: %s\n" "$module" >&2
        usage
        return 1
    fi
    spark_banner "$module"
    printf "${C_BLUE}==>${C_RESET} 运行模块 UT: ${C_BOLD}%s${C_RESET}\n" "$module"
    run_with_summary make -C "$ROOT_DIR" "test-$module"
}

run_case() {
    local module="$1"
    local case_name="$2"
    if ! has_module "$module"; then
        printf "unknown module: %s\n" "$module" >&2
        usage
        return 1
    fi
    spark_banner "$module/$case_name"
    printf "${C_BLUE}==>${C_RESET} 运行单 case: ${C_BOLD}%s/%s${C_RESET}\n" "$module" "$case_name"
    run_with_summary env MIRAGEFS_TEST_CASE="$case_name" make -C "$ROOT_DIR" "test-$module"
}

run_coverage() {
    spark_banner "coverage"
    run_with_summary make -C "$ROOT_DIR" coverage
    print_coverage_summary
}

cmd="${1:-all}"
case "$cmd" in
    -h|--help|help)
        usage
        ;;
    list)
        if [[ $# -ge 2 ]]; then
            list_cases "$2"
        else
            list_modules
        fi
        ;;
    all)
        run_all
        ;;
    coverage)
        run_coverage
        ;;
    coverage-check)
        spark_banner "coverage-check"
        if [[ $# -ge 2 ]]; then
            run_with_summary env COVERAGE_MIN="$2" make -C "$ROOT_DIR" coverage-check
        else
            run_with_summary make -C "$ROOT_DIR" coverage-check
        fi
        print_coverage_summary
        ;;
    *)
        if [[ $# -ge 2 ]]; then
            run_case "$1" "$2"
        else
            run_module "$1"
        fi
        ;;
esac