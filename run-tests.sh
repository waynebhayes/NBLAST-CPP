#!/bin/bash
set -uo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"

# Usage: ./run-tests.sh [--full]
#   --full  also run the slow R comparisons (1-random, 2-matrix); needs R + nat.nblast
FULL=0
[[ "${1:-}" == "--full" ]] && FULL=1
MIN_CORRELATION=0.99

GREEN='\033[0;32m'
RED='\033[0;31m'
BOLD='\033[1m'
RESET='\033[0m'
LINE=$(printf '%.0s─' {1..60})

total_pass=0
total_fail=0

# ── 1. build ──────────────────────────────────────────────────────────────────
printf "${BOLD}Building nblast++...${RESET} "
if build_output=$(make -C "$REPO_ROOT" release test_runner 2>&1); then
    printf "${GREEN}ok${RESET}\n"
else
    printf "${RED}FAILED${RESET}\n"
    printf '%s\n' "$build_output"
    exit 1
fi

# ── 2. unit tests ─────────────────────────────────────────────────────────────
printf "\n${BOLD}Unit Tests${RESET}\n%s\n" "$LINE"

unit_output=$("$REPO_ROOT/test_runner" 2>&1)
printf '%s\n' "$unit_output"

unit_clean=$(printf '%s' "$unit_output" | sed 's/\x1b\[[0-9;]*m//g')
unit_pass=$(printf '%s' "$unit_clean" | grep -oP 'Passed\s*:\s*\K[0-9]+' || true)
unit_fail=$(printf '%s' "$unit_clean" | grep -oP 'Failed\s*:\s*\K[0-9]+' || true)
if [[ -z "$unit_pass" || -z "$unit_fail" ]]; then
    printf "${RED}unit tests produced no pass/fail summary (did test_runner run?)${RESET}\n"
    total_fail=$((total_fail + 1))
fi
total_pass=$((total_pass + ${unit_pass:-0}))
total_fail=$((total_fail + ${unit_fail:-0}))

# ── 3. regression tests ───────────────────────────────────────────────────────
printf "\n${BOLD}Regression Tests${RESET}\n%s\n" "$LINE"

while IFS= read -r -d '' test_script; do
    label="${test_script#"$REPO_ROOT/regression-tests/"}"
    label="${label%-test.sh}"
    printf "  %-50s" "$label"

    if reg_output=$(bash "$test_script" 2>&1); then
        printf "${GREEN}PASS${RESET}\n"
        total_pass=$((total_pass + 1))
    else
        printf "${RED}FAIL${RESET}\n"
        printf '%s\n' "$reg_output" | sed 's/^/    /'
        total_fail=$((total_fail + 1))
    fi
done < <(find "$REPO_ROOT/regression-tests" -name "*-test.sh" -print0 | sort -z)

# ── 3b. slow comparisons against R (opt-in) ───────────────────────────────────

if [[ $FULL -eq 1 ]]; then
    printf "\n${BOLD}R Comparisons${RESET} (Pearson r >= %s)\n%s\n" "$MIN_CORRELATION" "$LINE"
    for t in 1-random 2-matrix; do
        dir="$REPO_ROOT/regression-tests/$t"
        printf "  %-50s" "$t"
        if ! out=$(bash "$dir/compare.sh" 2>&1); then
            printf "${RED}FAIL${RESET} (compare.sh error)\n"
            printf '%s\n' "$out" | tail -15 | sed 's/^/    /'
            total_fail=$((total_fail + 1))
        elif r=$(python3 -I "$REPO_ROOT/scripts/lib/check-correlation.py" \
                    "$dir/r_scores.tsv" "$dir/cpp_scores.tsv" "$MIN_CORRELATION" 2>&1); then
            printf "${GREEN}PASS${RESET} %s\n" "$r"
            total_pass=$((total_pass + 1))
        else
            printf "${RED}FAIL${RESET} %s\n" "$r"
            total_fail=$((total_fail + 1))
        fi
    done
fi

# ── 4. summary ────────────────────────────────────────────────────────────────
total=$((total_pass + total_fail))
printf "\n%s\n" "$LINE"
if [[ $total_fail -eq 0 ]]; then
    printf "${GREEN}${BOLD}ALL PASSED${RESET}  ${total_pass}/${total}\n"
else
    printf "${RED}${BOLD}FAILED${RESET}  ${total_pass}/${total} passed, ${RED}${total_fail} failed${RESET}\n"
fi

[[ $total_fail -eq 0 ]]
