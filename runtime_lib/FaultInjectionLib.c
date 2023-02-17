// Copyright (C) 2023 Intel Corporation (HDFIT components)
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>

#include "Utils.h"
#define OPTION_LENGTH 512
/*BEHROOZ: We assume that the maximum number of fault injection locations is 100 when
it comes to multiple bit-flip model.*/
#define MULTIPLE_CYCLE_LENGTH 100

// HDFIT: Defines required for env variable-based initialization
#define BLASFIOPSCNT_ENV_VAR "BLASFI_OPSCNT"

#define BLASFIMODE_ENV_VAR "BLASFI_MODE"
#define BLASFIMODE_NONE_CONST "NONE"
#define BLASFIMODE_TRANSIENT_CONST "TRANSIENT"

#define BLASFICORRUPTION_ENV_VAR "BLASFI_CORRUPTION"
#define BLASFICORRUPTION_NONE_CONST "NONE"
#define BLASFICORRUPTION_STUCKHIGH_CONST "STUCKHIGH"
#define BLASFICORRUPTION_STUCKLOW_CONST "STUCKLOW"
#define BLASFICORRUPTION_FLIP_CONST "FLIP"
//--------------------------------------------------------------

/*BEHROOZ: This variable keeps track of the number of next_cycles*/
static int fi_next_cycles_count = 0;
//==============================================================

/*BEHROOZ: I changed the below line to the current one to fix the fi_cycle*/
static long long curr_cycle = 1; //static long long curr_cycle = 0;

static FILE *injectedfaultsFile;

static int fiFlag = 1;	// Should we turn on fault injections ?

//TODO: this is all not thread-safe, fix if relevant
static int opcodecyclearray[OPCODE_CYCLE_ARRAY_LEN];
static bool is_fault_injected_in_curr_dyn_inst = false;
static bool reg_selected = false;

static struct {
  char fi_type[OPTION_LENGTH];
  bool fi_accordingto_cycle;
  // if both fi_cycle and fi_index are specified, use fi_cycle
  long long fi_cycle;
  long fi_index;

  // NOTE: the following config are randomly generated if not specified
  // in practice, use the following two configs only when you want to reproduce
  // a previous fault injection experiment
  int fi_reg_index;
  int fi_bit;
  //======== Add number of corrupted bits QINING @MAR 13th========
  int fi_num_bits;
  //==============================================================
  //======== Add second corrupted regs QINING @MAR 27th===========
  long long fi_second_cycle;
  //==============================================================
  /*BEHROOZ: Add multiple corrupted regs*/
  int fi_max_multiple; //JUNE 3rd
  long long fi_next_cycles[MULTIPLE_CYCLE_LENGTH];
  //==============================================================
} config = {"bitflip", false, -1, -1, -1, -1, 1, -1, -1, {-1}}; 
// -1 to tell the value is not specified in the config file

// declaration of the real implementation of the fault injection function
void injectFaultImpl(const char *fi_type, long llfi_index, unsigned size,
                       unsigned fi_bit, char *buf);

/**
 * private functions
 */
void _initRandomSeed() {
  unsigned int seed;
	FILE* urandom = fopen("/dev/urandom", "r");
	fread(&seed, sizeof(int), 1, urandom);
	fclose(urandom);
	srand(seed);
}

// get whether to make decision based on probability
// return true at the probability of the param: probability
bool _getDecision(double probability) {
  return (rand() / (RAND_MAX * 1.0)) <= probability;
}

// HDFIT: Re-using some utility code from our OpenBLAS implementation
unsigned long long _rand_uint128() {
	unsigned long long val = 0;
	for (size_t i=0; i<9; i++) {
		// Calling rand() 9 times, leveraging 15 bits at a time
		// RAND_MAX is guaranteed to be at least 32767
		// Multiplying val by RAND_MAX + 1 is equivalent to bit-shifting
		// by RAND_MAX'2 bit width - assuming it is a power of 2 - 1
		// After that, we can sum (or bitwise OR) to a new rand() call

		// coverity[DC.WEAK_CRYPTO]
		val = val * ((unsigned long long)RAND_MAX + 1) + rand();
	}
	return val;
}

