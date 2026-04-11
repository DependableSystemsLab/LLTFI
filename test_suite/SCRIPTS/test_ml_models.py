#! /usr/bin/env python3

"""
Tests for the TensorFlow and PyTorch model compilation and fault injection
pipelines in LLTFI.

Tests are organised in four tiers; each tier skips when its dependencies are
absent so the suite always finishes cleanly.

  Tier 1 — TensorFlow → ONNX
      requires: tensorflow, tf2onnx, onnx
  Tier 2 — PyTorch → ONNX
      requires: torch, onnx
  Tier 3 — ONNX → LLVM IR   (onnx-mlir / mlir-translate)
      requires: onnx-mlir binary, mlir-translate binary
      uses:     pre-built model.onnx from the mnist sample dir (if present)
                OR a model produced by tier 1/2
  Tier 4 — Fault injection on compiled ML model
      requires: LLTFI build, tier-3 output (model.ll + image input)

Run with: python3 test_ml_models.py
Or via the test driver: llfi_test --all_ml
"""

import os
import sys
import shutil
import subprocess
import tempfile


# ---------------------------------------------------------------------------
# Helpers shared by all tiers
# ---------------------------------------------------------------------------

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


def _has_dep(*modules):
    for m in modules:
        try:
            __import__(m)
        except ImportError:
            return False
    return True


def _find_binary(name):
    """Search ONNX_MLIR_BUILD/bin, LLVM_DST_ROOT/bin, then PATH."""
    for env_var in ('ONNX_MLIR_BUILD', 'LLVM_DST_ROOT'):
        base = os.environ.get(env_var)
        if base:
            p = os.path.join(base, 'bin', name)
            if os.path.isfile(p) and os.access(p, os.X_OK):
                return p
    return shutil.which(name)


def _import_llvm_paths():
    config_dir = os.path.join(_find_build_dir(), 'config')
    if config_dir not in sys.path:
        sys.path.insert(0, config_dir)
    import llvm_paths
    return llvm_paths


# ---------------------------------------------------------------------------
# Tier 1: TensorFlow → ONNX
# ---------------------------------------------------------------------------

def _train_tf_model(saved_model_dir):
    """Train a minimal single Dense-layer TF model on random data (1 epoch)."""
    import numpy as np
    import tensorflow as tf

    x = np.random.rand(32, 28, 28).astype('float32')
    y = np.random.randint(0, 10, 32)

    model = tf.keras.Sequential([
        tf.keras.layers.Flatten(input_shape=(28, 28)),
        tf.keras.layers.Dense(10, activation='softmax'),
    ])
    model.compile(optimizer='adam',
                  loss='sparse_categorical_crossentropy',
                  metrics=['accuracy'])
    model.fit(x, y, epochs=1, verbose=0)
    model.save(saved_model_dir)


def test_tensorflow_pipeline():
    result_list = []
    prefix = './ml/tensorflow'

    if not _has_dep('tensorflow', 'tf2onnx', 'onnx'):
        for name in ('train', 'tf_to_onnx', 'onnx_valid'):
            result_list.append({'name': f'{prefix}/{name}',
                'result': 'SKIP: tensorflow, tf2onnx, or onnx not installed'})
        return result_list

    with tempfile.TemporaryDirectory() as tmpdir:
        saved_model_dir = os.path.join(tmpdir, 'model.tf')
        onnx_path       = os.path.join(tmpdir, 'model.onnx')

        # --- Test 1: model trains without error ---
        try:
            _train_tf_model(saved_model_dir)
            if not os.path.isdir(saved_model_dir):
                raise RuntimeError('SavedModel directory not created')
            result_list.append({'name': f'{prefix}/train', 'result': 'PASS'})
        except Exception as e:
            result_list.append({'name': f'{prefix}/train',
                'result': f'FAIL: {e}'})
            return result_list

        # --- Test 2: convert SavedModel → ONNX ---
        p = subprocess.run(
            [sys.executable, '-m', 'tf2onnx.convert',
             '--saved-model', saved_model_dir,
             '--output', onnx_path],
            capture_output=True, text=True)
        if p.returncode != 0 or not os.path.isfile(onnx_path):
            result_list.append({'name': f'{prefix}/tf_to_onnx',
                'result': f'FAIL: tf2onnx exited {p.returncode}: {p.stderr.strip()[:200]}'})
            return result_list
        result_list.append({'name': f'{prefix}/tf_to_onnx', 'result': 'PASS'})

        # --- Test 3: ONNX model is structurally valid ---
        try:
            import onnx
            model = onnx.load(onnx_path)
            onnx.checker.check_model(model)
            result_list.append({'name': f'{prefix}/onnx_valid', 'result': 'PASS'})
        except Exception as e:
            result_list.append({'name': f'{prefix}/onnx_valid',
                'result': f'FAIL: {e}'})

    return result_list


