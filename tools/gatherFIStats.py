# Python script to gather FI stats
# Python 3.8
# Author: Udit Agarwal
# This script will take path to results folder as input and will print the FI stats.

import argparse, os
import json
import warnings
import sys, struct, math
from pdb import set_trace
from collections import Counter
import fasttext, torch

##### Globals #####

calculate_embedding_distance = False

class FIRun:

    # Model Input
    model_input = -1
    # FI run
    fi_run = -1
    # FI bit
    fi_bit = -1
    # FI layer
    fi_layer = -1
    # FI oldValue
    fi_old_val = -1
    # FI newValue
    fi_new_val = -1
    # FI Cycle
    fi_cycle = -1
    # FI opcode
    fi_opcode = ""
    # FI layer name
    fi_layer_name = ""

    def __init__(self, _model_input, _fi_run, _fi_bit, _fi_layer, _fi_old_val, _fi_new_val, _fi_cycle, _fi_opcode, _fi_layer_name):
        self.fi_run = _fi_run
        self.fi_bit = _fi_bit
        self.fi_layer = _fi_layer
        self.fi_old_val = _fi_old_val
        self.fi_new_val = _fi_new_val
        self.fi_cycle = _fi_cycle
        self.fi_opcode = _fi_opcode
        self.fi_layer_name = _fi_layer_name
        self.model_input = _model_input