void _parseHDFITVariables() {
	char* env_buf = NULL;
	long long totCycles = 0;
	config.fi_accordingto_cycle = true;
	config.fi_cycle = -1;
	config.fi_bit = -1;

	// Parsing total ops counter
	if(env_buf = getenv(BLASFIOPSCNT_ENV_VAR)) {
		totCycles = atoll(env_buf);
	} else {
		printf("%s environment variable uninitialized!\n", BLASFIOPSCNT_ENV_VAR);
		return;
	}

	// Parsing type of corruption
	if(env_buf = getenv(BLASFICORRUPTION_ENV_VAR)) {
		if(strcmp(env_buf, BLASFICORRUPTION_STUCKHIGH_CONST) == 0) {
			strncpy(config.fi_type, "stuck_at_1", OPTION_LENGTH);
		} else if(strcmp(env_buf, BLASFICORRUPTION_STUCKLOW_CONST) == 0) {
			strncpy(config.fi_type, "stuck_at_0", OPTION_LENGTH);
		} else if(strcmp(env_buf, BLASFICORRUPTION_FLIP_CONST) == 0) {
			strncpy(config.fi_type, "bitflip", OPTION_LENGTH);
		} else {
			printf("Invalid %s setting for environment variable %s!\n", env_buf, BLASFICORRUPTION_ENV_VAR);
			exit(-1);
		}
	} else {
		printf("%s environment variable uninitialized!\n", BLASFICORRUPTION_ENV_VAR);
		return;
	}

	// Parsing FI mode (just transient or disabled)
        if(env_buf = getenv(BLASFIMODE_ENV_VAR)) {
                if(strcmp(env_buf, BLASFIMODE_TRANSIENT_CONST) == 0) {
                        config.fi_cycle = totCycles>0 ? 1 + (_rand_uint128() % totCycles) : -1;
                } else if(strcmp(env_buf, BLASFIMODE_NONE_CONST) != 0) {
                        printf("Invalid %s setting for environment variable %s!\n", env_buf, BLASFIMODE_ENV_VAR);
                        exit(-1);
                }
        } else {
                printf("%s environment variable uninitialized!\n", BLASFIMODE_ENV_VAR);
                return;
        }
}

void _printHDFITOpsCnt() {
        printf("[HDFIT]\t Rank 0: OpsCnt = %lld\n", curr_cycle-1);
        fflush(stdout);
}

void _printHDFITVariables(long long fi_cycle, int fi_bit, unsigned size, char* opcode) {
#ifndef HDFIT_DOUBLE
	int fi_width = 32;
#else // HDFIT_DOUBLE
	int fi_width = 64;
#endif
	printf("[HDFIT]\t\t FI enabled on rank = 0\n");
	if(fi_cycle > 0)
	{
		printf("[HDFIT]\t\t FI at op = %lld\n", fi_cycle-1);
		printf("[HDFIT]\t\t Bit pos = %d\n", fi_bit%fi_width);
		printf("[HDFIT]\t\t Raw bit pos = %d\n", fi_bit);
		printf("[HDFIT]\t\t Size = %d\n", size);
		printf("[HDFIT]\t\t Op code = %s\n", opcode);
	}
	fflush(stdout);
}
//-------------------------------------------------------------------

/**
 * external libraries
 */
// HDFIT: Replacing standard configuration parsing with HDFIT interface
void initInjections() {
  _initRandomSeed();
  _parseHDFITVariables();
  // Registering function to print ops count with atexit
  atexit(_printHDFITOpsCnt);
  getOpcodeExecCycleArray(OPCODE_CYCLE_ARRAY_LEN, opcodecyclearray);
  start_tracing_flag = TRACING_FI_RUN_INIT; //Tell instTraceLib that we are going to inject faults
}
//--------------------------------------------------------------------

// HDFIT: simplified preFunc to improve performance
bool preFunc(long llfi_index, unsigned opcode, unsigned my_reg_index,
             unsigned total_reg_target_num) {

	reg_selected = false;
	if (config.fi_cycle == curr_cycle && !is_fault_injected_in_curr_dyn_inst) {
	// each register target of the instruction get equal probability of getting
	// selected. the idea comes from equal probability of drawing lots
	// NOTE: if fi_reg_index specified, use it, otherwise, randomly generate
		if (config.fi_reg_index >= 0)
			reg_selected = (my_reg_index == config.fi_reg_index);
		else
			reg_selected = _getDecision(1.0 / (total_reg_target_num - my_reg_index));

		if (reg_selected) {
			//debug(("selected reg index %u\n", my_reg_index));
			is_fault_injected_in_curr_dyn_inst = true;
		}
	}

	if (my_reg_index == 0) {
		is_fault_injected_in_curr_dyn_inst = false;
		curr_cycle++;
	}

	return reg_selected;
}
//-------------------------------------------------

