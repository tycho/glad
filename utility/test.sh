#!/bin/bash
#
# GLAD test suite runner.
# Generates a Ninja build file, runs all tests in parallel, and collects results.
#
# Environment variables (all optional):
#   EXIT_ON_FAILURE    Stop after first failure (default: 0)
#   PRINT_MESSAGE      Print output of failed tests to stdout (default: 0)
#   PYTHON             Python interpreter to use (default: python3)
#   GLAD_ARGS          Extra arguments for glad invocations (default: --reproducible)
#   GLAD               Full glad invocation (default: $PYTHON -m glad $GLAD_ARGS)
#   GCC                C compiler command + flags
#   MINGW_GCC          MinGW cross-compiler command + flags
#   WINE               Wine command (default: wine)
#   TEST_TMP           Build/scratch directory (default: build)
#   TEST_DIRECTORY     Root directory to search for tests (default: test)
#   TEST_PATTERN       Filename glob pattern for test files (default: test.*)
#   TEST_REPORT        Output path for JUnit XML report (default: test-report.xml)
#   TEST_REPORT_ENABLED  Whether to write the XML report (default: 1)
#   JOBS               Parallelism level (default: number of CPU cores)
#   CLEAN              Cleans temporary path for test runs before running (default: 0)
#
# Note: The TESTS variable from the old runner is no longer supported.
# Use TEST_DIRECTORY and TEST_PATTERN to restrict which tests are run.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

EXIT_ON_FAILURE=${EXIT_ON_FAILURE:=0}
export PRINT_MESSAGE=${PRINT_MESSAGE:=0}

export PYTHON=${PYTHON:="python3"}
export GLAD_ARGS=${GLAD_ARGS:="--reproducible"}

_GCC=${_GCC:="gcc"}
_MINGW_GCC=${_MINGW_GCC:="x86_64-w64-mingw32-gcc"}
_GCC_FLAGS="-Wall -Wextra -Werror -Wsign-conversion -Wcast-qual -Wstrict-prototypes -Wno-unknown-pragmas -ansi"

export GLAD=${GLAD:="$PYTHON -m glad ${GLAD_ARGS}"}
export GCC=${GCC:="$_GCC $_GCC_FLAGS"}
export MINGW_GCC=${MINGW_GCC:="$_MINGW_GCC $_GCC_FLAGS"}
export WINE=${WINE:="wine"}

export TEST_TMP
TEST_TMP=$(realpath "${TEST_TMP:="build"}")
export TEST_DIRECTORY=${TEST_DIRECTORY:="test"}
export TEST_PATTERN=${TEST_PATTERN:="test.*"}
export TEST_REPORT=${TEST_REPORT:="test-report.xml"}
export TEST_REPORT_ENABLED=${TEST_REPORT_ENABLED:=1}

# ---------------------------------------------------------------------------
# Parallelism
# ---------------------------------------------------------------------------

get_nproc() {
    nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4
}

JOBS=${JOBS:=$(get_nproc)}

if [ "$EXIT_ON_FAILURE" -eq 1 ]; then
    NINJA_KEEP=1
else
    NINJA_KEEP=0
fi

# ---------------------------------------------------------------------------
# Dependency checks
# ---------------------------------------------------------------------------

if ! command -v ninja &>/dev/null; then
    echo "error: 'ninja' not found in PATH" >&2
    exit 1
fi

if ! "$PYTHON" -c '' &>/dev/null; then
    echo "error: Python interpreter '$PYTHON' not found" >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# Stage 1: Generate Ninja build file
# ---------------------------------------------------------------------------

if [ "${CLEAN:-0}" -eq 1 ]; then
    echo "Cleaning $TEST_TMP..."
    rm -rf "$TEST_TMP"
fi

echo "Generating build.ninja..."
"$PYTHON" "$SCRIPT_DIR/test/gen_ninja.py"

# ---------------------------------------------------------------------------
# Stage 2: Run tests via Ninja
# ---------------------------------------------------------------------------

echo "Running tests (jobs=$JOBS)..."
NINJA_EXIT=0
ninja -f "$TEST_TMP/build.ninja" -k "$NINJA_KEEP" -j "$JOBS" || NINJA_EXIT=$?

# ---------------------------------------------------------------------------
# Stage 3: Collect results and write report
# ---------------------------------------------------------------------------

COLLECT_EXIT=0
"$PYTHON" "$SCRIPT_DIR/test/collect.py" || COLLECT_EXIT=$?

if [ "$NINJA_EXIT" -ne 0 ] || [ "$COLLECT_EXIT" -ne 0 ]; then
    exit 1
fi

# All tests passed — remove stamps so next run is clean,
# but preserve logs and meta for review.
find "$TEST_TMP" -name '*.stamp' -delete

exit 0
