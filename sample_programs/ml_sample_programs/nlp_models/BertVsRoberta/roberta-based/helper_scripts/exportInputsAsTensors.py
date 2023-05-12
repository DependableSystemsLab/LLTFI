from transformers import AutoTokenizer
import os
from onnx import numpy_helper

inputs = ["Main symptom of common flu is <mask>.",
"Main symptom of cancer is <mask>.",
"He is having a cough and fever, thus, he might be suffering from <mask>.",
"He is having sickness and vomiting, thus, he might be having <mask>.",
"Since you are having post-traumatic disorder, you should consult a <mask>.",
"Paracetamol is used to treat a <mask>.",
"The hereditary <mask> protein, HFE, specifically regulates transferrin-mediated iron uptake in HeLa cells.",
"Pelizaeus-Merzbacher disease is caused by overexpression of <mask> gene transcripts.",
"<mask> is a tumor suppressor gene.",
"<mask> is a symptom of diabetes."]

model_name = "roberta-base"
tokenizer = AutoTokenizer.from_pretrained(model_name)

for i in range(0, len(inputs)):
    input_txt = inputs[i]
    sequence = (input_txt)
    inputs_np = tokenizer(sequence, return_tensors="np")
    tensor_input_ids = numpy_helper.from_array(inputs_np['input_ids'])
    tensor_attention_mask = numpy_helper.from_array(inputs_np['attention_mask'])

    with open(os.path.join("", f"input{i}_0.pb"), "wb") as f:
        f.write(tensor_input_ids.SerializeToString())

    with open(os.path.join("", f"input{i}_1.pb"), "wb") as f:
        f.write(tensor_attention_mask.SerializeToString())
