import tensorflow as tf
from transformers import TFAutoModelForMaskedLM, AutoTokenizer
from transformers import AutoTokenizer, AutoModel, AutoModelWithLMHead, BertTokenizer, RobertaForMaskedLM
import os, glob, json, pdb, sys
from onnx import numpy_helper
from onnxruntime import InferenceSession
import numpy as np

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

def lltfi_sort(elem):                                                           
    return int(elem.split('layeroutput')[-1].split('-')[-1].split('.txt')[0])

def main(inpSample):

    global inputs

    # Get model's tokenizer and convert input.
    model_name = "roberta-base"
    model = RobertaForMaskedLM.from_pretrained(model_name)
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

    for file in sorted(glob.glob(os.path.join(PROG_OUT, "*.txt")), key=lltfi_sort):
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
