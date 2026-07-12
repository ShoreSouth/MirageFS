#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MODULES=(config common lsa object fsc fops namei runtime msh)

C_BOLD="\033[1m"
C_DIM="\033[2m"
C_GREEN="\033[32m"
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
  tools/test/run-ut.sh coverage        # 生成 lcov/genhtml 覆盖率报告
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

run_all() {
    spark_banner "all"
    printf "${C_BLUE}==>${C_RESET} 运行全部 UT\n"
    make -C "$ROOT_DIR" test
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
    make -C "$ROOT_DIR" "test-$module"
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
    MIRAGEFS_TEST_CASE="$case_name" make -C "$ROOT_DIR" "test-$module"
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
        spark_banner "coverage"
        make -C "$ROOT_DIR" coverage
        ;;
    coverage-check)
        spark_banner "coverage-check"
        if [[ $# -ge 2 ]]; then
            COVERAGE_MIN="$2" make -C "$ROOT_DIR" coverage-check
        else
            make -C "$ROOT_DIR" coverage-check
        fi
        ;;
    *)
        if [[ $# -ge 2 ]]; then
            run_case "$1" "$2"
        else
            run_module "$1"
        fi
        ;;
esac