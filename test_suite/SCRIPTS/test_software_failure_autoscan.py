#! /usr/bin/env python3

"""
Tests that SoftwareFailureAutoScan.py correctly scans an LLVM IR file,
produces the applicable-failures output file, and finds at least one
injectable failure mode.  Requires only the LLTFI build (no ML deps).
"""

import os
import sys
import subprocess
import tempfile


# Minimal C program with fopen/fread/fclose — enough to trigger WrongAPI
# and several other file-I/O failure selectors.
_MINIMAL_C = r"""
#include <stdio.h>
int main(void) {
    FILE *f = fopen("dummy.txt", "r");
    char buf[32];
    if (f) {
        fread(buf, 1, 32, f);
        fclose(f);
    }
    return 0;
}
"""


def _find_build_dir():
    return os.path.dirname(os.path.dirname(os.path.dirname(
        os.path.realpath(__file__))))


def _find_source_root():
    cache_file = os.path.join(_find_build_dir(), 'CMakeCache.txt')
    if not os.path.isfile(cache_file):
        return None
    with open(cache_file) as f:
        for line in f:
            if line.startswith('Project_SOURCE_DIR:STATIC='):
                return line.split('=', 1)[1].strip()
    return None


def _import_llvm_paths():
    config_dir = os.path.join(_find_build_dir(), 'config')
    if config_dir not in sys.path:
        sys.path.insert(0, config_dir)
    import llvm_paths
    return llvm_paths


def test_software_failure_autoscan():
    result_list = []

    build_dir = _find_build_dir()

    try:
        llvm_paths = _import_llvm_paths()
    except ImportError as e:
        result_list.append({'name': './SoftwareFailureAutoScan/scan',
            'result': f'FAIL: cannot import llvm_paths — {e}'})
        return 1, result_list

    clang = os.path.join(llvm_paths.LLVM_GXX_BIN_DIR, 'clang')
    # Use the BUILD copy of the script — it has correct relative paths to
    # llfi-passes.so and config/llvm_paths.py in the build tree.
    # Note: the build installs it without the .py extension.
    autoscan = os.path.join(build_dir, 'bin', 'SoftwareFailureAutoScan')

    for label, path in [('clang', clang), ('SoftwareFailureAutoScan.py', autoscan)]:
        if not os.path.isfile(path):
            result_list.append({'name': './SoftwareFailureAutoScan/scan',
                'result': f'FAIL: {label} not found at {path}'})
            return 1, result_list

    with tempfile.TemporaryDirectory() as tmpdir:
        c_src   = os.path.join(tmpdir, 'scan_test.c')
        ll_file = os.path.join(tmpdir, 'scan_test.ll')
        output_file = os.path.join(tmpdir, 'llfi.applicable.software.failures.txt')

        with open(c_src, 'w') as f:
            f.write(_MINIMAL_C)

        # --- Test 1: compile C → LLVM IR ---
        p = subprocess.run(
            [clang, '-S', '-emit-llvm', '-O0', c_src, '-o', ll_file],
            capture_output=True, encoding='utf-8', errors='replace')
        if p.returncode != 0 or not os.path.isfile(ll_file):
            result_list.append({'name': './SoftwareFailureAutoScan/compile',
                'result': f'FAIL: clang returned {p.returncode}: {p.stderr.strip()}'})
            return 1, result_list
        result_list.append({'name': './SoftwareFailureAutoScan/compile', 'result': 'PASS'})

        # --- Test 2: autoscan runs without error ---
        p = subprocess.run(
            ['python3', autoscan, '--no_input_yaml', ll_file],
            capture_output=True, encoding='utf-8', errors='replace')
        if p.returncode != 0:
            result_list.append({'name': './SoftwareFailureAutoScan/scan',
                'result': f'FAIL: exited {p.returncode}: {p.stderr.strip()}'})
            return 1, result_list
        result_list.append({'name': './SoftwareFailureAutoScan/scan', 'result': 'PASS'})

        # --- Test 3: output file is generated ---
        if not os.path.isfile(output_file):
            result_list.append({'name': './SoftwareFailureAutoScan/output_file',
                'result': 'FAIL: llfi.applicable.software.failures.txt not generated'})
            return 1, result_list
        result_list.append({'name': './SoftwareFailureAutoScan/output_file', 'result': 'PASS'})

        # --- Test 4: at least one failure mode found ---
        with open(output_file) as f:
            content = f.read().strip()
        if not content:
            result_list.append({'name': './SoftwareFailureAutoScan/finds_modes',
                'result': 'FAIL: output file is empty — no failure modes found'})
            return 1, result_list
        result_list.append({'name': './SoftwareFailureAutoScan/finds_modes', 'result': 'PASS'})

    return 0, result_list


if __name__ == '__main__':
    rc, results = test_software_failure_autoscan()
    for r in results:
        print(r['name'], '\t\t', r['result'])
    sys.exit(rc)
