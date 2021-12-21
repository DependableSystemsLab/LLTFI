# Python script to compare layer outputs in JSON format
# Python 3.8
# Author: Udit Agarwal
# This script will take 2 JSON files as input and will print the diff between them.
# It can also export a dot file showing the layer and layer outputs that have changed.

import argparse, os
import json
import warnings
import sys, struct, math
from pdb import set_trace
from math import floor, log10
import onnx
from collections import OrderedDict
import pygraphviz as pyg

###### GLOBALS #########

class DotGraph:

    Graph = None
    Layers = None

    def __init__(self, name):
        self.Graph = pyg.AGraph(strict=False, directed=True)
        self.Graph.graph_attr["label"] = '< <B>Model ' + name + '</B> >'
        self.Graph.graph_attr["compound"] = "true"
        self.Graph.node_attr["shape"] = "box"
        self.Graph.node_attr["fillcolor"] = "#EFE8E8"
        self.Graph.edge_attr["color"] = "black"

    def AddNodes(self, layers):
        self.Layers = layers

        for i in range(0, len(layers) - 1, 1):
            self.Graph.add_edge(layers[i], layers[i+1])
            node = self.Graph.get_node(layers[i+1])
            node.attr["style"] = "filled"

        node = self.Graph.get_node(layers[0])
        node.attr["style"] = "filled"


    def FormatFloat(self, f):
        f = str(f)
        retval = ""
        comp = f.split('e')
        retval = comp[0]
        if len(comp) == 2:
            retval = retval + "E" + comp[1]

        return str(retval)

    def AddMisMatch(self, layer, index, elements, old_val, new_val):
        # First chage the colour of the mismatch layer to red
        node = self.Graph.get_node(layer)
        node.attr["color"] = "red"
        node.attr["fillcolor"] = "#EFE8E8"
        node.attr["style"] = "filled,dashed"

        NameOfSubGraph = "cluster_"+layer+"_"+str(index)
        old_val_node_name = "Old Value: \n" + self.FormatFloat(old_val) + NameOfSubGraph 
        new_val_node_name = "New Value: \n" + self.FormatFloat(new_val) + NameOfSubGraph

        self.Graph.add_node(old_val_node_name, label=str("Old Value: \n" + self.FormatFloat(old_val)))
        self.Graph.add_node(new_val_node_name, label=str("New Value: \n" + self.FormatFloat(new_val)))

        # Add mismatch as a subgraph
        self.Graph.add_edge(old_val_node_name, new_val_node_name, color="blue")
        self.Graph.add_edge(layer, old_val_node_name, lhead=NameOfSubGraph)
        self.Graph.add_subgraph([old_val_node_name, new_val_node_name], name=NameOfSubGraph, color="blue", label='< <B>Index ' + str(index) + ' / ' + str(elements) + '</B> >', style='dashed')

    def PrintGraph(self):
        print(self.Graph.string())

    def SaveGraph(self):
        file = open('mismatch.dot', 'w')
        file.write(self.Graph.string())
        file.close()


