#! /usr/bin/env python3

import os
import sys
import subprocess


def _find_source_root():
    """Locate the LLTFI source root by reading Project_SOURCE_DIR from CMakeCache.txt."""
    # __file__ is in <build>/test_suite/SCRIPTS/ — go up 3 levels to reach <build>/
    build_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))
    cache_file = os.path.join(build_dir, 'CMakeCache.txt')
    if not os.path.isfile(cache_file):
        return None
    with open(cache_file) as f:
        for line in f:
            if line.startswith('Project_SOURCE_DIR:STATIC='):
                return line.split('=', 1)[1].strip()
    return None


def test_fidl_generation():
    """Run FIDL-Algorithm.py -a default and verify it produces the expected selector files."""
    result_list = []

    source_root = _find_source_root()
    if source_root is None:
        result_list.append({'name': './FIDL/generation', 'result':
            'FAIL: cannot determine LLTFI source root from CMakeCache.txt'})
        return 1, result_list

    fidl_script = os.path.join(source_root, 'tools', 'FIDL', 'FIDL-Algorithm.py')
    if not os.path.isfile(fidl_script):
        result_list.append({'name': './FIDL/generation', 'result':
            f'FAIL: FIDL-Algorithm.py not found at {fidl_script}'})
        return 1, result_list

    # --- Test 1: FIDL runs without error ---
    p = subprocess.run(
        ['python3', fidl_script, '-a', 'default'],
        capture_output=True, text=True, cwd=source_root)
    if p.returncode != 0:
        result_list.append({'name': './FIDL/generation', 'result':
            f'FAIL: FIDL-Algorithm.py exited {p.returncode}: {p.stderr.strip()}'})
        return 1, result_list
    result_list.append({'name': './FIDL/generation', 'result': 'PASS'})

    # --- Test 2: Expected number of selector files generated ---
    sw_dir = os.path.join(source_root, 'llvm_passes', 'software_failures')
    generated = [
        f for f in os.listdir(sw_dir)
        if f.startswith('_') and '_' in f[1:] and f.endswith('Selector.cpp')
    ]
    expected_count = 35  # matches number of modes in default_failures.yaml
    if len(generated) < expected_count:
        result_list.append({'name': './FIDL/selector_count', 'result':
            f'FAIL: expected >= {expected_count} generated selector files, got {len(generated)}'})
        return 1, result_list
    result_list.append({'name': './FIDL/selector_count', 'result': 'PASS'})

    # --- Test 3: injectors.yaml is populated (not empty) ---
    injectors_yaml = os.path.join(source_root, 'tools', 'FIDL', 'config', 'injectors.yaml')
    import yaml
    with open(injectors_yaml) as f:
        injectors = yaml.safe_load(f)
    default_injectors = injectors.get('default', {})
    if len(default_injectors) < expected_count:
        result_list.append({'name': './FIDL/injectors_yaml', 'result':
            f'FAIL: expected >= {expected_count} entries in injectors.yaml, got {len(default_injectors)}'})
        return 1, result_list
    result_list.append({'name': './FIDL/injectors_yaml', 'result': 'PASS'})

    return 0, result_list


if __name__ == '__main__':
    rc, results = test_fidl_generation()
    for r in results:
        print(r['name'], '\t\t', r['result'])
    sys.exit(rc)
