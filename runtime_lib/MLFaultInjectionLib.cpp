#include <vector>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <ctime>
#include <inttypes.h>

#define llu long long unsigned
#define OPTION_LENGTH 512

using namespace std;

union inputBuffer {
    uint32_t ui;
    float f;
};

// DS to hold runtime FI configuration.
struct LLTFIConfig {

  // Type of fault to inject.
  char fi_type[OPTION_LENGTH];

  // fi_cycles in which we have to do fault injection.
  vector<llu> fi_cycle;

  // number of faults to inject.
  int fi_max_multiple;

  // For emitting FI stats for a particular layer.
  int fi_ml_layer_num;
  char fi_ml_layer_name[100];

  LLTFIConfig() {
    strncpy(fi_type, "bitflip", 10);
    fi_max_multiple = 0;
    fi_ml_layer_num = -1;
    strncpy(fi_ml_layer_name, "", 100);
  }
};

static LLTFIConfig LLTFI_config;
static llu LLTFI_CurrentCycle = 0;
static int LLTFI_FICycleIndex = 0;
FILE *injectedfaultsFile = NULL;
bool LLTFI_doFI = true;

// Function to parse the runtime configuration file and
// configure the global variables.
void parseLLTFIConfigFile() {

  char ficonfigfilename[80];
  const unsigned CONFIG_LINE_LENGTH = 1024;
  char line[CONFIG_LINE_LENGTH];
  char option[OPTION_LENGTH];
  char *value = NULL;
  int fi_next_cycles_index = 0;

  // Open the runtime configuration file.
  strncpy(ficonfigfilename, "llfi.config.runtime.txt", 80);
  FILE *ficonfigFile;
  ficonfigFile = fopen(ficonfigfilename, "r");
  if (ficonfigFile == NULL) {
    fprintf(stderr, "ERROR: Unable to open llfi config file %s\n",
            ficonfigfilename);
    exit(1);
  }

  // Iterate through all the options in the runtime config file.
  while (fgets(line, CONFIG_LINE_LENGTH, ficonfigFile) != NULL) {
    if (line[0] == '#')
      continue;

    value = strtok(line, "=");
    strncpy(option, value, OPTION_LENGTH);
    value = strtok(NULL, "=");

    //debug(("option, %s, value, %s;", option, value));

    if (strcmp(option, "fi_type") == 0) {
      strncpy(LLTFI_config.fi_type, value, OPTION_LENGTH);
      if (LLTFI_config.fi_type[strlen(LLTFI_config.fi_type) - 1] == '\n')
        LLTFI_config.fi_type[strlen(LLTFI_config.fi_type) - 1] = '\0';
    }

    else if (strcmp(option, "fi_cycle") == 0) {
      LLTFI_config.fi_cycle.push_back(atoll(value));
    }

    else if (strcmp(option, "fi_max_multiple") == 0){
      LLTFI_config.fi_max_multiple = atoi(value);
    }

    else if (strcmp(option, "fi_next_cycle") == 0){
      LLTFI_config.fi_cycle.push_back(atoll(value));
    }

    // Parse FI stats for ML applications
    else if (strcmp(option, "ml_layer_name") == 0) {
      strncpy(LLTFI_config.fi_ml_layer_name, value, 100);
      if (LLTFI_config.fi_ml_layer_name[strlen(LLTFI_config.fi_ml_layer_name) - 1] == '\n')
        LLTFI_config.fi_ml_layer_name[strlen(LLTFI_config.fi_ml_layer_name) - 1] = '\0';
    }

    else if (strcmp(option, "ml_layer_number") == 0) {
        LLTFI_config.fi_ml_layer_num = atoll(value);
    }

    else {
      fprintf(stderr,
              "ERROR: Unknown option %s for LLFI runtime fault injection\n",
              option);
      exit(1);
    }
  }

  // Sanity checks
  assert(LLTFI_config.fi_type != NULL && "No fault injector selected.");
  assert(LLTFI_config.fi_cycle.size() > 0 && "No fi_cycle selected");
  assert(LLTFI_config.fi_max_multiple > 0 && "invalid fi_max_multiple in config file");
  assert((LLTFI_config.fi_ml_layer_num > 0 || LLTFI_config.fi_ml_layer_num == -1) &&
          "ml_layer_number should be grater than 0");

  // Sort the fi_cycle vector.
  sort(LLTFI_config.fi_cycle.begin(), LLTFI_config.fi_cycle.end());

  // Close the fi config file.
  fclose(ficonfigFile);
}

