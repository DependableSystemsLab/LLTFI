import os
import sys
import argparse
import onnx
from onnx import numpy_helper
from collections import OrderedDict

parser = argparse.ArgumentParser()
parser.add_argument('--model_path', type=str, required=True, help="Path to the ONNX model")
parser.add_argument('--output_model_path', type=str, required=True, help="Path to the extended ONNX model")
parser.add_argument('--layers', type=str, default="all", help="""Intermediate layers for which output has to be extracted. Semicolon seperated. like: --layers="conv;maxpool" """)
parser.add_argument('--summary', default=False, action="store_true", help="Just return the summary of the model")
args = parser.parse_args()

layers = []
all_layers = False

def extend_model_output(model, intermediate_outputs):
    global layers
    # onnx-mlir doesn't care about manually specified output types & shapes.
    DUMMY_TENSOR_TYPE = onnx.TensorProto.FLOAT

    original_op = model.graph.output[0].name

    # Remove all outputs from the model
    while (len(model.graph.output)):
        model.graph.output.pop()

    i = -1;
    layer_output = []
    for output_name in intermediate_outputs:
        i = i + 1 # Current layer
        if all_layers and ('Constant' not in output_name) and ('Gather' not in output_name):
            output_value_info = onnx.helper.make_tensor_value_info(output_name, DUMMY_TENSOR_TYPE, None)
            model.graph.output.extend([output_value_info])
            layer_output.append(i)
        else:
            temp = [n for n in layers if n in output_name]
            if len(temp) > 0:
                output_value_info = onnx.helper.make_tensor_value_info(output_name, DUMMY_TENSOR_TYPE, None)
                model.graph.output.extend([output_value_info])
                layer_output.append(i)


    if (not all_layers) and ( (len(intermediate_outputs) - 1) not in layer_output):
        # Add original output
        output_value_info = onnx.helper.make_tensor_value_info(original_op, DUMMY_TENSOR_TYPE, None)
        model.graph.output.extend([output_value_info])
        layer_output.append(len(intermediate_outputs) - 1)

    return model, layer_output

def print_summary(model):
    print(str([[n for n in node.output if n != ''] for node in model.graph.node]))

def main():

    global layers
    global all_layers
    layers = (args.layers).split(';')

    if 'all' in layers:
        all_layers = True

    # Load the onnx model.
    model = onnx.load(args.model_path)

    if args.summary:
        print_summary(model)
        exit()

    output_names = [o.name for o in model.graph.output]
    output_names = list(OrderedDict.fromkeys(output_names))
    output_names = sum([[n for n in node.output if n != ''] for node in model.graph.node], [])
    output_names = list(OrderedDict.fromkeys(output_names))
    model, layer_output = extend_model_output(model, output_names)
    onnx.save(model, args.output_model_path)

    print(str(layer_output).strip('[').strip(']').replace(' ', ''))

if __name__ == '__main__':
    main()

