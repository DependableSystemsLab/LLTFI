#!/bin/bash

python3 -m transformers.onnx --model=microsoft/BiomedNLP-PubMedBERT-base-uncased-abstract-fulltext --feature=masked-lm onnx_model/
cp onnx_model/model.onnx .
rm -r onnx_model/