# ---------------------------------------------------------------------------
# Tier 2: PyTorch → ONNX
# ---------------------------------------------------------------------------

def _export_pytorch_model(onnx_path):
    """Export a minimal untrained PyTorch model to ONNX."""
    import torch
    import torch.nn as nn

    class TinyNet(nn.Module):
        def __init__(self):
            super().__init__()
            self.fc = nn.Linear(784, 10)

        def forward(self, x):
            return torch.softmax(self.fc(x), dim=1)

    model = TinyNet()
    dummy = torch.randn(1, 784)
    torch.onnx.export(model, dummy, onnx_path,
                      input_names=['input'], output_names=['output'],
                      opset_version=11, verbose=False)


def test_pytorch_pipeline():
    result_list = []
    prefix = './ml/pytorch'

    if not _has_dep('torch', 'onnx'):
        for name in ('export_onnx', 'onnx_valid'):
            result_list.append({'name': f'{prefix}/{name}',
                'result': 'SKIP: torch or onnx not installed'})
        return result_list

    with tempfile.TemporaryDirectory() as tmpdir:
        onnx_path = os.path.join(tmpdir, 'model.onnx')

        # --- Test 1: export to ONNX ---
        try:
            _export_pytorch_model(onnx_path)
            if not os.path.isfile(onnx_path):
                raise RuntimeError('ONNX file not created')
            result_list.append({'name': f'{prefix}/export_onnx', 'result': 'PASS'})
        except Exception as e:
            result_list.append({'name': f'{prefix}/export_onnx',
                'result': f'FAIL: {e}'})
            return result_list

        # --- Test 2: ONNX model is structurally valid ---
        try:
            import onnx
            model = onnx.load(onnx_path)
            onnx.checker.check_model(model)
            result_list.append({'name': f'{prefix}/onnx_valid', 'result': 'PASS'})
        except Exception as e:
            result_list.append({'name': f'{prefix}/onnx_valid',
                'result': f'FAIL: {e}'})

    return result_list


# ---------------------------------------------------------------------------
# Tier 3: ONNX → LLVM IR via onnx-mlir + mlir-translate
# ---------------------------------------------------------------------------

def _find_prebuilt_onnx(source_root):
    """Return path to pre-built model.onnx in the mnist sample dir, or None."""
    candidate = os.path.join(
        source_root, 'sample_programs', 'ml_sample_programs',
        'vision_models', 'mnist', 'model.onnx')
    return candidate if os.path.isfile(candidate) else None


