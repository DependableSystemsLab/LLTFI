import argparse, onnx
from pdb import set_trace
import random

content = """compileOption:
    instSelMethod:
      - customInstselector:
          include:
            - CustomTensorOperator
          options:
            - -layerNo=1
            - -layerId=1986948931

    regSelMethod: regloc
    regloc: dstreg

    includeInjectionTrace:
        - forward

    tracingPropagation: False # trace dynamic instruction values.

    tracingPropagationOption:
        maxTrace: 250 # max number of instructions to trace during fault injection run
        debugTrace: False
        mlTrace: False # enable for tracing ML programs
        generateCDFG: True

runOption:
    - run:
        numOfRuns: 500
        fi_type: bitflip
        window_len_multiple_startindex: 1
        window_len_multiple_endindex: 500
        fi_max_multiple: 1
"""

'''
31069400218890051  ---> onnx.Constant 
28552536314307922  ---> onnx.Reshape 
1986948931  ---> onnx.Conv 
1970038098  ---> onnx.Relu    
30521821366870349  ---> onnx.MaxPoolSingleOut                                                                                                                                                
31367363490312788  ---> onnx.Transpose  
1835885895  ---> onnx.Gemm  
33884119937478483  ---> onnx.Softmax 
'''

def getONNXId(name):

    dict = {'Conv':1986948931, 'Relu':1970038098, 'MaxPool':30521821366870349,
            'MatMul':119251066446157, 'Add':6579265, 'AvgPool':30521821365761601,
            'Softmax':33884119937478483}

    return dict[name];

# Inject faults into these layers only
whiteList = ['Conv', 'Relu', 'MaxPool', 'Add', 'MatMul', 'AvgPool']

def main():
    global content
    parser = argparse.ArgumentParser()

    parser.add_argument("-l", "--layer", action="store", type=str, default="1", help="Layer for FI \t Default: 1")
    parser.add_argument("-r", "--runs", action="store", type=str, default="500", help="Number of runs \t Default: 500")
    parser.add_argument("-f", "--fault", action="store", type=str, default="1", help="FI_MAX_MULTIPLE \t Default: 1")
    parser.add_argument("--random_layer", action="store_true", default=False, help="Do you want to randomly select a layer? \t Default: False")
    parser.add_argument("--model_path", action="store", default="None", help="Path to Model.onnx file. It 's required in the case of random layer selection' \t Default: None")

    args = parser.parse_args()
    content = content.replace("numOfRuns: 500", "numOfRuns: "+str(args.runs))
    content = content.replace("fi_max_multiple: 1", "fi_max_multiple: "+str(args.fault))

    if args.random_layer:
        global whiteList
        model = onnx.load(args.model_path)
        layers = [node.op_type for node in model.graph.node]
        var_mod = [layers[i] for i in range(0, len(layers)) if layers[i] in whiteList]

        # Choose a random layer Id and layer Number
        layer_count = {}
        for layer in var_mod:
            if layer in layer_count.keys():
                layer_count[layer] = layer_count[layer] + 1
            else:
                layer_count[layer] = 1

        random_element_key = random.choice(list(layer_count))
        random_element_value = random.randint(1, int(layer_count[random_element_key]))

        content = content.replace("layerNo=1", "layerNo="+str(random_element_value))
        content = content.replace("layerId=1986948931", "layerId="+str(getONNXId(random_element_key)))
        
    else:
        content = content.replace("layerNo=4", "layerNo="+str(args.layer))

    f = open('input.yaml', 'w')
    f.write(content)

if __name__ == '__main__':
    main()
