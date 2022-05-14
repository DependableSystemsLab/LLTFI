/*
 * image.c - Sample program for MNIST image classification
 *
 * This program reads an image file into a tensor,
 * feeds it into a TensorFlow model,
 * and obtains prediction results in an array.
 *
 */

#include <stdio.h>
#include <OnnxMlirRuntime.h>
#include "json-c/json.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define RANK 2
#define NUM_OUTPUT_CLASSES 10 // For MNIST, there are 10 possible digits.


OMTensorList *run_main_graph(OMTensorList *);
void export_layer_output_to_json(OMTensorList *, char*, char*);

int main(int argc, char *argv[]) {

    char *filename;
    char* savefilename = "layeroutput.txt";
    char* output_seq = NULL;

    if (argc == 3) {
        filename = argv[1];
	output_seq = argv[2];
    } else {
        printf("Must supply the path to an image file.\n");
    }

    int width, height, channels;

    float *rgb_image = stbi_loadf(filename, &width, &height, &channels, 1);
    long in_shape[] = {RANK, RANK};
    OMTensor *x1 = omTensorCreate(rgb_image, in_shape, RANK, ONNX_TYPE_FLOAT);
    OMTensor *img_list[1] = {x1};
    OMTensorList *input = omTensorListCreate(img_list, 1);
    
    struct timeval start, end;
    gettimeofday(&start, NULL);

    // Call the compiled onnx model function.
    OMTensorList *outputList = run_main_graph(input);

    gettimeofday(&end, NULL);

    double time_taken = end.tv_sec + end.tv_usec / 1e6 -
                        start.tv_sec - start.tv_usec / 1e6; // in seconds


    printf("Time taken to execute the model: %f\n", time_taken);

    // Export layer outputs to a JSON file
    export_layer_output_to_json(outputList, savefilename, output_seq);

    // Get the first omt as output.
    OMTensor *output = omTensorListGetOmtByIndex(outputList, omTensorListGetSize(outputList) - 1);
    float *outputPtr = (float *)omTensorGetDataPtr(output);

    printf("Final prediction for %s is: ", filename);

    // Print its content, should be in softmax form
    for (int i = 0; i < NUM_OUTPUT_CLASSES; i++)
        printf("%f ", outputPtr[i]);
    printf("\n");

    stbi_image_free(rgb_image);

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

    result = malloc(sizeof(char*) * count);

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
