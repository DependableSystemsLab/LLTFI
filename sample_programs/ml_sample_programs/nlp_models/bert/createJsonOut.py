import sys
import os
import glob
import json
from bert_v2 import tokenization
from bert_v2 import run_squad
from functools import partial
import shutil
import collections


ROOT = os.getcwd()
SQUAD_DIR = os.path.join(ROOT, 'squad-1.1')
OUT = os.path.join(ROOT, 'out')
BERT_BASE_DIR = os.path.join(ROOT, 'uncased_L-12_H-768_A-12')
LLFI_OUT = os.path.join(ROOT, 'llfi')
PROG_OUT = os.path.join(LLFI_OUT, 'prog_output')

# define some constants used by the model
EVAL_BATCH_SIZE = 8
MAX_SEQ_LENGTH = 256
N_BEST_SIZE = 20
MAX_ANSWER_LENGTH = 30
MAX_QUERY_LENGTH = 64
DOC_STRIDE = 128
VOCAB_FILE = os.path.join(BERT_BASE_DIR, 'vocab.txt')

def lltfi_sort(elem):                                                           
    return int(elem.split('layeroutput')[-1].split('-')[-1].split('.txt')[0])

def append_feature(eval_features, eval_writer, feature):
    eval_features.append(feature)
    eval_writer.process_feature(feature)

def main(inpSample):
    tokenizer = tokenization.FullTokenizer(vocab_file=VOCAB_FILE, do_lower_case=True)

    eval_examples = run_squad.read_squad_examples(input_file=os.path.join(SQUAD_DIR, "dev-v1.1.json"), is_training=False)
    eval_writer = run_squad.FeatureWriter(filename=os.path.join(OUT, "eval.tf_record"), is_training=False)
    eval_features = []


    run_squad.convert_examples_to_features(
        examples=eval_examples,
        tokenizer=tokenizer,
        max_seq_length=MAX_SEQ_LENGTH,
        doc_stride=DOC_STRIDE,
        max_query_length=MAX_QUERY_LENGTH,
        is_training=False,
        output_fn=partial(append_feature, eval_features, eval_writer))
    eval_writer.close()

    # Read LLTFI output from llfi/prog_output and add it to 'listResArr'
    txtfiles = []

    for file in sorted(glob.glob(os.path.join(PROG_OUT, "*.txt")), key=lltfi_sort):
        txtfiles.append(file)

    listResArr = []
    for filename in txtfiles:
      resforSingleInput = []
      with open(filename, "r") as read_file:
          resultJson = json.load(read_file)

          for key, value in resultJson.items():
              resforSingleInput.append(value['Data'])
          listResArr.append(resforSingleInput)

    RawResult = collections.namedtuple("RawResult", ["unique_id", "start_logits", "end_logits"])
    pathOutput = os.path.join(OUT, 'onnx-pred')
    pathnBestOutput = os.path.join(OUT, 'onnx-nbest-pred')

    # Reference outputs
    refOutFile = f"ref_outputs_json/pred_inp_{str(inpSample)}/onnx_predictions.json"
    with open(refOutFile, "r") as read_file:
          refOut = json.load(read_file)

    #refOutNbestFile = f"ref_outputs_json/pred_inp_{str(inpSample)}/onnx_nbest_predictions.json"
    #with open(refOutNbestFile, "r") as read_file:
          #refOutNbest = json.load(read_file)

    # field 'unique_id'
    if inpSample == 8:
        uniqueId = 1000000000 + inpSample + 2
    elif inpSample == 9:
        uniqueId = 1000000000 + inpSample + 2
    else:
        uniqueId = 1000000000 + inpSample

    # Get output from 'listResArr' array and convert to text
    for index in range(len(listResArr)):
      all_results = []
      all_results.append(RawResult(unique_id=uniqueId, start_logits=listResArr[index][1], end_logits=listResArr[index][0]))

      out_predictions_json = f"onnx_predictions{index}.json"
      out_nbestpredictions_json = f"onnx_nbest_predictions{index}.json"
      out_null_odds_json = f"onnx_null_odds{index}.json"
      run_squad.write_predictions(eval_examples[:1], eval_features[:1], all_results,
                                  N_BEST_SIZE, MAX_ANSWER_LENGTH, True,
                                  os.path.join(OUT, out_predictions_json),
                                  os.path.join(OUT, out_nbestpredictions_json), 
                                  os.path.join(OUT, out_null_odds_json))

      # Compare the generated outputs(final prediction and N best predictions) with reference outputs
      with open(os.path.join(OUT, out_predictions_json), "r") as read_file:
        out = json.load(read_file)

      if out != refOut:
          print(f"FI in run number {index} led to incorrect output")
      else:
          print(f"FI in run number {index} led to correct output")

      #with open(os.path.join(OUT, out_nbestpredictions_json), "r") as read_file:
        #outNbest = json.load(read_file)

      #if outNbest != refOutNbest:
          #print(f"FI in run number {index} led to incorrect output(N best)")


      # Save the text output
      if not os.path.isdir(pathOutput):
        os.mkdir(pathOutput)
      if not os.path.isdir(pathnBestOutput):
        os.mkdir(pathnBestOutput)

      ONNX_PRED = os.path.join(OUT, 'onnx-pred')
      ONNX_NBEST_PRED = os.path.join(OUT, 'onnx-nbest-pred')

      shutil.move(os.path.join(OUT, out_predictions_json), os.path.join(ONNX_PRED, out_predictions_json))
      shutil.move(os.path.join(OUT, out_nbestpredictions_json), os.path.join(ONNX_NBEST_PRED, out_nbestpredictions_json))

if __name__ == "__main__":
    inpSample = int(sys.argv[1])
    main(inpSample)
