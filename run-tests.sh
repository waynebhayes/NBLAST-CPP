#!/bin/bash
set -uo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"

GREEN='\033[0;32m'
RED='\033[0;31m'
BOLD='\033[1m'
RESET='\033[0m'
LINE=$(printf '%.0s─' {1..60})

total_pass=0
total_fail=0

# ── 1. build ──────────────────────────────────────────────────────────────────
printf "${BOLD}Building nblast++...${RESET} "
if build_output=$(make -C "$REPO_ROOT" release 2>&1); then
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

# ── 4. summary ────────────────────────────────────────────────────────────────
total=$((total_pass + total_fail))
printf "\n%s\n" "$LINE"
if [[ $total_fail -eq 0 ]]; then
    printf "${GREEN}${BOLD}ALL PASSED${RESET}  ${total_pass}/${total}\n"
else
    printf "${RED}${BOLD}FAILED${RESET}  ${total_pass}/${total} passed, ${RED}${total_fail} failed${RESET}\n"
fi

[[ $total_fail -eq 0 ]]