# Conatins all stuff related to LLTFI FI
# TODO: Make it a singleton class
class LLTFI:

    fi_layer_id = None
    fi_layer_name = None
    fi_runtime_stats_dir = None
    fi_layer_op_mismatches = {}
    summary_only = False
    curr_file_name = None
    fi_stats = {}
    # Total number of experiments
    total_runs = 0
    # Total number of cases where bit flips corupted at least a single value in the output
    total_mismatches_found = 0
    # In how many cases, a single bit flip corrupted a single element in the output of a tensor operator
    single_output_corruption = 0
    # In how many cases, a single bit flip corrupted mutiple elements in the output of a tensor operator
    multiple_output_corruption = 0
    # FI in which bits lead to a flip in the sign of output element
    fi_bit_sign_flip = []
    # In how many cases old and new value differ by a single bit flip
    single_bit_flips = 0
    # In how many cases old and new value differ by muliple bit flips
    multiple_bit_flips = 0
    # Number of bitflips between old and new value
    distance_bitflips = []
    # Number of NaNs appeared as output of a tensor operator
    numbers_of_nan = 0
    # is new file
    is_new_file = False

    def SetFILayerId(self, layer_id):
        self.fi_layer_id = layer_id

    def SetFILayerName(self, name):
        self.fi_layer_name = name

    def GetFILayerId(self):
        return self.fi_layer_id

    def SetCurrFileName(self, fileName):
        self.curr_file_name = fileName
        self.fi_layer_op_mismatches[fileName] = []
        self.is_new_file = True

    def SetStatsDir(self, path):
        if os.path.isdir(path):
            self.fi_runtime_stats_dir = path
        else:
            print("Invalid Directory path: " + str(path) + "   Aborting!")
            exit()

    def ArgParser(self, argParser):

        if argParser.summary:
            self.summary_only = True
        else:
            self.summary_only = False

        if argParser.fiStatsDir != "None":
            self.SetStatsDir(str(argParser.fiStatsDir))

    def ReportMismatch(self, LayerId, d1_old, d2_new):
        if not self.is_new_file:
            assert (self.fi_layer_id is not None)
            if str(LayerId) == str(self.fi_layer_id):
                self.fi_layer_op_mismatches[self.curr_file_name].append([d1_old, d2_new])
                self.total_mismatches_found = self.total_mismatches_found + 1
            else:
                pass
        else:
            self.fi_layer_id = str(LayerId)
            self.fi_layer_op_mismatches[self.curr_file_name].append([d1_old, d2_new])
            self.total_mismatches_found = self.total_mismatches_found + 1
            self.is_new_file = False

    def GetFIStats(self):

        self.total_runs = len(os.listdir(self.fi_runtime_stats_dir))
        for key in [k for k,v in self.fi_layer_op_mismatches.items() if len(v) > 0]:

            id = key.split('.')[1]
            filename = ""
            for file in os.listdir(self.fi_runtime_stats_dir):
                if str(id) + ".txt" in file:
                    filename = file
                    break;
                else:
                    pass

            if filename == "":
                continue
            else:
                f = open(str(self.fi_runtime_stats_dir + "/" + filename), "r").read()
                bit = str((f.replace(" ", "").split(",")[-2]).split('=')[1])
                temp = [bit]
                temp = temp + [k for k in self.fi_layer_op_mismatches[key]]

                if key in self.fi_stats.keys():
                    self.fi_stats[key] = self.fi_stats[key] + temp
                else:
                    self.fi_stats[key] = []
                    self.fi_stats[key] = self.fi_stats[key] + temp

        self.single_output_corruption = 0
        self.multiple_output_corruption = 0
        for k, v in self.fi_stats.items():
            if len(v) == 2:
                self.single_output_corruption = self.single_output_corruption + 1
            else:
                if len(v) > 2:
                    self.multiple_output_corruption = self.multiple_output_corruption + 1

            for data_val in v[1:]:
                d_old = float(data_val[0])
                d_new = float(data_val[1])

                if (d_new > 0 and d_old < 0) or (d_new < 0 and d_old > 0):
                    self.fi_bit_sign_flip.append(v[0])

                if self.IsMultipleBitFlips(d_old, d_new):
                    self.multiple_bit_flips = self.multiple_bit_flips + 1
                else:
                    self.single_bit_flips = self.single_bit_flips + 1


    # Check if 2 floats differ by multiple bit flips or just a single bit flip
    def IsMultipleBitFlips(self, d1, d2):

        if str(d1) == 'nan' or str(d2) == 'nan':
            self.numbers_of_nan = self.numbers_of_nan + 1
            return True

        h1 = float.hex(float(d1))
        h2 = float.hex(float(d2))
        sign1 = h1.split('.')[0]
        sign2 = h2.split('.')[0]
        exp1 = h1.split('p')[-1]
        exp2 = h2.split('p')[-1]
        man1 = h1.split('.')[-1].split('p')[0]
        man2 = h2.split('.')[-1].split('p')[0]

        # Count bit flips in sign, exponent and mantissa
        numberOfFlips = self.CountBitFlips(sign1, sign2) + self.CountBitFlips(exp1, exp2) + self.CountBitFlips(man1, man2)
        self.distance_bitflips.append(numberOfFlips)

        assert numberOfFlips > 0

        if numberOfFlips > 1:
            return True
        else:
            return False

    # Count bitflips between 2 hex numbers
    def CountBitFlips(self, h1, h2):
        b1 = bin(int(h1, 16))
        b2 = bin(int(h2, 16))
        xor = bin(int(str(int(b1, 2) ^ int(b2, 2))))

        if int(xor, 2) == 0:
            return 0
        else:
            retval = 0
            for char in xor:
                if char == "1":
                    retval = retval + 1
            return retval

    def PrintSummary(self):
        print("Total number of files:" + str(self.total_runs))
        print("Total mismatches found in layer " + str(self.fi_layer_id) + " = " + str(self.total_mismatches_found))
        percentage = (float)(self.total_mismatches_found / self.total_runs) * 100
        print("""% of bitflips that corrupted the output of the FI layer: """ + str(percentage) + "%")
        percentage = (float)(self.single_output_corruption / self.total_mismatches_found) * 100
        print("""% of times a single bitflip caused just a single corrpution in layer output: """ + str(percentage) + "%")
        print( "FI in bits that cuased a sign flip: " + str(list(set(self.fi_bit_sign_flip))) )
        percentage = (float)(self.single_bit_flips / (self.single_bit_flips + self.multiple_bit_flips)) * 100
        print("% of single bit corruption in the output of the tensor operator: " + str(percentage) + "%")
        print("Number of NaNs in output of the corrupted tensor operator: " + str(self.numbers_of_nan))

    def PrintStats(self):
        print("Mismatches in FI layer:" + str(self.fi_layer_op_mismatches))
        print("FI Stats: " + str(self.fi_stats))
        self.PrintSummary()

    def PrintDump(self):
        self.GetFIStats()
        if self.summary_only:
            self.PrintSummary()
        else:
            self.PrintStats()