class NLPModel:

    # Model Name
    model_name = ""
    # Model input number
    model_input_num = ""
    # Model correct pred
    model_correct_pred = ""
    # Model incorrect pred
    model_all_pred = None
    # Number of FI runs
    fi_total_runs = 0
    # Path to model FI stats folder
    fi_stats_dir = ""
    # Path to model FI output folder
    fi_output_dir = ""
    # Text language
    text_lang = ""

    count_fi_bits = None
    count_fi_layers = None
    count_fi_opcode = None
    count_fi_layer_name = None

    # All SDCs
    all_sdc = None

    # SDC rate and its Confidence scores
    sdc_rate = None
    confidence_scores_95 = None
    confidence_scores_99 = None

    # Average Cosine Similarity and its Confidence scores
    avg_cosine_similarity = None
    CI_cosine_similarity = None

    # FIRuns
    fi_runs = None

    # FIOutputs
    fi_outputs = None

    # FIOutputs embedding distance
    fi_outputs_embedding_distance = None

    def __init__(self, _model_name, _model_input_num, _fi_stats_dir, _fi_output_path):
        self.model_name = _model_name
        self.fi_stats_dir = _fi_stats_dir
        self.fi_output_dir = _fi_output_path
        self.model_input_num = _model_input_num
        self.all_sdc = []
        self.model_all_pred = []
        self.fi_runs = {}
        self.fi_outputs = {}
        self.fi_outputs_embedding_distance = {}

        self.parseFIStats()

        if self.model_name == 'gpt2':
            self.parseGPT2Output()
        elif 'bert' in self.model_name and self.model_name != 'roberta':
            self.parseBertOutput()
        elif 't5' in self.model_name:
            self.parseT5DecoderOutput()
        elif self.model_name == 'roberta':
            self.parseRobertaOutput()
        else:
            print("Invalid model name")
            return

        global calculate_embedding_distance
        if calculate_embedding_distance:
            # Identify language of model_correct_pred
            self.text_lang = self.identifyLang(self.model_correct_pred)
            print("Language detected: " + self.text_lang)
            self.calculateEmbeddingDistance()

        self.analyzeFaults()

    # Calculate Embedding distance for CodeBert
    def calculateEmbeddingDistanceCodeBert(self):
        from transformers import AutoTokenizer, AutoModel
        tokenizer = AutoTokenizer.from_pretrained("microsoft/codebert-base")
        model = AutoModel.from_pretrained("microsoft/codebert-base")

        code_tokens=tokenizer.tokenize(str(self.model_correct_pred))
        tokens=[tokenizer.cls_token]+[tokenizer.sep_token]+code_tokens+[tokenizer.eos_token]
        input_ids=tokenizer.convert_tokens_to_ids(tokens)
        correct_embeddings = model(torch.tensor(input_ids)[None,:])[0]

        for key in self.fi_outputs.keys():
            if self.fi_outputs[key] != self.model_correct_pred and key in self.fi_runs.keys():
                # Calculate embedding distance
                code_tokens=tokenizer.tokenize(str(self.fi_outputs[key]))
                tokens=[tokenizer.cls_token]+[tokenizer.sep_token]+code_tokens+[tokenizer.eos_token]
                input_ids=tokenizer.convert_tokens_to_ids(tokens)
                incorrect_embeddings = model(torch.tensor(input_ids)[None,:])[0]

                set_trace()
                cos = torch.nn.CosineSimilarity(dim=0, eps=1e-6)
                distance = cos(correct_embeddings[0][0], incorrect_embeddings[0][0]).item()
                self.fi_outputs_embedding_distance[key] = distance

    # Calculate Embedding distance
    def calculateEmbeddingDistance(self):

        # codebert
        #if self.model_name == 'codebert':
        # g                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                return self.calculateEmbeddingDistanceCodeBert()

        from flair.data import Sentence
        from flair.embeddings import WordEmbeddings, DocumentPoolEmbeddings

        # Load embeddings
        word_embed = WordEmbeddings(self.text_lang)
        fasttext_embedding = DocumentPoolEmbeddings([word_embed])

        # Calculate embedding distance
        for key in self.fi_outputs.keys():
            if self.fi_outputs[key] != self.model_correct_pred and key in self.fi_runs.keys():
                # Calculate embedding distance
                sentence1 = Sentence(self.model_correct_pred)
                sentence2 = Sentence(self.fi_outputs[key])
                fasttext_embedding.embed(sentence1)
                fasttext_embedding.embed(sentence2)

                cos = torch.nn.CosineSimilarity(dim=0, eps=1e-6)
                distance = cos(sentence1.embedding, sentence2.embedding).item()

                self.fi_outputs_embedding_distance[key] = distance

    # Identify language of text
    def identifyLang(self, text):

        # Check if model file exists, otherwise download it.
        if not os.path.isfile('lid.176.ftz'):
            print("Downloading fasttext model")
            os.system("wget https://dl.fbaipublicfiles.com/fasttext/supervised-models/lid.176.ftz")

        model = fasttext.load_model('lid.176.ftz')
        predictions = model.predict(text, k=1)
        return predictions[0][0].split("__label__")[1]

    # Calculate confidence interval
    def mean_confidence_interval(self, data, confidence=0.95):
        import numpy as np
        import scipy.stats
        a = 1.0 * np.array(data)
        n = len(a)
        m, se = np.mean(a), scipy.stats.sem(a)
        h = se * scipy.stats.t.ppf((1 + confidence) / 2., n-1)
        return m, h

    # Analyze the effect of faults on the model
    def analyzeFaults(self):
        all_fi_bits = []
        all_fi_layers = []
        all_fi_opcode = []
        all_fi_layer_name = []

        # Assuming that fi_runs and fi_outputs share the smae keys.
        if len(self.fi_runs.keys()) != len(self.fi_outputs.keys()):
            print("----------------------------------")
            print("WARNING: Mismatch between number of FI runs and number of FI outputs")
            k = [i for i in self.fi_outputs.keys() if i not in self.fi_runs.keys()]
            print("\t Keys not found: " + str(k))
            print("\t Prediction corresponding to these keys: " + str([self.fi_outputs[i] for i in k]))
            print("\t Correct prediction: " + str(self.model_correct_pred))
            print("----------------------------------")

        for key in self.fi_outputs.keys():
            if self.fi_outputs[key] != self.model_correct_pred and key in self.fi_runs.keys():
                all_fi_bits.append(self.fi_runs[key].fi_bit)
                all_fi_layers.append(self.fi_runs[key].fi_layer)
                all_fi_opcode.append(self.fi_runs[key].fi_opcode)
                all_fi_layer_name.append(self.fi_runs[key].fi_layer_name)

            if self.fi_outputs[key] != self.model_correct_pred:
                self.all_sdc.append(key)

        self.count_fi_bits = Counter(all_fi_bits)
        self.count_fi_layers = Counter(all_fi_layers)
        self.count_fi_opcode = Counter(all_fi_opcode)
        self.count_fi_layer_name = Counter(all_fi_layer_name)

        # Calculate SDC rate
        self.sdc_rate = len(self.all_sdc) / self.fi_total_runs
        self.confidence_scores_95 = 1.96 * math.sqrt(self.sdc_rate * (1 - self.sdc_rate) / self.fi_total_runs)
        self.confidence_scores_99 = 2.58 * math.sqrt(self.sdc_rate * (1 - self.sdc_rate) / self.fi_total_runs)

        # Calculate Average Cosine Similarity and 95% confidence scores
        distances = [self.fi_outputs_embedding_distance[key] for key in self.fi_outputs_embedding_distance.keys()]
        self.avg_cosine_similarity, self.CI_cosine_similarity = self.mean_confidence_interval(distances, confidence=0.95)

    # Parse BioBert output
    def parseBertOutput(self):
        assert 'bert' in self.model_name

        # Check if PredResult.txt exists
        if not os.path.isfile(self.fi_output_dir + "/onnx-pred/onnx-pred.txt"):
            print("onnx-pred.txt does not exist")
            return

        # Open output file
        with open(self.fi_output_dir + "/onnx-pred/onnx-pred.txt") as opFile:
            content = opFile.read()
            content = content.split("Run")[1:]

            # Make sure that the number of FI stats is same as the number of predictions.
            #if len(content) != self.fi_total_runs and "Mismatch between number of FI stats and number of predictions":
            #    set_trace()
            all_predictions = []

            for run in content:
                try:
                    run_num = int(run.split(': ')[-1].split(' ', 1)[0])
                    run_pred = str(run.split('\n')[1])

                    all_predictions.append(run_pred)
                    # Store the prediction
                    self.fi_outputs[run_num] = run_pred
                except:
                    set_trace()
                    print("Error parsing run: ", run)
                    continue

            count = Counter(all_predictions)
            self.model_correct_pred = count.most_common(1)[0][0]
            self.model_all_pred = all_predictions

    # Parse RoBerta output
    def parseRobertaOutput(self):
        assert self.model_name == 'roberta'

        # Check if PredResult.txt exists
        if not os.path.isfile(self.fi_output_dir + "/PredResult.txt"):
            print("PredResult.txt does not exist")
            return

        # Open output file
        with open(self.fi_output_dir + "/PredResult.txt") as opFile:
            content = opFile.read()
            content = content.split("\n")[:999]

            all_predictions = []
            for run in content:
                try:
                    run_num = int(run.split('#')[1].split(' ')[0])
                    run_pred = str(run.split(':')[-1].replace(' ',''))

                    all_predictions.append(run_pred)
                    # Store the prediction
                    self.fi_outputs[run_num] = run_pred
                except:
                    print("Error parsing run: ", run)
                    continue

            count = Counter(all_predictions)
            self.model_correct_pred = count.most_common(1)[0][0]
            self.model_all_pred = all_predictions

    # Parse GPT2 output
    def parseGPT2Output(self):
        assert self.model_name == 'gpt2'

        # Check if PredResult.txt exists
        if not os.path.isfile(self.fi_output_dir + "/PredResult.txt"):
            print("PredResult.txt does not exist")
            return

        # Open output file
        with open(self.fi_output_dir + "/PredResult.txt") as opFile:
            content = opFile.read()
            content = content.split("Run")[1:]

            all_predictions = []

            for run in content:
                try:
                    run_num = int(run.split('#')[1].split(' ')[0])
                    run_pred = str(run.replace(' ','').split('\n')[1].split(':')[0])

                    all_predictions.append(run_pred)
                    # Store the prediction
                    self.fi_outputs[run_num] = run_pred
                except:
                    print("Error parsing run: ", run)
                    continue

            count = Counter(all_predictions)
            self.model_correct_pred = count.most_common(1)[0][0]
            self.model_all_pred = all_predictions

    # Parse T5 decoder output
    def parseT5DecoderOutput(self):
        assert 't5' in self.model_name

        # Check if PredResult.txt exists
        if not os.path.isfile(self.fi_output_dir + "/PredResult.txt"):
            print("PredResult.txt does not exist")
            return

        # Open output file
        with open(self.fi_output_dir + "/PredResult.txt") as opFile:
            content = opFile.read()
            content = content.split("Run")[1:]

            all_predictions = []

            for run in content:
                try:
                    run_num = int(run.split('#')[1].split(' ')[0])
                    run_pred = str(run.replace('\n', '').split(':')[-1])

                    all_predictions.append(run_pred)
                    # Store the prediction
                    self.fi_outputs[run_num] = run_pred
                except:
                    print("Error parsing run: ", run)
                    continue

            count = Counter(all_predictions)
            self.model_correct_pred = count.most_common(1)[0][0]
            self.model_all_pred = all_predictions

    # Parse FI stats
    def parseFIStats(self):
        # Iterate through all files in the FI stats directory
        for subdir, dirs, files in os.walk(self.fi_stats_dir):
            for file in files:
                # Check if the file is a FI stats file
                if 'llfi' in file and 'swp' not in file:
                    self.fi_total_runs = self.fi_total_runs + 1
                    # Parse the FI stats file
                    fi_num = int(str(file).split('-')[-1].split('.')[0])
                    # Open and parse file
                    with open(os.path.abspath(subdir) + "/" + file, 'r') as f:
                        content = f.readlines()
                        content = content[0].replace(' ', '').split(',')
                        fi_bit = -1
                        fi_layer = -1
                        fi_old_val = -1
                        fi_new_val = -1
                        fi_cycle = -1
                        fi_layer_name = ""
                        fi_opcode = ""

                        for fields in content:
                            first, second = fields.split('=')
                            if first == 'fi_cycle':
                                fi_cycle = int(second)
                            elif first == 'fi_bit':
                                fi_bit = int(second)
                            elif first == 'ml_layer_number':
                                fi_layer = int(second)
                            elif first == 'oldHex':
                                fi_old_val = int(second, 16)
                            elif first == 'newHex':
                                fi_new_val = int(second, 16)
                            elif first == 'opcode':
                                fi_opcode = second
                            elif first == 'ml_layer_name':
                                fi_layer_name = second

                        # Validate inputs
                        assert fi_bit != -1 and fi_layer != -1 and fi_old_val != -1 and fi_new_val != -1 and fi_cycle != -1 and fi_layer_name != "" and fi_opcode != ""
                        runObj = FIRun(self.model_input_num, fi_num, fi_bit, fi_layer, fi_old_val, fi_new_val, fi_cycle, fi_opcode, fi_layer_name)

                        # Add to dictionary
                        self.fi_runs[fi_num] = runObj

    # Print stats
    def printStats(self, args):
        if args.sdc_rates:
            print("------- Input: " + str(self.model_input_num) + " -------")
            print("SDC Rate: " + str(len(self.all_sdc) / self.fi_total_runs))
            print("SDC Rate Confidence Scores: 95%: " + str(self.confidence_scores_95))
            print("Total SDCs: " + str(len(self.all_sdc)) + "  Total runs: " + str(self.fi_total_runs))

        if args.calc_cs:
            print("Average Cosine Similarity: " + str(self.avg_cosine_similarity))
            print("Cosine Similarity Confidence Scores: 95%: " + str(self.CI_cosine_similarity))

        if args.verbose:
            # Print all SDCs
            print("All SDCs: " + str([[key, self.fi_outputs[key]] for key in self.all_sdc]))
            print("Correct prediction: " + str(self.model_correct_pred))
            print("FI_Layers: " + str(self.count_fi_layers))
            print("FI_Bits: " + str(self.count_fi_bits))
            print("Corrupted vals: " + str([[key, self.fi_runs[key].fi_new_val]  for key in self.all_sdc]))
            print("FI Layers: " + str([[key, self.fi_runs[key].fi_layer]  for key in self.all_sdc]))

