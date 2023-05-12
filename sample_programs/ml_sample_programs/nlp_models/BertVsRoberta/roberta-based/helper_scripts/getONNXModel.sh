#!/bin/bash

python3 -m transformers.onnx --model=roberta-base --feature=masked-lm onnx_model/
cp onnx_model/model.onnx .
rm -r onnx_model/