# Global LLTFI Object
LLTFIObject = LLTFI()

# Dict to maintain a list of mismatches found
mismatch = []

# Should we calculate FI stats
FIStatsCalculate = False

###### HELPER FUNCTIONS #######
def PrintStructuralDifferenceError():
    print("Input JSON files are structurally difference. Abort!");
    exit()

def Assert(a):
    if not a:
        PrintStructuralDifferenceError();
    else:
        pass

def AssertData(d1, d2, elements, layer, index, delta):
    if abs(float(d1) - float(d2)) <= float(delta):
        return False
    else:
        #print("Mismatch found at layer:" + layer + " index: " + index + " one value= " + d1 + " second value= " + d2)
        mismatch.append([layer, index, elements, float(d1), float(d2)])

        global FIStatsCalculate
        if FIStatsCalculate:
            global LLTFIObject
            LLTFIObject.ReportMismatch(layer, d1, d2)
        return True


def export_dot_graph(layerNames, GraphName, SelectedLayers):
    g = DotGraph(GraphName)
    g.AddNodes(layerNames)

    layerNamesSmall = [x.lower() for x in layerNames]
    SelectedLayers = [x.lower() for x in SelectedLayers]

    for item in mismatch:

        layer_name = layerNamesSmall[int(item[0])]

        if ('all' in SelectedLayers) or (layer_name.split('_')[0] in SelectedLayers) or (layer_name is layerNamesSmall[-1]):
            g.AddMisMatch(layerNames[int(item[0])], item[1], item[2], item[3], item[4])

    g.SaveGraph()


def getJsonDiff(j1, j2, delta):

    with open(j1, "r") as f:
        with open(j2, "r") as g:
            jf = json.load(f)
            jg = json.load(g)

            Assert(len(jf) == len(jg));

            # Iterate each layer
            for i in range(0, len(jf), 1):
                key = str(i)

                Assert(jf[key]['Layer Id'] == jg[key]['Layer Id'])
                Assert(jf[key]['Rank'] == jg[key]['Rank'])
                Assert(jf[key]['Number of Elements'] == jg[key]['Number of Elements'])
                Assert(jf[key]['Shape'] == jg[key]['Shape'])

                #Iterate all data outputs
                for j in range(0, len(jf[key]['Data']), 1):
                    retval = AssertData(str(jf[key]['Data'][j]), str(jg[key]['Data'][j]), str(jf[key]['Number of Elements']), str(jf[key]['Layer Id']), str(j), delta)

# Get total number of layers in a model
layers_in_json_file = None

def getTotalLayers(j1):

    global layers_in_json_file

    if layers_in_json_file is None:
        with open(j1, "r") as f:
            jf = json.load(f)
            layers_in_json_file = int(jf[str(len(jf) - 1)]['Layer Id'])

    return layers_in_json_file


