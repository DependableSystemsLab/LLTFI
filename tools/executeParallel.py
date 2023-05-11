# Script to execute the model in parallel
import argparse, os, shutil
import json
import warnings
import sys, struct, math
from pdb import set_trace
from collections import Counter
import subprocess
import time

##### MAIN #####
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", help="Path to model's directory")
    parser.add_argument("input_start", help="What's the first input?")
    parser.add_argument("input_end", help="What's the last input?")

    args = parser.parse_args()

    # Validate inputs
    if not os.path.isdir(args.directory):
        print("Model directory does not exist")
        return

    input_start = None
    input_end = None
    try:
        input_start = int(args.input_start)
        input_end = int(args.input_end)
        assert input_start <= input_end
    except ValueError:
        print("Input start and end must be integers")
        return

    original_directory = os.path.abspath(args.directory)

    all_dirs = []
    all_processes = []

    for i in range(input_start, input_end + 1):
        # Copy model directory again
        shutil.copytree(original_directory, original_directory + "-" + str(i))
        new_dir = original_directory + "-" + str(i)

        # Validate directory
        if not os.path.isdir(new_dir):
            print("Directory copy failed")
            return

        print("Created directory: " + new_dir)

        # Change directory to new directory
        os.chdir(new_dir)

        # Creat an output file
        with open("output.txt", "w") as f:
            print("Launching process for input " + str(i) + "...")
            # execute runAllInput.sh script in this directory.
            p = subprocess.Popen(["bash", "./runAllInputs.sh", str(i)], stdout=f, stderr=f)

            # Add directory to list of directories
            all_dirs.append(new_dir)
            all_processes.append(p)

        # Change directory back to original directory
        os.chdir("..")

    # Wait for all processes to finish
    for p in all_processes:
        while p.poll() is None:
            time.sleep(5)


if __name__ == '__main__':
    main()
