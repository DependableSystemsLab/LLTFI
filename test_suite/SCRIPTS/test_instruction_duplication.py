#!/usr/bin/env python3

"""
Tests for the Selective Instruction Duplication (SID) pass
(llvm_passes/instruction_duplication/InstructionDuplication.cpp).

The pass is compiled to SEDPasses.so and invoked via opt with the legacy pass
manager.  All tests write temporary LLVM IR, run opt, and inspect the output.

Tests are reported as SKIP when SEDPasses.so has not been built yet.

Run standalone:  python3 test_instruction_duplication.py
Via test driver: llfi_test --all_ml
"""

import os
import re
import subprocess
import sys
import tempfile


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _find_build_dir():
    """Return the CMake build root (two levels above this script's directory)."""
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


def _find_opt():
    """Find the opt binary: prefer the LLVM install used by the build."""
    build_dir = _find_build_dir()
    config_dir = os.path.join(build_dir, 'config')
    if os.path.isdir(config_dir) and config_dir not in sys.path:
        sys.path.insert(0, config_dir)
    try:
        import llvm_paths
        candidate = os.path.join(llvm_paths.LLVM_DST_ROOT, 'bin', 'opt')
        if os.path.isfile(candidate) and os.access(candidate, os.X_OK):
            return candidate
    except (ImportError, AttributeError):
        pass
    import shutil
    return shutil.which('opt')


def _find_sed_so():
    """Return path to SEDPasses.so in the build tree, or None."""
    build_dir = _find_build_dir()
    candidate = os.path.join(build_dir, 'llvm_passes',
                             'instruction_duplication', 'SEDPasses.so')
    return candidate if os.path.isfile(candidate) else None


def _run_pass(opt, sed_so, ir_text, extra_flags=None, tmpdir=None):
    """
    Write ir_text to a temp file, run InstructionDuplicationPass via opt,
    and return (returncode, stdout_text).

    extra_flags is a list of additional opt flags (e.g. ['--enableChainDuplication']).
    """
    ir_path = os.path.join(tmpdir, 'input.ll')
    out_path = os.path.join(tmpdir, 'output.ll')
    with open(ir_path, 'w') as f:
        f.write(ir_text)

    cmd = [
        opt,
        '-load', sed_so,
        '--InstructionDuplicationPass',
        '--enable-new-pm=0',
        '-S', ir_path,
        '-o', out_path,
    ]
    if extra_flags:
        cmd.extend(extra_flags)

    p = subprocess.run(cmd, capture_output=True, text=True)
    output = ''
    if os.path.isfile(out_path):
        with open(out_path) as f:
            output = f.read()
    return p.returncode, output


# ---------------------------------------------------------------------------
# Minimal LLVM IR fixtures
# Note: 1986948931 == "conv" operator ID; 119251066446157 == "matmul".
# OMInstrumentPoint(id, 2) marks the start of an operator region;
# OMInstrumentPoint(id, 1) marks the end.
# ---------------------------------------------------------------------------

_IR_INSIDE_BOUNDARY = """\
; ModuleID = 'test_inside'
target triple = "x86_64-unknown-linux-gnu"

declare void @OMInstrumentPoint(i64, i64)

define float @main_graph(float %a, float %b) {
entry:
  call void @OMInstrumentPoint(i64 1986948931, i64 2)
  %r = fadd float %a, %b
  call void @OMInstrumentPoint(i64 1986948931, i64 1)
  ret float %r
}
"""

_IR_OUTSIDE_BOUNDARY = """\
; ModuleID = 'test_outside'
target triple = "x86_64-unknown-linux-gnu"

define float @main_graph(float %a, float %b) {
entry:
  %r = fadd float %a, %b
  ret float %r
}
"""

_IR_CHAIN = """\
; ModuleID = 'test_chain'
target triple = "x86_64-unknown-linux-gnu"

declare void @OMInstrumentPoint(i64, i64)

define float @main_graph(float %a, float %b, float %c) {
entry:
  call void @OMInstrumentPoint(i64 1986948931, i64 2)
  %r1 = fadd float %a, %b
  %r2 = fmul float %r1, %c
  call void @OMInstrumentPoint(i64 1986948931, i64 1)
  ret float %r2
}
"""

_IR_WRONG_OPERATOR = """\
; ModuleID = 'test_filter'
target triple = "x86_64-unknown-linux-gnu"

declare void @OMInstrumentPoint(i64, i64)

define float @main_graph(float %a, float %b) {
entry:
  call void @OMInstrumentPoint(i64 119251066446157, i64 2)
  %r = fadd float %a, %b
  call void @OMInstrumentPoint(i64 119251066446157, i64 1)
  ret float %r
}
"""

