/*
 * input.c - Sample program for vgg16
 *
 *
 *
 */

#include <stdio.h>
#include <OnnxMlirRuntime.h>
#include "json-c/json.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "onnx/onnx_pb.h"
#include <fstream>
#include <vector>

#define NUM_INPUTS 1 // When using the same input.c file for a different model, specify the number of inputs here depending on the model

using namespace std;

extern "C" {
  OMTensorList *run_main_graph(OMTensorList *);
}

void export_layer_output_to_json(OMTensorList *, char*, char*);

int main(int argc, char *argv[]) {

    //Input pointers needed for the model
    char *inp[NUM_INPUTS];
    char* savefilename = "layeroutput.txt";
    char* output_seq = NULL;
    vector<void*> heapAllocs;
    
    unsigned int numArguments = NUM_INPUTS+2; 
    if (argc == numArguments) {
    	for (int i = 0; i < NUM_INPUTS; i++){
    		inp[i] = argv[i+1];
    	}
	output_seq = argv[NUM_INPUTS+1];
    } else {
        printf("Must supply the path to an image file.\n");
    }

    OMTensor *img_list[NUM_INPUTS];
    for (int i = 0; i < NUM_INPUTS; i++) {
	    onnx::TensorProto input;
	    std::ifstream in(inp[i], std::ios_base::binary);
            input.ParseFromIstream(&in);
	    
	    auto d = input.dims();

	    // When using the same input.c file for a different model, Change "float" to the type of input data that the model expects
	    float* input_data {reinterpret_cast<float *>(const_cast<char*>(input.raw_data().data()))};
	    float* heap_input_data;
	    long *in_shape = (long*)malloc(d.size()*sizeof(long));
	    heapAllocs.push_back((void*)in_shape);
	  
	    int64_t totalSize = 1; 
            std::cout<< d.size()<< std::endl;
	    for(int j =0; j < d.size(); j++){
		    in_shape[j] = d[j];
		    totalSize *= d[j];
		    std::cout << "in_shape[i] " << in_shape[j] << endl;
	    }

	    heap_input_data = (float*)malloc(sizeof(float) * totalSize);
	    memcpy(heap_input_data, input_data, totalSize * sizeof(float));

	    
	    //When using the same input.c file for a different model, Change ONNX_TYPE depending on the input type
	    OMTensor *x1 = omTensorCreate(heap_input_data, in_shape, d.size(), ONNX_TYPE_FLOAT);
	    img_list[i] = x1;
	    heapAllocs.push_back((void*)heap_input_data);
    }
    
    OMTensorList *graph_input = omTensorListCreate(img_list, 4);

    // Call the compiled onnx model function.
    OMTensorList *outputList = run_main_graph(graph_input);

    // Export layer outputs to a JSON file
    export_layer_output_to_json(outputList, savefilename, output_seq);

    for (void* ptr : heapAllocs) {
	   free(ptr);
    }

  return 0;
}

// Function to split a string
char** str_split(char* a_str, const char a_delim, int* len)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;
    *len = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = (char**)malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);

            *len = (*len) + 1;
        }
        assert(idx == count - 1);
    }

    return result;
}

// Convert a list of char**to int*
int* convert_to_int(char** list, int count)
{
	int* result = (int*) malloc(count*sizeof(int));

	for (int i = 0 ; i < count; i++)
	{
		result[i] = atoi(list[i]);
	}

	return result;
}

// Turn this on for debugging JSON creater.
// #define DEBUG_MSG(...) printf(__VA_ARGS__)
#define DEBUG_MSG(...)

//Function to export layer outputs to JSON format.
void export_layer_output_to_json(OMTensorList *outputList, char* savefile, char* expected_op_seq)
{

	int count = 0;
	char** tokens = str_split(expected_op_seq, ',', &count);
	int* layer_seq = convert_to_int(tokens, count);

	// Global JSON object
	json_object* jobj = json_object_new_object();

	for (int64_t i = 0; i < omTensorListGetSize(outputList); i++) {

		// JSON object for this layer
		json_object* jobj_layer = json_object_new_object();

        	DEBUG_MSG("Reading output of layer %lu\n", i);

        	OMTensor *omt = omTensorListGetOmtByIndex(outputList, i);

		// Get properties of the tensor that you want to export to the JSON file
		int64_t rank = omTensorGetRank(omt);
		int64_t *shape = omTensorGetShape(omt);
		int64_t numElements = (int64_t) (omTensorGetNumElems(omt) / shape[0]);
		float *dataBuf = (float *)omTensorGetDataPtr(omt);
		int64_t bufferSize = omTensorGetBufferSize(omt);
	
		DEBUG_MSG("Rank: %lu \nNumber of elements: %lu \n", rank, numElements);
		DEBUG_MSG("Shape: ");
		for (int64_t j = 0; j < rank; j++){
			DEBUG_MSG("%lu,  ", shape[j]);
		}
		DEBUG_MSG("\n");

		DEBUG_MSG("Buffer Size: %lu\n", bufferSize);

		json_object *JLayerId = json_object_new_int(layer_seq[i]);	
		json_object *JRank = json_object_new_int(rank);	
		json_object *JNumElements = json_object_new_int(numElements);

		json_object_object_add(jobj_layer, "Layer Id", JLayerId);	
		json_object_object_add(jobj_layer, "Rank", JRank);	
		json_object_object_add(jobj_layer, "Number of Elements", JNumElements);	
	
		
		json_object* JShape = json_object_new_array();
		for (int64_t j = 0; j < rank; j++) {
			json_object* temp = json_object_new_int(shape[j]);
			json_object_array_add(JShape, temp);
		}


		json_object* JData = json_object_new_array();
		for (int64_t j = 0; j < numElements; j++) {
			json_object* temp = json_object_new_double(dataBuf[j]);
			json_object_array_add(JData, temp);
		}
		
		json_object_object_add(jobj_layer, "Shape", JShape);	
		json_object_object_add(jobj_layer, "Data", JData);	

		char str[5];
		sprintf(str, "%ld", i);
		json_object_object_add(jobj, str, jobj_layer);
    	}

    // Free heap memory
    free(tokens);
    free(layer_seq);
       
	char* val = (char*) json_object_get_string(jobj);	
	FILE* save = fopen(savefile, "w+");
	fputs(val, save);
	fclose(save);
}
