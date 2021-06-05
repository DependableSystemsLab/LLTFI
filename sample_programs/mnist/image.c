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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define RANK 2
#define NUM_OUTPUT_CLASSES 10 // For MNIST, there are 10 possible digits.

OMTensorList *run_main_graph(OMTensorList *);

int main(int argc, char *argv[]) {

    char *filename;

    if (argc == 2) {
        filename = argv[1];
    } else {
        printf("Must supply the path to an image file.\n");
    }

    int width, height, channels;

    float *rgb_image = stbi_loadf(filename, &width, &height, &channels, 1);
    long in_shape[] = {RANK, RANK};
    OMTensor *x1 = omTensorCreate(rgb_image, in_shape, RANK, ONNX_TYPE_FLOAT);
    OMTensor *img_list[1] = {x1};
    OMTensorList *input = omTensorListCreate(img_list, 1);

    // Call the compiled onnx model function.
    OMTensorList *outputList = run_main_graph(input);

    // Get the first omt as output.
    OMTensor *output = omTensorListGetOmtByIndex(outputList, 0);
    float *outputPtr = (float *)omTensorGetDataPtr(output);

    printf("Output for %s is: ", filename);

    // Print its content, should be in softmax form
    for (int i = 0; i < NUM_OUTPUT_CLASSES; i++)
        printf("%f ", outputPtr[i]);
    printf("\n");

    stbi_image_free(rgb_image);

  return 0;
}

