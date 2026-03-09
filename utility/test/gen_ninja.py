#!/usr/bin/env python3
"""
Generate a Ninja build file for the GLAD test suite.

Reads test files from TEST_DIRECTORY, expands directive variables at
generation time, writes per-step shell scripts, and emits build.ninja
into TEST_TMP.

Each test file may contain directives in comments of the form:
    GLAD:    <command>
    COMPILE: <command>
    RUN:     <command>  (optional)

Variable references in directives ($VAR or ${VAR}):
    $tmp       Per-test scratch directory
    $test      Absolute path to the test file
    $test_dir  Directory containing the test file
    $repo_root Path to the root of the git repository
    All other references are resolved from the environment.
"""

import fnmatch
import json
import os
import re
import stat
import sys
from pathlib import Path


# ---------------------------------------------------------------------------
# Test discovery
# ---------------------------------------------------------------------------

def find_tests(test_dir: str, pattern: str) -> list[str]:
    """
    Walk test_dir and return sorted absolute paths of all files whose
    name case-insensitively matches the glob pattern.
    """
    pattern_lower = pattern.lower()
    results = []
    for root, dirs, files in os.walk(test_dir):
        dirs.sort()
        for name in sorted(files):
            if fnmatch.fnmatch(name.lower(), pattern_lower):
                results.append(os.path.join(root, name))
    return results


# ---------------------------------------------------------------------------
# Directive extraction
# ---------------------------------------------------------------------------

# Matches "KEY: value" appearing anywhere on a line (e.g. inside a comment).
_DIRECTIVE_RE = re.compile(r'(?:^|[\s*#/])([A-Z]+):\s+(.*)')

def extract_directive(test_file: str, key: str) -> str | None:
    """Return the value of the first matching directive, or None."""
    with open(test_file, encoding='utf-8', errors='replace') as fh:
        for line in fh:
            m = _DIRECTIVE_RE.search(line)
            if m and m.group(1) == key:
                return m.group(2).strip()
    return None


# ---------------------------------------------------------------------------
# Variable expansion
# ---------------------------------------------------------------------------

_VAR_RE = re.compile(r'\$\{(\w+)\}|\$(\w+)')

def expand_vars(s: str, vars_dict: dict[str, str]) -> str:
    """
    Expand $VAR and ${VAR} references using vars_dict.
    Unknown references are left as-is (matching bash's behaviour for
    undefined variables would silently drop them, but leaving them is
    safer for debugging).
    """
    def replacer(m: re.Match) -> str:
        name = m.group(1) or m.group(2)
        return vars_dict.get(name, m.group(0))
    return _VAR_RE.sub(replacer, s)


# ---------------------------------------------------------------------------
# Step script generation
# ---------------------------------------------------------------------------

_STEP_SCRIPT_TEMPLATE = """\
#!/bin/bash
# Generated step script — do not edit.
# Step: {step_name}
# Test: {test_name}
STAMP="$1"
LOG="{log}"
META="{meta}"

mkdir -p "{step_dir}"
cd "{step_dir}"
export PYTHONPATH="{repo_root}${{PYTHONPATH:+:$PYTHONPATH}}"

START=$(date +%s)
(
{command}
) > "$LOG" 2>&1
EXIT_CODE=$?
END=$(date +%s)

printf 'exit_code=%d\\nduration=%d\\n' "$EXIT_CODE" "$((END - START))" > "$META"

if [ "$EXIT_CODE" -eq 0 ]; then
    touch "$STAMP"
fi

exit "$EXIT_CODE"
"""

def write_step_script(path: str, step_name: str, test_name: str,
                      command: str, step_dir: str) -> None:
    content = _STEP_SCRIPT_TEMPLATE.format(
        step_name=step_name,
        test_name=test_name,
        log=os.path.join(step_dir, f'{step_name}.log'),
        meta=os.path.join(step_dir, f'{step_name}.meta'),
        step_dir=step_dir,
        repo_root=os.getcwd(),
        command=command,
    )
    with open(path, 'w') as fh:
        fh.write(content)
    # Make executable
    current = os.stat(path).st_mode
    os.chmod(path, current | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)


# ---------------------------------------------------------------------------
# Ninja escaping
# ---------------------------------------------------------------------------

def ninja_path(p: str) -> str:
    """Escape a path for use in a Ninja build file."""
    return p.replace('$', '$$').replace(' ', '$ ').replace(':', '$:')


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def require_env(key: str, default: str | None = None) -> str:
    val = os.environ.get(key, default)
    if val is None:
        print(f'error: environment variable {key} is not set', file=sys.stderr)
        sys.exit(1)
    return val


