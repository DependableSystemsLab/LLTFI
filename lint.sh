#!/usr/bin/env bash
# lint.sh — Run all LLTFI linters (clang-tidy, clang-format, flake8).
#
# Usage:
#   ./lint.sh              # check only (exit 1 if any issues found)
#   ./lint.sh --fix        # auto-fix clang-format and flake8 issues in-place
#   ./lint.sh --cpp        # C++ checks only
#   ./lint.sh --python     # Python checks only
#   ./lint.sh --install    # install missing Python tools then lint
#
# Requirements:
#   C++:    clang-tidy-20 and clang-format-20 (apt install clang-tidy-20 clang-format-20)
#           compile_commands.json in LLTFI-build/
#           (add -DCMAKE_EXPORT_COMPILE_COMMANDS=ON to cmake to generate it)
#   Python: flake8 and flake8-bugbear (pip install flake8 flake8-bugbear)

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${LLFI_BUILD_ROOT:-${REPO_ROOT}/../LLTFI-build}"
FIX=0
RUN_CPP=1
RUN_PYTHON=1
ERRORS=0

# ---------------------------------------------------------------------------
# Parse arguments
# ---------------------------------------------------------------------------
for arg in "$@"; do
  case "$arg" in
    --fix)     FIX=1 ;;
    --cpp)     RUN_PYTHON=0 ;;
    --python)  RUN_CPP=0 ;;
    --install)
      echo "==> Installing Python lint tools..."
      pip3 install --quiet flake8 flake8-bugbear
      ;;
    --help|-h)
      head -15 "$0" | grep '^#' | sed 's/^# \?//'
      exit 0
      ;;
    *)
      echo "Unknown argument: $arg" >&2
      exit 1
      ;;
  esac
done

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
find_tool() {
  # find_tool <preferred> [fallback...]
  for t in "$@"; do
    if command -v "$t" &>/dev/null; then echo "$t"; return 0; fi
  done
  return 1
}

section() { echo; echo "==> $*"; }
pass()    { echo "    PASS: $*"; }
fail()    { echo "    FAIL: $*"; ERRORS=$((ERRORS + 1)); }
warn()    { echo "    WARN: $* (tool not found — skipping)"; }

# ---------------------------------------------------------------------------
# C++ linting
# ---------------------------------------------------------------------------
if [[ $RUN_CPP -eq 1 ]]; then

  # clang-format
  section "clang-format (C++ formatting)"
  if CFMT=$(find_tool clang-format-20 clang-format); then
    CPP_FILES=$(find "${REPO_ROOT}/llvm_passes" -name '*.cpp' -o -name '*.h' \
      | grep -v 'software_failures/_' )
    FMT_ISSUES=0
    for f in $CPP_FILES; do
      if [[ $FIX -eq 1 ]]; then
        "$CFMT" -i "$f"
      else
        diff_out=$("$CFMT" --dry-run --Werror "$f" 2>&1 || true)
        if [[ -n "$diff_out" ]]; then
          echo "    $f: formatting issues (run lint.sh --fix to auto-fix)"
          FMT_ISSUES=$((FMT_ISSUES + 1))
        fi
      fi
    done
    if [[ $FIX -eq 1 ]]; then
      pass "reformatted all C++ files"
    elif [[ $FMT_ISSUES -eq 0 ]]; then
      pass "all C++ files are correctly formatted"
    else
      fail "$FMT_ISSUES file(s) have formatting issues"
    fi
  else
    warn "clang-format-20 not found (apt install clang-format-20)"
  fi

  # clang-tidy
  section "clang-tidy (C++ static analysis)"
  if CTIDY=$(find_tool clang-tidy-20 clang-tidy); then
    COMPDB="${BUILD_DIR}/compile_commands.json"
    if [[ ! -f "$COMPDB" ]]; then
      warn "compile_commands.json not found at ${COMPDB}." \
           "Rebuild with: cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..."
    else
      TIDY_ISSUES=0
      # Only check hand-written passes; exclude FIDL-generated selectors
      CPP_SOURCES=$(find "${REPO_ROOT}/llvm_passes" -name '*.cpp' \
        | grep -v 'software_failures/_')
      for f in $CPP_SOURCES; do
        result=$("$CTIDY" -p "$BUILD_DIR" --quiet "$f" 2>&1 \
          | grep -v "^[0-9]* warnings generated\.$" || true)
        if [[ -n "$result" ]]; then
          echo "$result"
          TIDY_ISSUES=$((TIDY_ISSUES + 1))
        fi
      done
      if [[ $TIDY_ISSUES -eq 0 ]]; then
        pass "no clang-tidy issues found"
      else
        fail "$TIDY_ISSUES file(s) have clang-tidy issues"
      fi
    fi
  else
    warn "clang-tidy-20 not found (apt install clang-tidy-20)"
  fi

fi

# ---------------------------------------------------------------------------
# Python linting
# ---------------------------------------------------------------------------
if [[ $RUN_PYTHON -eq 1 ]]; then

  section "flake8 (Python style and correctness)"
  if python3 -m flake8 --version &>/dev/null; then
    PYTHON_DIRS=(
      "${REPO_ROOT}/bin"
      "${REPO_ROOT}/test_suite/SCRIPTS"
      "${REPO_ROOT}/tools/FIDL"
      "${REPO_ROOT}/tools/GenerateMakefile"
    )
    # Filter to directories that actually exist
    EXISTING_DIRS=()
    for d in "${PYTHON_DIRS[@]}"; do
      [[ -d "$d" ]] && EXISTING_DIRS+=("$d")
    done

    if [[ ${#EXISTING_DIRS[@]} -gt 0 ]]; then
      if python3 -m flake8 "${EXISTING_DIRS[@]}"; then
        pass "no flake8 issues found"
      else
        fail "flake8 reported issues"
      fi
    fi

    # Check that flake8-bugbear is installed (catches bare except:, shell=True)
    if ! python3 -m flake8 --select=B --quiet /dev/null 2>/dev/null; then
      warn "flake8-bugbear not installed — bare-except and shell=True checks skipped" \
           "(pip install flake8-bugbear)"
    fi
  else
    warn "flake8 not found (pip install flake8 flake8-bugbear)"
  fi

fi

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo
if [[ $ERRORS -eq 0 ]]; then
  echo "==> All lint checks passed."
  exit 0
else
  echo "==> $ERRORS lint check(s) failed."
  exit 1
fi