##### MAIN #####
def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("first")
    parser.add_argument("second")
    parser.add_argument("ONNXModel")

    parser.add_argument("-d", "--delta", action="store", type=str, default="0", help="What should be the minimum difference between the mismatches? \t Default: 0.0")
    parser.add_argument("-f", "--folder", action="store_true", default=False, help="Specify this flag if the second argument is a folder path? \t Default: False")
    parser.add_argument("--dot", action="store_true", default=False, help="Do you want to output a dot file? \t Default: False")
    parser.add_argument("-fdiff", "--print_diff_final_op", action="store_true", default=False, help="Print mismatch on terminal only when there is a mismatch in the final layer o/p. \t Default: False")
    parser.add_argument("--selected_layers_in_dot", action="store", default="all", help="""Show only mismatches in selected layers during DOT file generation. Semicolon seperate values. \t Default: "all" """)
    parser.add_argument("--summary", action="store_true", default=False, help="Do you just want to output a summary of mismatches? \t Default: False")
    parser.add_argument("--getFIStatsRQ2", action="store_true", default=False, help="Should I compute FI stats? \t Default: False")
    parser.add_argument("--fiStatsDir", action="store", type=str, default="None", help="Directory path containg logs produced by LLTFI at runtime. \t Default: None")

    args = parser.parse_args()

    global LLTFIObject
    LLTFIObject.ArgParser(args)

    diff = None

    global FIStatsCalculate
    FIStatsCalculate = args.getFIStatsRQ2

    if args.folder:
        dir_path = args.second

        if os.path.isdir(dir_path):

            files = os.listdir(dir_path)
            global_mismatch = {}

            # Iterate all files in a directory
            for file in files:
                if os.path.isfile(dir_path + file):

                    global mismatch
                    LLTFIObject.SetCurrFileName(str(file))

                    mismatch = []
                    getJsonDiff(args.first, dir_path + file, args.delta)
                    if len(mismatch) > 0:
                        global_mismatch[str(dir_path + file)] = mismatch

            if len(global_mismatch) > 0:

                # Print mismatch only when final o/p differs
                if args.print_diff_final_op:
                    for key, value in list(global_mismatch.items()):

                        mismatch_found = False;

                        for mismatch in value:
  
                            if mismatch[0] == str(getTotalLayers(args.first)):
                                mismatch_found = True
                                break;

                        # Remove the mismatches where the last layer doesn't differ
                        if not mismatch_found:
                            global_mismatch.pop(key)
                else:
                    pass

                # Print summary only
                if args.summary:
                    print("Mismatches found in " + str(len([key for key in global_mismatch.keys()])) + " file(s).")
                else:
                    print("Mismatch found in: " + str([key for key in global_mismatch.keys()]))

            else:
                print("No mismatch found.")

        else:
            print("Invalid directory path: " + args.second)
            exit()
    
    else:
        if os.path.isfile(args.first) and os.path.isfile(args.second):

            LLTFIObject.SetCurrFileName(args.second)

            diff = getJsonDiff(args.first, args.second, args.delta)

            if len(mismatch) > 0:

                # Just print a summary of mismatches
                if args.summary:
                    print("Found " + str(len(mismatch)) + " mismatche(s)")
                else:
                    print(json.dumps(mismatch))

            else:
                print("No mismatch found.")
        else:
            print("Invalid file path(s): " + args.first + "   \n " + args.second)
            exit()
    
    # Export mismatches as a dot file. Don't output the dot file when folder is given as an input.
    if args.dot and (not args.folder):
        model = onnx.load(args.ONNXModel)

        output_names = []
        # Output Layer names
        for i in range(0, len(model.graph.node), 1):
            output_names.append(str(model.graph.node[i].op_type) + "_" + str(i+1))

        # Now export a dot graph showing the layer structure and mismatch points
        export_dot_graph(output_names, model.graph.doc_string, args.selected_layers_in_dot.split(';'))

    if args.getFIStatsRQ2:
        LLTFIObject.PrintDump()

def main_deprecated():
    warnings.warn("jsondiff is deprecated. Use jdiff instead.", DeprecationWarning)
    main()

if __name__ == '__main__':
    main()

