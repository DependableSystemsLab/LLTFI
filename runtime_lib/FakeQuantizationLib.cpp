#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>

#include "FakeQuantizationLib.h"
#include "FaultInjector.h"

float *x_values;
float *w_values;
int x_index = 0;
int w_index = 0;

// For Stage 2
int CurrentLayerIndex = 0;

int bit_width;
int max_number;
char cwd[PATH_MAX];

// For W
float min_w;
float max_w;

// For X
float min_x;
float max_x;

// Max and Min Multipliers when Dequantizing the values
float maxValue;
float minValue;

float s_x;
float s_w;

bool firstIteration = true;
bool stage2 = false;

void makeFakeQuantFolder(char *dirname)
{

    // Attempt to create a directory
    if (mkdir(dirname, 0755) == -1)
    {
        if (errno == EEXIST)
        {
            printf("Directory already exists.\n");
        }
        else
        {
            perror("Error creating directory");
            return;
        }
    }
    else
    {
        printf("Directory created successfully.\n");
    }
}

int getCurrentCWD(int currentLayerNumber)
{

    // Get the current working directory
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("Current working directory: %s\n", cwd);
        strcat(cwd, "/llfi/FakeQuantHelper");
        makeFakeQuantFolder(cwd);
        sprintf(cwd, "%s/layer%i.txt", cwd, currentLayerNumber);
    }
    else
    {
        perror("getcwd() error");
        return 1;
    }

    return 0;
}

void saveFile()
{

    FILE *file;
    file = fopen(cwd, "w"); // Open the file for writing
    if (file == NULL)
    {
        perror("Error opening file for writing");
        return;
    }
    fprintf(file, "%f\n", s_x);
    fprintf(file, "%f\n", s_w);
    fprintf(file, "%f\n", min_w * min_x);
    fprintf(file, "%f\n", max_w * min_x);
    fclose(file);
}

void ReadFromFile()
{

    FILE *file;
    // Reading from the file
    file = fopen(cwd, "r");
    if (file == NULL)
    {
        perror("Error opening file for reading or file does not exists");
        return;
    }
    fscanf(file, "%f", &s_x);
    fscanf(file, "%f", &s_w);
    fscanf(file, "%f", &minValue);
    fscanf(file, "%f", &maxValue);

    fclose(file);

    printf("Reading this %f", s_x);
    printf("Reading this %f", s_w);
}

int checkIfFileExists()
{

    FILE *file;
    // Reading from the file
    file = fopen(cwd, "r");
    if (file == NULL)
    {
        return 0;
    }
    return 1;
}