extern "C" {
  // This function will be called at the beginning of the main function.
  void initInjections() {

    srand(time(0));
    parseLLTFIConfigFile();

    char injectedfaultsfilename[80];
    strncpy(injectedfaultsfilename, "llfi.stat.fi.injectedfaults.txt", 80);
    injectedfaultsFile = fopen(injectedfaultsfilename, "a");
    if (injectedfaultsFile == NULL) {
      fprintf(stderr, "ERROR: Unable to open injected faults stat file %s\n",
              injectedfaultsfilename);
      exit(1);
    }
  }

  // This function will be called at the end of main() function.
  void postInjections() {
    fclose(injectedfaultsFile);
    LLTFI_doFI = false;
  }

  // Function to check if we should inject fault
  bool preFunc(long llfi_index, unsigned opcode, unsigned my_reg_index,
             unsigned total_reg_target_num) {

    if (!LLTFI_doFI) return 0;

    LLTFI_CurrentCycle++;

    // If current cycle is the FI cycle.
    if (LLTFI_CurrentCycle == LLTFI_config.fi_cycle[LLTFI_FICycleIndex]) {
      LLTFI_FICycleIndex++;

      if (LLTFI_FICycleIndex >= LLTFI_config.fi_max_multiple)
        LLTFI_doFI = false;

      return true;
    }

    return 0;
  }

  // Function to actually inject the fault.
  void injectFunc(long llfi_index, unsigned size, char *buf,
                  unsigned my_reg_index, unsigned reg_pos, char* opcode_str) {

    fprintf(stderr, "MSG: injectFunc() has being called\n");

    unsigned int fi_bytepos, fi_bitpos;
    unsigned char oldbuf;

    fi_bitpos = rand() % size;
    fi_bytepos = fi_bitpos / 8;
    oldbuf = buf[fi_bytepos];
    inputBuffer oldVal = {.f = *((float*)buf)};
    inputBuffer newVal;

    if (strcmp(LLTFI_config.fi_type, "bitflip") == 0) {

      int8_t val = buf[fi_bytepos];
      int shift = fi_bitpos % 8;

      val ^= 0x1 << shift;

      buf[fi_bytepos] = val;
    }
    else {
      assert(false && "Not recognized fi_type");
    }

    newVal = {.f = *((float*)buf)};

    if (LLTFI_config.fi_ml_layer_num > 0)
      fprintf(injectedfaultsFile,
          "FI stat: fi_type=%s, fi_max_multiple=%d, fi_index=%ld, "
          "fi_cycle=%lld, fi_reg_index=%u, fi_reg_pos=%u, fi_reg_width=%u, "
          "fi_bit=%u, opcode=%s, oldHex=0x%x, newHex=0x%x, oldFloat=%f, "
          " newFloat=%f, ml_layer_name=%s, ml_layer_number=%d\n",
           LLTFI_config.fi_type, LLTFI_config.fi_max_multiple,
           llfi_index, LLTFI_CurrentCycle, my_reg_index, reg_pos, size,
           fi_bitpos, opcode_str, oldVal.ui, newVal.ui, oldVal.f, newVal.f,
           LLTFI_config.fi_ml_layer_name, LLTFI_config.fi_ml_layer_num);
    else
      fprintf(injectedfaultsFile,
          "FI stat: fi_type=%s, fi_max_multiple=%d, fi_index=%ld, "
          "fi_cycle=%lld, fi_reg_index=%u, fi_reg_pos=%u, fi_reg_width=%u, "
          "fi_bit=%u, opcode=%s, oldHex=0x%x, newHex=0x%x, oldFloat=%f, "
          " newFloat=%f\n",
           LLTFI_config.fi_type, LLTFI_config.fi_max_multiple,
           llfi_index, LLTFI_CurrentCycle, my_reg_index, reg_pos, size,
           fi_bitpos, opcode_str, oldVal.ui, newVal.ui, oldVal.f, newVal.f);

    fflush(injectedfaultsFile);
  }
}
