from transformers import AutoTokenizer
import os
from onnx import numpy_helper

inputs = ["Main symptom of common flu is [MASK].",
"Main symptom of cancer is [MASK].",
"He is having a cough and fever, thus, he might be suffering from [MASK].",
"He is having sickness and vomiting, thus, he might be having [MASK].",
"Since you are having post-traumatic disorder, you should consult a [MASK].",
"Paracetamol is used to treat a [MASK].",
"[MASK] is a tumor suppressor gene.",
"Chest pain is a symptom of [MASK]",
"He is having chest pain and vomiting, thus he might be having [MASK]",
"[MASK] is a symptom of diabetes."]

model_name = "microsoft/BiomedNLP-PubMedBERT-base-uncased-abstract-fulltext"
tokenizer = AutoTokenizer.from_pretrained(model_name)

for i in range(0, len(inputs)):
    input_txt = inputs[i]
    sequence = (input_txt)
    inputs_np = tokenizer(sequence, return_tensors="np")
    tensor_input_ids = numpy_helper.from_array(inputs_np['input_ids'])
    tensor_token_type_ids = numpy_helper.from_array(inputs_np['token_type_ids'])
    tensor_attention_mask = numpy_helper.from_array(inputs_np['attention_mask'])

    with open(os.path.join("", f"input{i}_0.pb"), "wb") as f:
        f.write(tensor_input_ids.SerializeToString())

    with open(os.path.join("", f"input{i}_1.pb"), "wb") as f:
        f.write(tensor_attention_mask.SerializeToString())

    with open(os.path.join("", f"input{i}_2.pb"), "wb") as f:
        f.write(tensor_token_type_ids.SerializeToString())
