import tensorflow as tf
from transformers import TFAutoModelForMaskedLM, AutoTokenizer
from transformers import AutoTokenizer, AutoModel, AutoModelWithLMHead, BertTokenizer, BertForMaskedLM
import os, glob, json, pdb, sys
from onnx import numpy_helper
from onnxruntime import InferenceSession
import numpy as np

inputs = ["Main symptom of common flu is [MASK].",
"Main symptom of cancer is [MASK].",
"He is having a cough and fever, thus, he might be suffering from [MASK].",
"He is having sickness and vomiting, thus, he might be having [MASK].",
"Since you are having post-traumatic disorder, you should consult a [MASK].",
"Paracetamol is used to treat a [MASK].",
"The hereditary [MASK] protein, HFE, specifically regulates transferrin-mediated iron uptake in HeLa cells.",
"Pelizaeus-Merzbacher disease is caused by overexpression of [MASK] gene transcripts.",
"[MASK] is a tumor suppressor gene.",
"[MASK] is a symptom of diabetes."]

def main(inpSample):

    global inputs

    # Get model's tokenizer and convert input.
    model_name = "microsoft/BiomedNLP-PubMedBERT-base-uncased-abstract-fulltext"
    model = BertForMaskedLM.from_pretrained(model_name)
    tokenizer = AutoTokenizer.from_pretrained(model_name)

    sequence = (inputs[inpSample])
    inputs = tokenizer(sequence, return_tensors="pt")
    inputs_np = tokenizer(sequence, return_tensors="np")
    inputs_tf = tokenizer(sequence, return_tensors="tf")
    mask_token_index = tf.where(inputs_tf["input_ids"] == tokenizer.mask_token_id)[0, 1]

    #Path to LLTFI layer output
    ROOT = os.getcwd()
    LLFI_OUT = os.path.join(ROOT, 'llfi')
    PROG_OUT = os.path.join(LLFI_OUT, 'prog_output')
    OUT = os.path.join(ROOT, 'out')
    pathOutput = os.path.join(OUT, 'onnx-pred')
    filePred = os.path.join(pathOutput, 'onnx-pred.txt')

    # Read LLTFI output from llfi/prog_output and add it to 'listResArr'
    txtfiles = []

    for file in glob.glob(os.path.join(PROG_OUT, "*.txt")):
        txtfiles.append(file)

    listResArr = []
    listShareApp = []
    for filename in txtfiles:
        resforSingleInput = []
        shapeForSingleInput = []
        with open(filename, "r") as read_file:
            resultJson = json.load(read_file)

            for key, value in resultJson.items():
                resforSingleInput.append(value['Data'])
                shapeForSingleInput.append(value['Shape'])
            listResArr.append(resforSingleInput)
            listShareApp.append(shapeForSingleInput)

    outputs = ""
    for i in range(0, len(listResArr)):
        outputs += f"\n-------- Run: {i} -----------\n"
        modelOp = listResArr[i][0]
        modelOpShape = listShareApp[i][0]

        npArr =  np.array(modelOp)
        npArr = np.reshape(npArr, modelOpShape[1:])
        maskedTokenLogits = npArr[mask_token_index.numpy()]
        top_5_tokens = tf.math.top_k(maskedTokenLogits, 5).indices.numpy()
        for token in top_5_tokens:
            outputs += str(tokenizer.decode([token])) + "\n"

    if not os.path.isdir(pathOutput):
        os.mkdir(pathOutput)

    # If file exists, remove it.
    if os.path.exists(filePred):
        os.remove(filePred)

    with open(filePred, "a") as write_file:
        write_file.write(outputs)



if __name__ == "__main__":
    inpSample = int(sys.argv[1])
    main(inpSample)