def test_onnx_to_ir(onnx_path=None):
    """
    Compile an ONNX model to LLVM IR using onnx-mlir and mlir-translate.

    If onnx_path is None, tries to find the pre-built model.onnx from the
    mnist sample directory.  Skips if onnx-mlir or mlir-translate are not on
    PATH or in ONNX_MLIR_BUILD/bin.
    """
    result_list = []
    prefix = './ml/onnx_to_ir'

    onnx_mlir    = _find_binary('onnx-mlir')
    mlir_translate = _find_binary('mlir-translate')

    if not onnx_mlir or not mlir_translate:
        missing = []
        if not onnx_mlir:
            missing.append('onnx-mlir')
        if not mlir_translate:
            missing.append('mlir-translate')
        for name in ('compile_mlir', 'translate_to_ll'):
            result_list.append({'name': f'{prefix}/{name}',
                'result': f'SKIP: {", ".join(missing)} not found '
                          f'(set ONNX_MLIR_BUILD or add to PATH)'})
        return result_list

    source_root = _find_source_root()
    if onnx_path is None and source_root:
        onnx_path = _find_prebuilt_onnx(source_root)

    if onnx_path is None or not os.path.isfile(onnx_path):
        for name in ('compile_mlir', 'translate_to_ll'):
            result_list.append({'name': f'{prefix}/{name}',
                'result': 'SKIP: no ONNX model available '
                          '(run compile.sh in sample_programs/ml_sample_programs/'
                          'vision_models/mnist/ first, or install tensorflow/torch)'})
        return result_list

    with tempfile.TemporaryDirectory() as tmpdir:
        # Copy ONNX to tmpdir so onnx-mlir outputs land there cleanly
        local_onnx = os.path.join(tmpdir, 'model.onnx')
        shutil.copy2(onnx_path, local_onnx)
        mlir_out = os.path.join(tmpdir, 'model.onnx.mlir')
        ll_out   = os.path.join(tmpdir, 'model.mlir.ll')

        # --- Test 1: onnx-mlir compiles ONNX → MLIR ---
        p = subprocess.run(
            [onnx_mlir, '--EmitLLVMIR', local_onnx],
            capture_output=True, text=True, cwd=tmpdir)
        if p.returncode != 0 or not os.path.isfile(mlir_out):
            result_list.append({'name': f'{prefix}/compile_mlir',
                'result': f'FAIL: onnx-mlir exited {p.returncode}: {p.stderr.strip()[:200]}'})
            return result_list
        result_list.append({'name': f'{prefix}/compile_mlir', 'result': 'PASS'})

        # --- Test 2: mlir-translate produces LLVM IR ---
        with open(ll_out, 'w') as ll_file:
            p = subprocess.run(
                [mlir_translate, '-mlir-to-llvmir', mlir_out],
                stdout=ll_file, stderr=subprocess.PIPE, text=True, cwd=tmpdir)
        if p.returncode != 0 or not os.path.isfile(ll_out) or \
                os.path.getsize(ll_out) == 0:
            result_list.append({'name': f'{prefix}/translate_to_ll',
                'result': f'FAIL: mlir-translate exited {p.returncode}: {p.stderr.strip()[:200]}'})
        else:
            result_list.append({'name': f'{prefix}/translate_to_ll', 'result': 'PASS'})

    return result_list


# ---------------------------------------------------------------------------
# Tier 4: Fault injection on a compiled ML model
# ---------------------------------------------------------------------------