def main() -> None:
    test_tmp       = require_env('TEST_TMP')
    test_directory = require_env('TEST_DIRECTORY', 'test')
    test_pattern   = require_env('TEST_PATTERN', 'test.*')

    # Resolve the test directory relative to CWD (where test.sh was invoked).
    test_dir_abs = str(Path(test_directory).resolve())

    test_files = find_tests(test_dir_abs, test_pattern)
    if not test_files:
        print(f'error: no tests found in {test_directory!r} matching {test_pattern!r}',
              file=sys.stderr)
        sys.exit(1)

    # Base env dict used for variable expansion in all directives.
    base_env = dict(os.environ)

    ninja_lines: list[str] = []
    ninja_lines += [
        '# Generated by utility/test/gen_ninja.py — do not edit.',
        'ninja_required_version = 1.5',
        '',
        'rule step',
        '  command = bash $script $out',
        '  description = $desc',
        '',
    ]

    terminal_stamps: list[str] = []
    tests_manifest: list[dict] = []

    os.makedirs(test_tmp, exist_ok=True)

    for test_file in test_files:
        test_file_abs = str(Path(test_file).resolve())
        test_file_dir = str(Path(test_file_abs).parent)

        # Derive a clean test name, e.g. "c/run/gl/default/001".
        try:
            rel = Path(test_file_abs).relative_to(test_dir_abs)
        except ValueError:
            rel = Path(test_file)
        test_name = str(rel.parent)   # drop the filename component

        per_test_dir = os.path.join(test_tmp, test_name)

        # Build the variable substitution dict for this test.
        test_vars = {
            **base_env,
            'tmp':       per_test_dir,
            'test':      test_file_abs,
            'test_dir':  test_file_dir,
            'repo_root': os.getcwd(),
        }

        # Extract and expand directives.
        glad_raw    = extract_directive(test_file, 'GLAD')
        compile_raw = extract_directive(test_file, 'COMPILE')
        run_raw     = extract_directive(test_file, 'RUN')

        if not glad_raw or not compile_raw:
            print(f'warning: {test_file} is missing GLAD or COMPILE directive; skipping',
                  file=sys.stderr)
            continue

        glad_cmd    = expand_vars(glad_raw,    test_vars)
        compile_cmd = expand_vars(compile_raw, test_vars)
        run_cmd     = expand_vars(run_raw,     test_vars) if run_raw else None

        os.makedirs(per_test_dir, exist_ok=True)

        # Emit build statements for each step, chained via order-only deps.
        steps: list[str] = []
        prev_stamp: str | None = None

        for step_name, cmd in [('glad', glad_cmd),
                                ('compile', compile_cmd),
                                ('run', run_cmd)]:
            if cmd is None:
                continue

            script_path = os.path.join(per_test_dir, f'{step_name}.sh')
            stamp_path  = os.path.join(per_test_dir, f'{step_name}.stamp')

            write_step_script(script_path, step_name, test_name, cmd, per_test_dir)

            escaped_stamp  = ninja_path(stamp_path)
            escaped_script = ninja_path(script_path)

            dep_clause = f' | {ninja_path(prev_stamp)}' if prev_stamp else ''
            ninja_lines += [
                f'build {escaped_stamp}: step{dep_clause}',
                f'  script = {escaped_script}',
                f'  desc = [{test_name}] {step_name.upper()}',
                '',
            ]

            steps.append(step_name)
            prev_stamp = stamp_path

        if prev_stamp is None:
            continue

        terminal_stamps.append(prev_stamp)
        tests_manifest.append({
            'name':  test_name,
            'steps': steps,
            'dir':   per_test_dir,
        })

    if not terminal_stamps:
        print('error: no valid tests found', file=sys.stderr)
        sys.exit(1)

    # Top-level 'all' phony target.
    stamps_escaped = ' '.join(ninja_path(s) for s in terminal_stamps)
    ninja_lines += [
        f'build all: phony {stamps_escaped}',
        'default all',
    ]

    # Write the Ninja file.
    ninja_path_out = os.path.join(test_tmp, 'build.ninja')
    with open(ninja_path_out, 'w') as fh:
        fh.write('\n'.join(ninja_lines) + '\n')

    # Write the test manifest consumed by collect.py.
    manifest_path = os.path.join(test_tmp, 'tests.json')
    with open(manifest_path, 'w') as fh:
        json.dump({'tests': tests_manifest}, fh, indent=2)

    print(f'Generated {ninja_path_out} ({len(tests_manifest)} tests)')


if __name__ == '__main__':
    main()