_IR_MULTIPLE_OPS = """\
; ModuleID = 'test_multi'
target triple = "x86_64-unknown-linux-gnu"

declare void @OMInstrumentPoint(i64, i64)

define float @main_graph(float %a, float %b, float %c) {
entry:
  call void @OMInstrumentPoint(i64 1986948931, i64 2)
  %r1 = fadd float %a, %b
  call void @OMInstrumentPoint(i64 1986948931, i64 1)
  call void @OMInstrumentPoint(i64 119251066446157, i64 2)
  %r2 = fmul float %r1, %c
  call void @OMInstrumentPoint(i64 119251066446157, i64 1)
  ret float %r2
}
"""


# ---------------------------------------------------------------------------
# Individual test functions
# ---------------------------------------------------------------------------

def _test_smoke(opt, sed_so):
    """Pass runs on valid IR without error."""
    prefix = './instruction_duplication/smoke'
    with tempfile.TemporaryDirectory() as tmpdir:
        rc, _ = _run_pass(opt, sed_so, _IR_INSIDE_BOUNDARY, tmpdir=tmpdir)
    if rc != 0:
        return [{'name': prefix, 'result': f'FAIL: opt exited {rc}'}]
    return [{'name': prefix, 'result': 'PASS'}]


def _test_instrumentation_inserted(opt, sed_so):
    """compareFloatValues call is injected for an fadd inside an operator boundary."""
    prefix = './instruction_duplication/instrumentation_inserted'
    with tempfile.TemporaryDirectory() as tmpdir:
        rc, output = _run_pass(opt, sed_so, _IR_INSIDE_BOUNDARY, tmpdir=tmpdir)
    if rc != 0:
        return [{'name': prefix, 'result': f'FAIL: opt exited {rc}'}]
    if not re.search(r'call float @compareFloatValues', output):
        return [{'name': prefix,
                 'result': 'FAIL: compareFloatValues call not found in output IR'}]
    return [{'name': prefix, 'result': 'PASS'}]


def _test_instruction_duplicated(opt, sed_so):
    """The original arithmetic instruction is duplicated in the output IR."""
    prefix = './instruction_duplication/instruction_duplicated'
    with tempfile.TemporaryDirectory() as tmpdir:
        rc, output = _run_pass(opt, sed_so, _IR_INSIDE_BOUNDARY, tmpdir=tmpdir)
    if rc != 0:
        return [{'name': prefix, 'result': f'FAIL: opt exited {rc}'}]
    # Two fadd instructions should appear (original + duplicate)
    fadds = re.findall(r'fadd float', output)
    if len(fadds) < 2:
        return [{'name': prefix,
                 'result': f'FAIL: expected >= 2 fadd instructions, found {len(fadds)}'}]
    return [{'name': prefix, 'result': 'PASS'}]


def _test_no_duplication_outside_boundary(opt, sed_so):
    """Arithmetic outside any OMInstrumentPoint boundary is not duplicated."""
    prefix = './instruction_duplication/no_duplication_outside_boundary'
    with tempfile.TemporaryDirectory() as tmpdir:
        rc, output = _run_pass(opt, sed_so, _IR_OUTSIDE_BOUNDARY, tmpdir=tmpdir)
    if rc != 0:
        return [{'name': prefix, 'result': f'FAIL: opt exited {rc}'}]
    if re.search(r'call float @compareFloatValues', output):
        return [{'name': prefix,
                 'result': 'FAIL: compareFloatValues unexpectedly found in output IR'}]
    fadds = re.findall(r'fadd float', output)
    if len(fadds) != 1:
        return [{'name': prefix,
                 'result': f'FAIL: expected exactly 1 fadd, found {len(fadds)}'}]
    return [{'name': prefix, 'result': 'PASS'}]


