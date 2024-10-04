#ifndef FAKE_QUANTIZATION_H
#define FAKE_QUANTIZATION_H

#include <stdio.h>
#include <math.h>
#include <limits.h>


#ifdef __cplusplus
extern "C" {
#endif
float getWAndX(float w1, float x1, int currentLayerIndex, int totalNumberOfLayers);
void finished(int currentLayerIndex, int totalNumberOfLayers, int minPercentileThreshold, int maxPercentileThreshold, int bitWidth);
float dequantize(int q);
float Quantize(float w1, float x1, int currentLayerIndex, int totalNumberOfLayers);
float QuantizeMatMul(float w1, float x1, int currentLayerIndex, int totalNumberOfLayers);
float FakeQuantIntegerBasedAddition(float num1, float num2);
float FakeQuantDequantizeAndBiasAddition(float num1, float num2);
#ifdef __cplusplus
}
#endif

#endif // FAKE_QUANTIZATION_H
