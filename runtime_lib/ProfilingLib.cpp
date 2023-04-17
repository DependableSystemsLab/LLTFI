#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <iostream>
#include <vector>
#include <string>

struct layerProfCycle {
  int layerNo;
  std::string layerName;
  long long unsigned cycleStart;
  long long unsigned cycleEnd;

  layerProfCycle(int layerNo, std::string layerName) {
    this->layerNo = layerNo;
    this->layerName = layerName;
    this->cycleStart = -1;
    this->cycleEnd = -1;
  }

  void registerCycle (long long unsigned cycle) {
    if (this->cycleStart == -1) {
      this->cycleStart = cycle;
    }

    this->cycleEnd = cycle;
  }
};

static std::vector<layerProfCycle> layerProfileInfo;
static int64_t globalLayerNo = 0;
static layerProfCycle *currentLayer = NULL;


// Export these functions in C dilect.
extern "C" {
#include "Utils.h"

static long long unsigned opcodecount[OPCODE_CYCLE_ARRAY_LEN] = {0};
static long long unsigned globalCycle = 0;

void lltfiMLLayer(int64_t layerName, int64_t start) {

  assert(start == 1 || start == 2 && "Layer start is denoted by 1 and end by 2");

  int64_t *layerNamePtr = &layerName;
  char* layerNameStr = (char*)layerNamePtr;

  if (start == 1) { /* Layer started. */
    globalLayerNo++;
    currentLayer = new layerProfCycle(globalLayerNo, std::string(layerNameStr));
  }
  else {

    layerProfileInfo.push_back(*currentLayer);
    delete currentLayer;
    currentLayer = NULL;
  }
}

void doProfiling(int opcode) {
  assert(opcodecount[opcode] >= 0 &&
         "dynamic instruction number too large to be handled by llfi");
  opcodecount[opcode]++;
  globalCycle++;
  if (currentLayer != NULL)
    currentLayer->registerCycle(globalCycle);
}

void endProfiling() {
  FILE *profileFile;
  char profilefilename[80] = "llfi.stat.prof.txt";
  profileFile = fopen(profilefilename, "w");
  if (profileFile == NULL) {
    fprintf(stderr, "ERROR: Unable to open profiling result file %s\n",
            profilefilename);
    exit(1);
  }

  int opcode_cycle_arr[OPCODE_CYCLE_ARRAY_LEN];
  getOpcodeExecCycleArray(OPCODE_CYCLE_ARRAY_LEN, opcode_cycle_arr);

  unsigned i = 0;
  long long unsigned total_cycle = 0;
  for (i = 0; i < 100; ++i) {
    assert(total_cycle >= 0 &&
            "total dynamic instruction cycle too large to be handled by llfi");
    if (opcodecount[i] > 0) {
      assert(opcode_cycle_arr[i] >= 0 &&
          "opcode does not exist, need to update instructions.def");
      total_cycle += opcodecount[i] * opcode_cycle_arr[i];
    }
  }

  fprintf(profileFile, "# do not edit\n");
  fprintf(profileFile,
          "# cycle considered the execution cycle of each instruction type\n");
  fprintf(profileFile, "total_cycle=%lld\n", total_cycle);

  for (auto layer : layerProfileInfo) {
    fprintf(profileFile, "ml_layer=%d,%s,%lld,%lld\n", layer.layerNo,
            layer.layerName.c_str(), layer.cycleStart, layer.cycleEnd);
  }

	fclose(profileFile);
}
} // End of extern "C"
