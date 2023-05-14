import tensorflow as tf
from transformers import TFAutoModelForMaskedLM, AutoTokenizer
from transformers import AutoTokenizer, AutoModel, AutoModelWithLMHead, BertTokenizer, BertForMaskedLM, RobertaTokenizer, RobertaForMaskedLM
import os, glob, json, pdb, sys
from onnx import numpy_helper
from onnxruntime import InferenceSession
import numpy as np

inputs = ["if (x is not None) <mask> (x>1)",
"public String getSecretKey ( String subdomain ) { String secretKey = secretKeys . get ( subdomain ) ; if (secretKey == <mask>) { secretKey = defaultSecretKey ; } return secretKey ; }",
"protected Iterator < Map . Entry < K , V > > createEntrySetIterator ( ) { if ( size ( ) <mask> 0 ) { return EmptyIterator . INSTANCE ; } return new EntrySetIterator < K , V > ( this ) ; }",
"public static boolean isPermanent ( ResourceModel resourceModel ) { Object resource = resourceModel . getResource ( ) ; try { return ( Boolean ) <mask> ; } catch ( ClassCastException e ) { return false ; } }",
"public void clear ( ) { modCount ++ ; HashEntry [ ] data = this . data ; for ( int i = data . length - 1 ; i <mask> 0 ; i -- ) { data [ i ] = null ; } size = 0 ; }",
"public void addPrincipal ( String principal ) { if ( ! readOnly <mask> ! principals . contains ( principal ) ) { principals . add ( principal ) ; principalsModified = true ; } }",
"protected final void addAllApplications ( Set < Class < ? > > set ) { for ( Class < ? > cls : set ) { if ( ! cls . isInterface ( ) && ! Modifier . isAbstract ( cls . getModifiers ( ) ) ) { if ( ! this . classmap . <mask> ( cls ) ) { this . classNames . add ( cls . getName ( ) ) ; } } } }",
"public void setName ( String name ) { if ( name != null && name . <mask> ( this . name ) ) { return ; } this . name = name ; Roster packet = new Roster ( ) ; packet . setType ( IQ . Type . set ) ; packet . addItem ( new JID ( user ) , name , ask , subscription , getGroupNames ( ) ) ; connection . sendPacket ( packet ) ; }",
"""public String getString ( String defaultValue ) { if ( value instanceof String || value instanceof Number ) { return value . toString ( ) ; } if ( value == null ) { return null ; } if ( value instanceof JSONArray ) { return ( ( JSONArray ) value ) . toJSONString ( ) ; } if ( value instanceof JSONObject ) { return ( ( JSONObject ) value ) . <mask> ( ) ; } if ( value == null ) { return defaultValue ; } throw createException ( "Expected string:" ) ; }""",
""" public Double getDouble ( Double defaultValue ) { if ( value instanceof Number ) { return ( ( Number ) value ) . doubleValue ( ) ; } if ( value instanceof String ) { String s = ( String ) value ; return Double . <mask> ( s ) ; } if ( value == null ) { return defaultValue ; } throw createException ( "Expected number:" ) ; }"""]

def lltfi_sort(elem):                                                           
    return int(elem.split('layeroutput')[-1].split('-')[-1].split('.txt')[0])

def main(inpSample):

    global inputs

    # Get model's tokenizer and convert input.
    model_name = "microsoft/codebert-base-mlm"
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
    outputs = ""
    i = 0
    for filename in txtfiles:
        resforSingleInput = []
        shapeForSingleInput = []
        with open(filename, "r") as read_file:
            resultJson = json.load(read_file)

            for key, value in resultJson.items():
                resforSingleInput.append(value['Data'])
                shapeForSingleInput.append(value['Shape'])

            outputs += f"\n-------- Run: {i} -----------\n"
            modelOp = resforSingleInput[0]
            modelOpShape = shapeForSingleInput[0]

            npArr =  np.array(modelOp)
            npArr = np.reshape(npArr, modelOpShape[1:])
            maskedTokenLogits = npArr[mask_token_index.numpy()]
            top_5_tokens = tf.math.top_k(maskedTokenLogits, 5).indices.numpy()
            i = i + 1
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