void injectFunc(long llfi_index, unsigned size, 
                char *buf, unsigned my_reg_index, unsigned reg_pos, char* opcode_str) {
  fprintf(stderr, "MSG: injectFunc() has being called\n");
  if (! fiFlag) return;
  start_tracing_flag = TRACING_FI_RUN_FAULT_INSERTED; //Tell instTraceLib that we have injected a fault

// HDFIT: Optionally re-initializing seed for apps that need it
#ifdef HDFIT_SRAND
  _initRandomSeed();
#endif
//------------------------------------------------------------

  unsigned fi_bit, fi_bytepos, fi_bitpos;
  unsigned char oldbuf;
  
  //======== Add opcode_str QINING @MAR 11th========
  unsigned fi_num_bits;
  fi_num_bits = config.fi_num_bits;
  char* score_board = (char*) calloc (size, sizeof(char));
  //================================================
  //======== Add opcode_str QINING @MAR 11th========
  int runs =0;
  /*BEHROOZ: We give value to fi_cycle_to_print because we want to make sure that the 
    fi_cycle that is printed in the for loop has the correct value when it 
    comes to cases where we want to both inject in more than one bit and also
    inject in more than one location.*/
  long long fi_cycle_to_print = config.fi_cycle;
  //================================================
  for(runs = 0; runs < fi_num_bits && runs < size; runs++){
  	  // NOTE: if fi_bit specified, use it, otherwise, randomly generate
	  if (config.fi_bit >= 0)
	    fi_bit = config.fi_bit;
	  else
	  {
	    //======== Add opcode_str QINING @MAR 11th========
	    do{
	    	fi_bit = rand() / (RAND_MAX * 1.0) * size;
	    }while(score_board[fi_bit] == 1);
	    score_board[fi_bit] = 1;
	    //================================================
	  }
	  assert (fi_bit < size && "fi_bit larger than the target size");
	  fi_bytepos = fi_bit / 8;
	  fi_bitpos = fi_bit % 8;
	  
	  memcpy(&oldbuf, &buf[fi_bytepos], 1);
	
	  //======== Add opcode_str QINING @MAR 11th========
// HDFIT: Using custom print function instead of default file
	  _printHDFITVariables(config.fi_cycle, fi_bit, size, opcode_str);
/*
	  fprintf(injectedfaultsFile, 
          "FI stat: fi_type=%s, fi_max_multiple=%d, fi_index=%ld, fi_cycle=%lld, fi_reg_index=%u, "
          "fi_reg_pos=%u, fi_reg_width=%u, fi_bit=%u, opcode=%s\n", config.fi_type, config.fi_max_multiple,
          llfi_index, fi_cycle_to_print, my_reg_index, reg_pos, size, fi_bit, opcode_str);
*/
	  /*BEHROOZ: The below line is substituted with the above one as there was an 
           issue when we wanted to both inject in multiple bits and multiple
           locations.
           llfi_index, config.fi_cycle, my_reg_index, reg_pos, size, fi_bit, opcode_str);*/
	  //===============================================================
	  //fflush(injectedfaultsFile);
	  //===============================================================
//-----------------------------------------------------------

	  //======== Add second corrupted regs QINING @MAR 27th===========
	  //update the fi_cycle to the fi_second_cycle,
	  // so later procedures can still use fi_cycle to print stat info
	  if(config.fi_second_cycle != -1)
	  {
	  	config.fi_cycle = config.fi_second_cycle;
	  	config.fi_second_cycle = -1;
	  }
          /*BEHROOZ: Add multiple corrupted regs*/
          else
          {
              long long next_cycle = -1;
              int index = 0;
              for(index = 0; index < fi_next_cycles_count && next_cycle == -1; index++)
              {
                   if(config.fi_next_cycles[index] != -1)
                   {
                        next_cycle = config.fi_next_cycles[index];
	                config.fi_cycle = next_cycle;
	  	        config.fi_next_cycles[index] = -1;
                   }                   
              }
          }
	  //==============================================================
  	  injectFaultImpl(config.fi_type, llfi_index, size, fi_bit, buf);
  }
  //==================================================
  /*
  debug(("FI stat: fi_type=%s, fi_index=%ld, fi_cycle=%lld, fi_reg_index=%u, "
         "fi_bit=%u, size=%u, old=0x%hhx, new=0x%hhx\n", config.fi_type,
            llfi_index, config.fi_cycle, my_reg_index, fi_bit, 
            size,  oldbuf, buf[fi_bytepos]));
*/
}

void turnOffInjections() {
	fiFlag = 0;
}

void turnOnInjections() {
	fiFlag = 1;
}

// HDFIT: dropping fclose for output file
void postInjections() {
       //fclose(injectedfaultsFile);
}

//------------------------------------------------------------
