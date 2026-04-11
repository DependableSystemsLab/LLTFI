#! /usr/bin/env python3

"""
Tests for the ONNX/ML analysis tools in tools/:
  - CompareLayerOutputs.py  (requires: onnx, pygraphviz)
  - ExtendONNXModel.py       (requires: onnx)
  - outputONNXGraph.py       (requires: onnx, pydot)

Tests that require a missing dependency are reported as SKIP, not FAIL.
"""

import os
import sys
import json
import subprocess
import tempfile


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


def _make_minimal_onnx(path):
    """Create a minimal single-Relu ONNX model and save to path."""
    import onnx
    from onnx import helper, TensorProto
    X = helper.make_tensor_value_info('X', TensorProto.FLOAT, [1, 3])
    Y = helper.make_tensor_value_info('Y', TensorProto.FLOAT, [1, 3])
    relu = helper.make_node('Relu', inputs=['X'], outputs=['Y'])
    graph = helper.make_graph([relu], 'test_graph', [X], [Y])
    model = helper.make_model(graph, opset_imports=[helper.make_opsetid('', 11)])
    onnx.save(model, path)


def _make_layer_json(layers_data):
    """Build the JSON format expected by CompareLayerOutputs.py."""
    result = {}
    for i, values in enumerate(layers_data):
        result[str(i)] = {
            'Layer Id': str(i),
            'Rank': 1,
            'Number of Elements': len(values),
            'Shape': [len(values)],
            'Data': values,
        }
    return result


def test_ml_tools():
    result_list = []

    source_root = _find_source_root()
    if source_root is None:
        result_list.append({'name': './ml_tools/setup',
            'result': 'FAIL: cannot determine source root from CMakeCache.txt'})
        return 1, result_list

    compare_script = os.path.join(source_root, 'tools', 'CompareLayerOutputs.py')
    extend_script  = os.path.join(source_root, 'tools', 'ExtendONNXModel.py')
    graph_script   = os.path.join(source_root, 'tools', 'outputONNXGraph.py')

    # -----------------------------------------------------------------------
    # CompareLayerOutputs — requires onnx + pygraphviz to import the module
    # -----------------------------------------------------------------------
    if not _has_dep('onnx', 'pygraphviz'):
        for name in ('compare_layer_outputs_match', 'compare_layer_outputs_diff'):
            result_list.append({'name': f'./ml_tools/{name}',
                'result': 'SKIP: onnx or pygraphviz not installed'})
    else:
        with tempfile.TemporaryDirectory() as tmpdir:
            golden_json  = os.path.join(tmpdir, 'golden.json')
            same_json    = os.path.join(tmpdir, 'faulty_same.json')
            diff_json    = os.path.join(tmpdir, 'faulty_diff.json')
            dummy_onnx   = os.path.join(tmpdir, 'dummy.onnx')

            _make_minimal_onnx(dummy_onnx)

            golden_data = [[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]]
            same_data   = [[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]]
            diff_data   = [[1.0, 2.0, 3.0], [4.0, 5.0, 99.0]]  # last value changed

            for path, data in [(golden_json, golden_data),
                               (same_json,   same_data),
                               (diff_json,   diff_data)]:
                with open(path, 'w') as f:
                    json.dump(_make_layer_json(data), f)

            # Test: identical data → no mismatch
            p = subprocess.run(
                ['python3', compare_script, golden_json, same_json, dummy_onnx],
                capture_output=True, text=True)
            if p.returncode != 0 or 'No mismatch found' not in p.stdout:
                result_list.append({'name': './ml_tools/compare_layer_outputs_match',
                    'result': f'FAIL: expected "No mismatch found", got: {p.stdout.strip()[:120]}'})
            else:
                result_list.append({'name': './ml_tools/compare_layer_outputs_match',
                    'result': 'PASS'})

            # Test: differing data → mismatch detected
            p = subprocess.run(
                ['python3', compare_script, golden_json, diff_json, dummy_onnx],
                capture_output=True, text=True)
            if p.returncode != 0 or 'No mismatch found' in p.stdout:
                result_list.append({'name': './ml_tools/compare_layer_outputs_diff',
                    'result': f'FAIL: expected mismatch detection, got: {p.stdout.strip()[:120]}'})
            else:
                result_list.append({'name': './ml_tools/compare_layer_outputs_diff',
                    'result': 'PASS'})

    # -----------------------------------------------------------------------
    # ExtendONNXModel — requires onnx
    # -----------------------------------------------------------------------
    if not _has_dep('onnx'):
        result_list.append({'name': './ml_tools/extend_onnx_model',
            'result': 'SKIP: onnx not installed'})
    else:
        with tempfile.TemporaryDirectory() as tmpdir:
            model_in  = os.path.join(tmpdir, 'model.onnx')
            model_out = os.path.join(tmpdir, 'extended.onnx')
            _make_minimal_onnx(model_in)

            p = subprocess.run(
                ['python3', extend_script,
                 '--model_path', model_in,
                 '--output_model_path', model_out,
                 '--layers', 'all'],
                capture_output=True, text=True)
            if p.returncode != 0 or not os.path.isfile(model_out):
                result_list.append({'name': './ml_tools/extend_onnx_model',
                    'result': f'FAIL: exited {p.returncode}: {p.stderr.strip()[:120]}'})
            else:
                result_list.append({'name': './ml_tools/extend_onnx_model',
                    'result': 'PASS'})

    # -----------------------------------------------------------------------
    # outputONNXGraph — requires onnx + pydot
    # -----------------------------------------------------------------------
    if not _has_dep('onnx', 'pydot'):
        result_list.append({'name': './ml_tools/output_onnx_graph',
            'result': 'SKIP: onnx or pydot not installed'})
    else:
        with tempfile.TemporaryDirectory() as tmpdir:
            model_path = os.path.join(tmpdir, 'model.onnx')
            dot_path   = os.path.join(tmpdir, 'model.dot')
            _make_minimal_onnx(model_path)

            p = subprocess.run(
                ['python3', graph_script, model_path, dot_path],
                capture_output=True, text=True)
            if p.returncode != 0 or not os.path.isfile(dot_path):
                result_list.append({'name': './ml_tools/output_onnx_graph',
                    'result': f'FAIL: exited {p.returncode}: {p.stderr.strip()[:120]}'})
            else:
                result_list.append({'name': './ml_tools/output_onnx_graph',
                    'result': 'PASS'})

    return 0, result_list


if __name__ == '__main__':
    rc, results = test_ml_tools()
    for r in results:
        print(r['name'], '\t\t', r['result'])
    sys.exit(rc)
