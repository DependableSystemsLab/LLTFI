#!/usr/bin/env python

import onnx
import pydot
import os
import pdb

#----------------------------------------------------------------------

def get_tensor_shape(node):
    # returns the shape of the tensor given an ONNX node

    return tuple( int(item.dim_value) for item in node.type.tensor_type.shape.dim )


#----------------------------------------------------------------------

def makeDot(model, addIndex = False):

    ingraph = model.graph

    # see e.g. https://pythonhaven.wordpress.com/2009/12/09/generating_graphs_with_pydot/
    outgraph = pydot.Dot(graph_type='digraph')

    #----------
    # Note that in the onnx model (at least when created
    # from pytorch) the computational boxes do not have names
    # but rather the connections between them
    #
    # This is more like a netlist. Note that each net in a
    # netlist should only have one output connected to it
    # (which defines the value) but can have multiple
    # inputs connected. We draw an edge from each of the
    # inputs to the single output
    #----------

    # this maps from an edge / netlist name to the node
    # which provides the output with this name
    nameToNodeOfOutput = {}

    #----------
    # find input nodes which have initializers
    # these are not real inputs but rather weights
    # learned during training
    #----------

    initializerNames = set([ node.name for node in ingraph.initializer ])

    #----------
    # add boxes for the input nodes
    #----------
    for index, node in enumerate(ingraph.input):
        # note that (at least when generated from pytorch)
        # things like convolution matrix weights
        # can be considered as inputs

        if node.name in initializerNames:
            # this is a weight node, skip it
            continue

        labels = [ "input " + node.name,
                  get_tensor_shape(node)
                  ]

        gn = pydot.Node(
            "in%d" % (index + 1),
            label = "\n".join([ str(x) for x in labels ]),
            shape = 'record', style = 'filled',
            fillcolor = '#A2CECE')
        outgraph.add_node(gn)

        assert node.name not in nameToNodeOfOutput
        nameToNodeOfOutput[node.name] = gn

    #----------
    # add boxes for the output nodes
    #----------

    outputGraphNodes = []

    for index, node in enumerate(ingraph.output):
        # note that (at least when generated from pytorch)
        # things like convolution matrix weights
        # can be considered as inputs

        labels = [ "output " + node.name,
                  get_tensor_shape(node)
                  ]


        gn = pydot.Node(
            "out%d" % (index + 1),
            label = "\n".join([ str(x) for x in labels ]),
            shape = 'record')

        outgraph.add_node(gn)

        outputGraphNodes.append(gn)

    #----------
    # add boxes for the computational nodes
    # and the corresponding edges
    #----------

    for index, node in enumerate(ingraph.node):
        # note that these nodes most of the time
        # do not have a name, i.e. node.name is the empty string

        labels = [ node.op_type, str(node.input), str(node.output),
                   ]

        #----------
        # this should go into some kind of decorator
        #----------
        if node.op_type in ('Conv', 'MaxPool'):

            # TODO: get number of filter banks

            for attr in node.attribute:
                # TODO: we should guarantee an ordering of the labels
                if attr.name == 'kernel_shape':
                    shape = tuple(int(x) for x in attr.ints)
                    labels.append("kernel size " + str(shape))

                elif attr.name == 'strides':
                    shape = tuple(int(x) for x in attr.ints)
                    if shape != (1,1):
                        labels.append("strides " + str(shape))


        elif node.op_type == 'Reshape':

            for attr in node.attribute:
                # TODO: we should guarantee an ordering of the labels
                if attr.name == 'shape':
                    shape = tuple(int(x) for x in attr.ints)
                    labels.append("shape " + str(shape))


        elif node.op_type == 'Dropout':

            for attr in node.attribute:
                # TODO: we should guarantee an ordering of the labels
                if attr.name == 'ratio':
                    labels.append("p=" + str(attr.f))


        #----------

        if addIndex:
            # for debugging
            labels.append("(index = %d)" % index)


        # create a graphviz node
        gn = pydot.Node(
                "n%d" % (index + 1),
                label = "\n".join([ str(x) for x in labels ]),
                shape = 'record', style = 'filled')
        outgraph.add_node(gn)

        # add outputs first
        for outputName in node.output:
            assert outputName not in nameToNodeOfOutput
            nameToNodeOfOutput[outputName] = gn

        # TODO: add more information about the node

        # add edges to inputs
        for inputName in node.input:

            # skip weights for the moment
            if inputName in initializerNames:
                continue

            # get the pydot node we have to connect to
            inputNode = nameToNodeOfOutput[inputName]

            outgraph.add_edge(pydot.Edge(src = inputNode, dst = gn))

    #----------
    # add edges of output nodes to their sources
    #----------

    # note that the output nodes do not have an input

    for node, graphNode in zip(ingraph.output, outputGraphNodes):

        # get the pydot node we have to connect to
        inputNode = nameToNodeOfOutput[node.name]

        outgraph.add_edge(pydot.Edge(src = inputNode, dst = graphNode))


    return outgraph

#----------------------------------------------------------------------

if __name__ == '__main__':

    import sys
    ARGV = sys.argv[1:]

    assert len(ARGV) == 2, "usage: in.onnx output.{dot,pdf,...}"

    inputFname, outputFname = ARGV

    if os.path.exists(outputFname):
        print >> sys.stderr,"output file " + outputFname + " exists already, refusing to overwrite it"
        sys.exit(1)


    # infer output format from suffix
    outputFormat = outputFname.split('.')[-1].lower()

    if inputFname.endswith(".gz"):
        import gzip
        fin = gzip.GzipFile(inputFname)
    else:
        fin = open(inputFname)

    model = onnx.load(inputFname)

    outgraph = makeDot(model)

    #----------
    # write the graph out
    #----------

    outgraph.write(outputFname, format = outputFormat)