##### MAIN #####
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("results", help="Path to results directory")
    parser.add_argument("model_name", help="Name of the model. Can be one of: biobert, codebert, gpt2, t5-encoder, t5-decoder, roberta")
    parser.add_argument("--calculate-embeddings", action="store_true", default=False, help="Calculate embedding distance between correct and incorrect predictions. \t Default: False")
    parser.add_argument("--sdc-rates", action="store_true", default=False, help="Print the SDC rate \t Default: False")
    parser.add_argument("--calc-cs", action="store_true", default=False, help="Print the Cosine Similarity rate \t Default: False")
    parser.add_argument("--verbose", action="store_true", default=False, help="Print verbose output \t Default: False")
    # Export to CSV?
    parser.add_argument("--export-csv", action="store_true", default=False, help="Export to CSV \t Default: False")

    args = parser.parse_args()

    global calculate_embedding_distance
    calculate_embedding_distance = args.calculate_embeddings

    # Validate inputs
    if not os.path.isdir(args.results):
        print("Results directory does not exist")
        return

    if args.model_name not in ["biobert", "codebert", "gpt2", "t5-encoder", "t5-decoder", "roberta"]:
        print("Invalid model name")
        return

    stats_for_inputs = {}

    # get sub-directory of results directory
    for dir in [ f for f in os.scandir(args.results) if f.is_dir() ]:
        input_num = int(dir.name)
        fi_stats_dir = ""
        model_output_dir = ""

        # get sub-directory of input directory
        for dirs1 in [ f for f in os.scandir(dir.path) if f.is_dir() ]:
            directory_name = str(dirs1.name)
            if directory_name not in ['prediction', 'out']:
                assert 'llfi' in directory_name
                # This is LLFI's FI stats directory
                fi_stats_dir = os.path.abspath(dirs1.path)
            else:
                # This is the model's output directory
                model_output_dir = os.path.abspath(dirs1.path)

        # Validate that both directories exist
        assert os.path.isdir(fi_stats_dir) and os.path.isdir(model_output_dir)
        assert input_num >= 0 and input_num < 10

        NLPModelObj = NLPModel(args.model_name, input_num, fi_stats_dir, model_output_dir)
        stats_for_inputs[input_num] = NLPModelObj

    # Print stats
    for i in sorted(stats_for_inputs.keys()):
        stats_for_inputs[i].printStats(args)

    # Export to CSV
    if args.export_csv:

        # Export overall statistics
        header = ["Input", "SDC Rate", "SDC Rate Confidence Scores", "Total SDCs", "Total runs", "Average Cosine Similarity", "Cosine Similarity Confidence Scores"]
        rows = []
        for i in sorted(stats_for_inputs.keys()):
            rows.append([i, stats_for_inputs[i].sdc_rate, stats_for_inputs[i].confidence_scores_95, len(stats_for_inputs[i].all_sdc), stats_for_inputs[i].fi_total_runs, stats_for_inputs[i].avg_cosine_similarity, stats_for_inputs[i].CI_cosine_similarity])

        import csv
        with open(str(args.model_name) + '_results.csv', 'w') as f:
            write = csv.writer(f)
            # Export Model Name and leave a row empty
            write.writerow([args.model_name])
            write.writerow([])
            write.writerow(header)
            write.writerows(rows)

        # Export SDCs for each input
        header = ["S. No.", "SDC", "", "Correct Prediction", "", "fi_run", "fi_bit", "fi_layer", "fi_old_val", "fi_new_val", "fi_cycle", "fi_opcode", "fi_layer_name"]
        rows = []
        for i in sorted(stats_for_inputs.keys()):
            model_name = args.model_name
            input_num = i
            counter = 0
            rows.append(["Input: " + str(input_num)])
            rows.append([])
            for sdc in stats_for_inputs[i].all_sdc:
                counter = counter + 1
                wrong_pred = stats_for_inputs[i].fi_outputs[sdc]
                fi_run = stats_for_inputs[i].fi_runs[sdc].fi_run
                fi_bit = stats_for_inputs[i].fi_runs[sdc].fi_bit
                fi_layer = stats_for_inputs[i].fi_runs[sdc].fi_layer
                fi_old_val = stats_for_inputs[i].fi_runs[sdc].fi_old_val
                fi_new_val = stats_for_inputs[i].fi_runs[sdc].fi_new_val
                fi_cycle = stats_for_inputs[i].fi_runs[sdc].fi_cycle
                fi_opcode = stats_for_inputs[i].fi_runs[sdc].fi_opcode
                fi_layer_name = stats_for_inputs[i].fi_runs[sdc].fi_layer_name

                rows.append([counter, wrong_pred, "", stats_for_inputs[i].model_correct_pred, "", fi_run, fi_bit, fi_layer, fi_old_val, fi_new_val, fi_cycle, fi_opcode, fi_layer_name])
                rows.append([])

        # Open CSV file
        with open(str(args.model_name) +"_all_inputs_results.csv", 'w') as f:
            write = csv.writer(f)
            # Export Model Name and leave a row empty
            write.writerow([args.model_name])
            write.writerow([])
            write.writerow(header)
            for row in rows:
                write.writerow(row)


if __name__ == '__main__':
    main()
