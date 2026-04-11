#! /usr/bin/env python3

import os
import sys
import shutil
import yaml
import subprocess

def examineTraceFile(work_dir):
	try:
		with open(os.path.join(work_dir, 'input.yaml'), 'r') as inputyaml:
			config_dict = yaml.safe_load(inputyaml)
	except OSError:
		print ("FAIL: (ERROR) input.yaml not found! work_dir:", work_dir)
		return False
	try:
		if config_dict['compileOption']['tracingPropagation'] == True:
			## we should have trace file
			tracefile = os.path.join(work_dir, 'llfi', 'baseline', 'llfi.stat.trace.prof.txt')
			if os.path.isfile(tracefile) and os.path.getsize(tracefile):
				return True
			else:
				return False
		else:
			## Tracing option disabled, pass
			return True
	except Exception:
		## Tracing option disabled, pass
		return True


def checkLLFIDir(work_dir, target_IR, prog_input):
	llfi_dir = os.path.join(work_dir, "llfi")
	if os.path.isdir(llfi_dir) == False:
		return "FAIL: No ./llfi folder found!"
	stats_dir = os.path.join(llfi_dir, "llfi_stat_output")
	if os.path.isdir(stats_dir) == False:
		return "FAIL: No ./llfi/llfi_stat_output folder found!"
	baseline_dir = os.path.join(llfi_dir, "baseline")
	if os.path.isdir(baseline_dir) == False:
		return "FAIL: No ./llfi/baseline folder found!"
	prog_output_dir = os.path.join(llfi_dir, "prog_output")
	if os.path.isdir(prog_output_dir) == False:
		return "FAIL: No ./llfi/prog_output folder found!"
	std_output_dir = os.path.join(llfi_dir, "std_output")
	if os.path.isdir(std_output_dir) == False:
		return "FAIL: No ./llfi/std_output folder found!"

	stats = [f for f in os.listdir(stats_dir)]
	if len(stats) == 0:
		return "FAIL: No stats file found!"

	if examineTraceFile(work_dir) == False:
		return "FAIL: Tracing was enabled byt trace file not generated!"

	return "PASS"


def check_injection(*prog_list):
	r = 0
	suite = {}
	script_dir = os.path.dirname(os.path.realpath(__file__))
	testsuite_dir = os.path.join(script_dir, os.pardir)
	with open(os.path.join(testsuite_dir, "test_suite.yaml")) as f:
		try:
			suite = yaml.safe_load(f)
		except Exception:
			print("ERROR: Unable to load yaml file: test_suite.yaml", file=sys.stderr)
			return -1

	work_dict = {}
	for test in suite.get("SoftwareFaults", {}):
		if len(prog_list) == 0 or test in prog_list or "SoftwareFaults" in prog_list:
			work_dict["./SoftwareFaults/"+test] = suite["SoftwareFaults"][test]
	for test in suite.get("HardwareFaults", {}):
		if len(prog_list) == 0 or test in prog_list or "HardwareFaults" in prog_list:
			work_dict["./HardwareFaults/"+test] = suite["HardwareFaults"][test]
	for test in suite.get("BatchMode", {}):
		if len(prog_list) == 0 or test in prog_list or "BatchMode" in prog_list:
			work_dict["./BatchMode/"+test] = suite["BatchMode"][test]
	
	
	result_list = []
	for test_path in work_dict:
		inject_dir = os.path.abspath(os.path.join(testsuite_dir, test_path))
		inject_prog = suite["PROGRAMS"][work_dict[test_path]][0]
		inject_input = str(suite["INPUTS"][work_dict[test_path]])
		if test_path.startswith('./BatchMode'):
			# print("\tChecking on BatchMode:", test_path)
			models = [m for m in os.listdir(inject_dir) if os.path.isdir(os.path.join(inject_dir, m))]
			for m in models:
				subdir = os.path.join(inject_dir, m)
				# print("\t\tChecking on model:", m)
				result = checkLLFIDir(subdir, inject_prog, inject_input)
				if result != "PASS":
					break
			if len(models) == 0:
				result = "Subdirectories for failure modes not found!"
		else:
			result = checkLLFIDir(inject_dir, inject_prog, inject_input)
		if result != "PASS":
			r += 1
		record = {"name":test_path, "result":result}
		result_list.append(record)

	return r, result_list

if __name__ == "__main__":
	r, result_list = check_injection(*sys.argv[1:])
	print ("=============== Result ===============")
	for record in result_list:
		print(record["name"], "\t\t", record["result"])
	sys.exit(r)