def test_fault_injection():
    """
    Smoke-test the LLTFI fault injection pipeline on a compiled ML model.

    Requires:
      - model.ll and eight.png in sample_programs/.../mnist/
        (produced by compile.sh — run that first if this test SKIPs)
      - LLTFI build with instrument, profile, injectfault scripts
    """
    result_list = []
    prefix = './ml/fault_injection'

    source_root = _find_source_root()
    if source_root is None:
        result_list.append({'name': f'{prefix}/instrument',
            'result': 'FAIL: cannot determine source root from CMakeCache.txt'})
        return result_list

    mnist_dir = os.path.join(source_root, 'sample_programs',
                             'ml_sample_programs', 'vision_models', 'mnist')
    model_ll  = os.path.join(mnist_dir, 'model.ll')
    image_png = os.path.join(mnist_dir, 'eight.png')
    op_seq    = os.path.join(mnist_dir, 'expected_op_seq.txt')

    if not os.path.isfile(model_ll):
        for name in ('instrument', 'profile', 'inject'):
            result_list.append({'name': f'{prefix}/{name}',
                'result': 'SKIP: model.ll not found — run compile.sh in '
                          'sample_programs/ml_sample_programs/vision_models/mnist/ first'})
        return result_list

    try:
        llvm_paths = _import_llvm_paths()
    except ImportError as e:
        result_list.append({'name': f'{prefix}/instrument',
            'result': f'FAIL: cannot import llvm_paths — {e}'})
        return result_list

    build_dir      = _find_build_dir()
    instrument_bin = os.path.join(build_dir, 'bin', 'instrument')
    profile_bin    = os.path.join(build_dir, 'bin', 'profile')
    inject_bin     = os.path.join(build_dir, 'bin', 'injectfault')

    for label, path in [('instrument', instrument_bin),
                        ('profile',    profile_bin),
                        ('injectfault', inject_bin)]:
        if not os.path.isfile(path):
            result_list.append({'name': f'{prefix}/instrument',
                'result': f'FAIL: {label} not found at {path}'})
            return result_list

    with tempfile.TemporaryDirectory() as tmpdir:
        # Copy model and support files into a clean workdir so llfi/ output
        # lands in tmpdir and does not pollute the source tree.
        local_ll  = os.path.join(tmpdir, 'model.ll')
        local_img = os.path.join(tmpdir, 'eight.png')
        shutil.copy2(model_ll, local_ll)
        if os.path.isfile(image_png):
            shutil.copy2(image_png, local_img)
        # Copy input.yaml so instrument knows FI config
        local_yaml = os.path.join(tmpdir, 'input.yaml')
        shutil.copy2(os.path.join(mnist_dir, 'input.yaml'), local_yaml)

        # Read expected_op_seq if available (second arg to profile/inject)
        extra_arg = ''
        if os.path.isfile(op_seq):
            with open(op_seq) as f:
                extra_arg = f.read().strip()

        # --- Test 1: instrument ---
        p = subprocess.run(
            [instrument_bin, '--readable', local_ll],
            capture_output=True, text=True, cwd=tmpdir)
        llfi_dir = os.path.join(tmpdir, 'llfi')
        if p.returncode != 0 or not os.path.isdir(llfi_dir):
            result_list.append({'name': f'{prefix}/instrument',
                'result': f'FAIL: instrument exited {p.returncode}: {p.stderr.strip()[:200]}'})
            return result_list
        result_list.append({'name': f'{prefix}/instrument', 'result': 'PASS'})

        # --- Test 2: profile ---
        profiling_exe = os.path.join(llfi_dir, 'model-profiling.exe')
        if not os.path.isfile(profiling_exe):
            result_list.append({'name': f'{prefix}/profile',
                'result': f'FAIL: profiling exe not found at {profiling_exe}'})
            return result_list

        profile_cmd = [profile_bin, profiling_exe]
        if os.path.isfile(local_img):
            profile_cmd.append(local_img)
        if extra_arg:
            profile_cmd.append(extra_arg)
        p = subprocess.run(profile_cmd, capture_output=True, text=True, cwd=tmpdir)
        if p.returncode != 0:
            result_list.append({'name': f'{prefix}/profile',
                'result': f'FAIL: profile exited {p.returncode}: {p.stderr.strip()[:200]}'})
            return result_list
        result_list.append({'name': f'{prefix}/profile', 'result': 'PASS'})

        # --- Test 3: inject (1 run to verify the pipeline works) ---
        inject_exe = os.path.join(llfi_dir, 'model-faultinjection.exe')
        if not os.path.isfile(inject_exe):
            result_list.append({'name': f'{prefix}/inject',
                'result': f'FAIL: fault injection exe not found at {inject_exe}'})
            return result_list

        inject_cmd = [inject_bin, inject_exe]
        if os.path.isfile(local_img):
            inject_cmd.append(local_img)
        if extra_arg:
            inject_cmd.append(extra_arg)
        p = subprocess.run(inject_cmd, capture_output=True, text=True, cwd=tmpdir)
        # A non-zero exit from the injected binary is normal (crash = fault effect)
        stat_dir = os.path.join(llfi_dir, 'llfi_stat_output')
        if not os.path.isdir(stat_dir):
            result_list.append({'name': f'{prefix}/inject',
                'result': f'FAIL: no llfi_stat_output directory produced'})
        else:
            result_list.append({'name': f'{prefix}/inject', 'result': 'PASS'})

    return result_list


# ---------------------------------------------------------------------------
# Top-level entry point
# ---------------------------------------------------------------------------

def test_ml_models():
    """Run all four tiers and return a combined result list."""
    all_results = []
    all_results.extend(test_tensorflow_pipeline())
    all_results.extend(test_pytorch_pipeline())
    all_results.extend(test_onnx_to_ir())
    all_results.extend(test_fault_injection())

    has_fail = any(r['result'].startswith('FAIL') for r in all_results)
    return (1 if has_fail else 0), all_results


if __name__ == '__main__':
    rc, results = test_ml_models()
    for r in results:
        print(r['name'], '\t\t', r['result'])
    sys.exit(rc)