def _test_chain_duplication(opt, sed_so):
    """With --enableChainDuplication, a consecutive arithmetic chain is duplicated."""
    prefix = './instruction_duplication/chain_duplication'
    with tempfile.TemporaryDirectory() as tmpdir:
        rc, output = _run_pass(opt, sed_so, _IR_CHAIN,
                               extra_flags=['--enableChainDuplication'],
                               tmpdir=tmpdir)
    if rc != 0:
        return [{'name': prefix, 'result': f'FAIL: opt exited {rc}'}]
    if not re.search(r'call float @compareFloatValues', output):
        return [{'name': prefix,
                 'result': 'FAIL: compareFloatValues call not found in chain output IR'}]
    # Both fadd and fmul should be duplicated
    fadds = re.findall(r'fadd float', output)
    fmuls = re.findall(r'fmul float', output)
    if len(fadds) < 2:
        return [{'name': prefix,
                 'result': f'FAIL: expected >= 2 fadd in chain, found {len(fadds)}'}]
    if len(fmuls) < 2:
        return [{'name': prefix,
                 'result': f'FAIL: expected >= 2 fmul in chain, found {len(fmuls)}'}]
    return [{'name': prefix, 'result': 'PASS'}]


def _test_operator_filtering(opt, sed_so):
    """With --operatorName=conv, a matmul operator region is not instrumented."""
    prefix = './instruction_duplication/operator_filtering'
    with tempfile.TemporaryDirectory() as tmpdir:
        rc, output = _run_pass(opt, sed_so, _IR_WRONG_OPERATOR,
                               extra_flags=['--operatorName=conv'],
                               tmpdir=tmpdir)
    if rc != 0:
        return [{'name': prefix, 'result': f'FAIL: opt exited {rc}'}]
    if re.search(r'call float @compareFloatValues', output):
        return [{'name': prefix,
                 'result': 'FAIL: compareFloatValues found despite operator mismatch'}]
    return [{'name': prefix, 'result': 'PASS'}]


def _test_multiple_operator_regions(opt, sed_so):
    """With --operatorName=conv, only the conv region is instrumented, not matmul."""
    prefix = './instruction_duplication/multiple_operator_regions'
    with tempfile.TemporaryDirectory() as tmpdir:
        rc, output = _run_pass(opt, sed_so, _IR_MULTIPLE_OPS,
                               extra_flags=['--operatorName=conv'],
                               tmpdir=tmpdir)
    if rc != 0:
        return [{'name': prefix, 'result': f'FAIL: opt exited {rc}'}]
    if not re.search(r'call float @compareFloatValues', output):
        return [{'name': prefix,
                 'result': 'FAIL: compareFloatValues not found for conv region'}]
    # Only the fadd (conv region) should be duplicated, not the fmul (matmul region)
    fmuls = re.findall(r'fmul float', output)
    if len(fmuls) != 1:
        return [{'name': prefix,
                 'result': f'FAIL: fmul in matmul region was duplicated (found {len(fmuls)})'}]
    return [{'name': prefix, 'result': 'PASS'}]


# ---------------------------------------------------------------------------
# Top-level entry point
# ---------------------------------------------------------------------------

def test_instruction_duplication():
    """
    Run all InstructionDuplication pass tests.

    Returns (returncode, result_list) where returncode is 0 on success.
    """
    result_list = []
    skip_msg = 'SKIP: SEDPasses.so not found — build LLTFI first'

    sed_so = _find_sed_so()
    if not sed_so:
        for name in ('smoke', 'instrumentation_inserted', 'instruction_duplicated',
                     'no_duplication_outside_boundary', 'chain_duplication',
                     'operator_filtering', 'multiple_operator_regions'):
            result_list.append({
                'name': f'./instruction_duplication/{name}',
                'result': skip_msg,
            })
        return 0, result_list

    opt = _find_opt()
    if not opt:
        for name in ('smoke', 'instrumentation_inserted', 'instruction_duplicated',
                     'no_duplication_outside_boundary', 'chain_duplication',
                     'operator_filtering', 'multiple_operator_regions'):
            result_list.append({
                'name': f'./instruction_duplication/{name}',
                'result': 'SKIP: opt binary not found',
            })
        return 0, result_list

    result_list.extend(_test_smoke(opt, sed_so))
    result_list.extend(_test_instrumentation_inserted(opt, sed_so))
    result_list.extend(_test_instruction_duplicated(opt, sed_so))
    result_list.extend(_test_no_duplication_outside_boundary(opt, sed_so))
    result_list.extend(_test_chain_duplication(opt, sed_so))
    result_list.extend(_test_operator_filtering(opt, sed_so))
    result_list.extend(_test_multiple_operator_regions(opt, sed_so))

    has_fail = any(r['result'].startswith('FAIL') for r in result_list)
    return (1 if has_fail else 0), result_list


if __name__ == '__main__':
    rc, results = test_instruction_duplication()
    for r in results:
        print(r['name'], '\t\t', r['result'])
    sys.exit(rc)