// Comparison function for floating-point numbers
int compare_floats(const void *a, const void *b)
{
    const float *fa = (const float *)a;
    const float *fb = (const float *)b;
    if (*fa < *fb)
    {
        return -1;
    }
    else if (*fa > *fb)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
float calculate_mean(float array[], int n)
{
    float sum = 0.0;
    for (int i = 0; i < n; i++)
    {
        sum += array[i];
    }
    return sum / n;
}

float find_percentile_value(float array[], int n, int percentile)
{
    if (n == 0)
        return 0; // Handle empty array edge case

    // Calculate the index for the given percentile
    double index = (percentile / 100.0) * (n + 1);

    // Calculate integer indices around the calculated index
    int lower_index = (int)index;
    int upper_index = lower_index + 1;
    if (upper_index >= n)
        upper_index = n - 1; // Boundary condition

    return array[lower_index];
}

void FindPercentile(int minPercentileThreshold, int maxPercentileThreshold)
{
    qsort(x_values, x_index, sizeof(float), compare_floats);
    qsort(w_values, w_index, sizeof(float), compare_floats);

    if (minPercentileThreshold == 0)
    {
        min_w = w_values[0];
    }
    else
    {
        min_w = find_percentile_value(w_values, w_index, minPercentileThreshold);
    }

    if (maxPercentileThreshold == 100)
    {
        max_w = w_values[w_index - 1];
    }
    else
    {
        max_w = find_percentile_value(w_values, w_index, maxPercentileThreshold);
    }

    min_x = x_values[0];
    max_x = x_values[x_index - 1];

    float mean = calculate_mean(w_values, w_index);
    float median = find_percentile_value(w_values, w_index, 50); // 50th percentile is the median
    float percentile25 = find_percentile_value(w_values, w_index, 25);
    float percentile75 = find_percentile_value(w_values, w_index, 75);
    double percentile90 = find_percentile_value(w_values, w_index, 90);
    float b_1 = find_percentile_value(w_values, w_index, 75);

    printf("W Mean: %lf\n", mean);
    printf("W Median: %lf\n", median);
    printf("W 25th Percentile: %lf\n", percentile25);
    printf("W 75th Percentile: %lf\n", percentile75);
    printf("W B-75th Percentile: %f\n", b_1);
    printf("W 90th Percentile: %lf\n", percentile90);

    mean = calculate_mean(x_values, x_index);
    median = find_percentile_value(x_values, x_index, 50); // 50th percentile is the median
    percentile25 = find_percentile_value(x_values, x_index, 25);
    percentile75 = find_percentile_value(x_values, x_index, 75);
    percentile90 = find_percentile_value(x_values, x_index, 90);
    b_1 = find_percentile_value(x_values, x_index, 75);
    printf("X Mean: %f\n", mean);
    printf("X Median: %f\n", median);
    printf("X 25th Percentile: %f\n", percentile25);
    printf("X 75th Percentile: %f\n", percentile75);
    printf("X B-75th Percentile: %f\n", b_1);
    printf("X 90th Percentile: %lf\n", percentile90);
}

/*
 * Quantize is Int( r / s )
 * Where s is 2 max( |r| ) / (2^b - 1)
 * Dequantize is S * Q(r)
 * Where q is quantized int and s_w is scaling factor of w and
 * s_x is the scaling factor of x

 * y = W X
 * y = S_w * Q(w) * S_x * Q(x)
*/

float scale(float x_max, float x_min)
{
    return (2 * fmax(fabs(x_max), fabs(x_min))) / ((2 ^ bit_width) - 1);
}

int quantize_helper(float x, float s)
{
    return (int)(x / s);
}

float dequantize(int q)
{
    if (!stage2)
    {
        return q;
    }

    float result = (float)(s_w * s_x * q);

    if (result < minValue)
    {
        result = minValue;
    }
    else if (result > maxValue)
    {
        result = maxValue;
    }

    return result;
}
float Quantize(float w1, float x1, int currentLayerIndex, int totalNumberOfLayers)
{
    stage2 = true;
    if (CurrentLayerIndex == currentLayerIndex)
    {
        int ans = (quantize_helper(w1, s_w) * quantize_helper(x1, s_x));
        if (ans > (max_number))
        {
            ans = max_number;
        }
        if (ans < (-1 * max_number))
        {
            ans = (-1 * max_number);
        }

        return ans;

    }
    else
    {
        getCurrentCWD(currentLayerIndex);
        ReadFromFile();
        CurrentLayerIndex = currentLayerIndex;
        printf("New layer Number so s_w is %f and s_x is %f\n", s_w, s_x);
        int ans = (quantize_helper(w1, s_w) * quantize_helper(x1, s_x));
        if (ans > (max_number))
        {
            ans = max_number;
        }
        if (ans < (-1 * max_number))
        {
            ans = (-1 * max_number);
        }

        return ans;

    }
}

void finished(int currentLayerIndex, int totalNumberOfLayers, int minPercentileThreshold, int maxPercentileThreshold, int bitWidth)
{
    bit_width = bitWidth;
    max_number = 2 ^ bit_width;
    printf("In this Layer - %i\n", currentLayerIndex);
    FindPercentile(minPercentileThreshold, maxPercentileThreshold);
    printf("Got called in finished!\n");
    printf("Index for w %i and Index for x %i\n", w_index, x_index);
    printf("Actual Min for x %f and Actual Max %f\n", x_values[0], x_values[x_index - 1]);
    printf("Min for x %f and Max %f\n", min_x, max_x);
    printf("Min for w %f and Max %f\n", min_w, max_w);
    printf("Actual Min for w %f and Actual Max %f\n", w_values[0], w_values[w_index - 1]);
    s_x = scale(max_x, min_x);
    s_w = scale(max_w, min_w);
    printf("Got this number %f\n", s_x);
    printf("Got this number %f\n", s_w);
    printf("Completed\n");
    getCurrentCWD(currentLayerIndex);
    saveFile();
    if (currentLayerIndex == totalNumberOfLayers)
    {
        free(x_values);
        free(w_values);
    }
    else
    {
        w_index = 0;
        x_index = 0;
        s_w = 0.0;
        s_x = 0.0;

        firstIteration = true;
    }
}

float getWAndX(float w1, float x1, int currentLayerIndex, int totalNumberOfLayers)
{
    if (firstIteration)
    {
        if (x_values == NULL && w_values == NULL)
        {
            x_values = (float *)malloc(INT_MAX * sizeof(float));
            w_values = (float *)malloc(INT_MAX * sizeof(float));
        }
        if (x_values == NULL || w_values == NULL)
        {
            printf("Malloc Failed\n");
            perror("Malloc Allocation failed\n");
            return 1;
        }

        x_index = 0;
        w_index = 0;

        firstIteration = false;
    }

    x_values[x_index] = x1;
    x_index++;

    w_values[w_index] = w1;
    w_index++;

    return (w1 * x1);
}

float FakeQuantIntegerBasedAddition(float num1, float num2)
{
    int intNum1 = (int)num1;
    int intNum2 = (int)num2;

    return (float)(intNum1 + intNum2);
}

float FakeQuantDequantizeAndBiasAddition(float quantizeNum, float basis)
{
    int QuantizeIntNum = (int)quantizeNum;
    float dequnatizedFloat = dequantize(QuantizeIntNum);
    float result = dequnatizedFloat + basis;

    return result;
}

float QuantizeMatMul(float w1, float x1, int currentLayerIndex, int totalNumberOfLayers)
{
    int quantizedInt = (int)Quantize(w1, x1, currentLayerIndex, totalNumberOfLayers);
    return dequantize(quantizedInt);
}