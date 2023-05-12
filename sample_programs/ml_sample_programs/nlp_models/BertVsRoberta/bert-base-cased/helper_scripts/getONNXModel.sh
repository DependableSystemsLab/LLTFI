#!/bin/bash

python3 -m transformers.onnx --model=bert-base-cased --feature=masked-lm onnx_model/
cp onnx_model/model.onnx .
rm -r onnx_model/
