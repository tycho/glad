#!/usr/bin/env python3
"""
Collect GLAD test suite results and write a JUnit XML report.

Reads the test manifest written by gen_ninja.py, inspects the per-step
.meta and .log files, prints a summary, and (optionally) writes a JUnit
XML report to TEST_REPORT.

Exit code is non-zero if any test failed.
"""

import json
import os
import sys
from xml.sax.saxutils import escape


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def read_meta(path: str) -> dict:
    """
    Parse a .meta file written by a step script.
    Returns {'exit_code': int|None, 'duration': int}.
    """
    result = {'exit_code': None, 'duration': 0}
    try:
        with open(path) as fh:
            for line in fh:
                key, _, val = line.strip().partition('=')
                if key == 'exit_code':
                    result['exit_code'] = int(val)
                elif key == 'duration':
                    result['duration'] = int(val)
    except (FileNotFoundError, ValueError):
        pass
    return result


def read_log(path: str) -> str:
    try:
        with open(path) as fh:
            return fh.read()
    except FileNotFoundError:
        return ''


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    test_tmp            = os.environ.get('TEST_TMP', 'build')
    test_report         = os.environ.get('TEST_REPORT', 'test-report.xml')
    report_enabled      = os.environ.get('TEST_REPORT_ENABLED', '1') == '1'
    print_message       = os.environ.get('PRINT_MESSAGE', '0') == '1'

    manifest_path = os.path.join(test_tmp, 'tests.json')
    try:
        with open(manifest_path) as fh:
            manifest = json.load(fh)
    except FileNotFoundError:
        print(f'error: test manifest not found at {manifest_path}', file=sys.stderr)
        sys.exit(1)

    tests = manifest['tests']
    total  = len(tests)
    failed = 0

    # Each entry: (test_name, class, duration, did_fail, combined_output)
    report_cases: list[dict] = []

    for test in tests:
        name     = test['name']
        steps    = test['steps']
        test_dir = test['dir']

        test_failed    = False
        total_duration = 0
        output_parts: list[str] = []

        for step in steps:
            meta = read_meta(os.path.join(test_dir, f'{step}.meta'))
            log  = read_log(os.path.join(test_dir, f'{step}.log'))

            total_duration += meta['duration']

            if meta['exit_code'] is None or meta['exit_code'] != 0:
                test_failed = True

            if log:
                output_parts.append(f'=== {step.upper()} ===\n{log}')

        if test_failed:
            failed += 1

        # Derive JUnit classname / test name from the path components.
        # e.g. "c/run/gl/default/001" → classname="c", name="run/gl/default/001"
        parts = name.split('/')
        junit_class = parts[0] if parts else name
        junit_name  = '/'.join(parts[1:]) if len(parts) > 1 else name

        combined_output = '\n'.join(output_parts)

        report_cases.append({
            'name':   junit_name,
            'class':  junit_class,
            'time':   total_duration,
            'failed': test_failed,
            'output': combined_output,
        })

        if test_failed and print_message:
            print(f'\nFAIL: {name}')
            if combined_output:
                print(combined_output)

    # -----------------------------------------------------------------------
    # JUnit XML
    # -----------------------------------------------------------------------

    if report_enabled:
        lines = [
            '<?xml version="1.0" encoding="UTF-8"?>',
            f'<testsuite tests="{total}" failures="{failed}">',
        ]
        for case in report_cases:
            lines.append(
                f'    <testcase name="{escape(case["name"])}"'
                f' classname="{escape(case["class"])}"'
                f' time="{case["time"]}">'
            )
            if case['failed']:
                lines.append('        <failure />')
            if case['output']:
                lines.append(f'        <system-out>{escape(case["output"])}</system-out>')
            lines.append('    </testcase>')
        lines.append('</testsuite>')

        with open(test_report, 'w') as fh:
            fh.write('\n'.join(lines) + '\n')

    # -----------------------------------------------------------------------
    # Summary
    # -----------------------------------------------------------------------

    ran = sum(1 for t in tests
              if any(read_meta(os.path.join(t['dir'], f'{s}.meta'))['exit_code'] is not None
                     for s in t['steps']))

    print(f'\nTotal tests: {total}, Tests ran: {ran}, Tests failed: {failed}')

    if failed > 0:
        sys.exit(1)


if __name__ == '__main__':
    main()
