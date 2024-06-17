# DSN 2024 Tutorial Activity - Instructions

## Setup

You may choose from one of three choices to use LLTFI for this tutorial.
You are also welcome to use your own installation of LLTFI, but these instructions assume you are using the Docker or Docker on VM image with specific experiment folder paths.

For Options #1 and 2, we will distribute the installation files via USB Flash Drive.

*Note: Please have at least **30 GB** of free disk space available for this tutorial.*


1. If you are using either Linux or Mac OS, please use [Docker](https://docs.docker.com/engine/install/).
   If you are using Ubuntu, please follow [these specific instructions](https://docs.docker.com/engine/install/ubuntu/#install-using-the-repository).
   You will need sudo power to run Docker, unless you have been added to a [Docker group](https://docs.docker.com/engine/install/linux-postinstall/).

    1. Copy the `lltfi-demo.tar` file to your local machine from the USB Drive.
    2. Run the following to load the saved docker image.
       ```
       docker load -i lltfi-demo.tar
       ```

2. If you are using Windows, please install [VirtualBox 7.0](https://www.virtualbox.org/wiki/Downloads) and use the provided VM image.
   The VM image has the Docker image preloaded.

   1. Copy the `xubuntu20_lltfi.ova` file to your local machine from the USB Drive.
   2. Select `File`, `Import Appliance...`.
   3. Under File, select `xubuntu20_lltfi.ova`.
   4. Adjust the RAM allocation so that it has at least 4 GB.
   5. Press `Finish` to import the VM image
   6. Once imported, start the VM and login as `osboxes.org` (Default user) with the password: `osboxes.org`.

3. If you prefer not to use a USB Flash Drive, and if you are using either Linux or Mac OS, you may also pull the Docker image directly from Docker Hub.
   ```
   docker image pull abrahamchan/lltfi-demo
   docker tag lltfi-demo abrahamchan/lltfi-demo
   ```


## Running LLTFI

1. Once you have installed Docker image, please check that the Docker image is successfully loaded by typing `docker images` in the terminal.

   ```
   >>> docker images
   REPOSITORY   TAG       IMAGE ID       CREATED        SIZE
   lltfi-demo   latest    c6e6104ffa8a   7 days ago   28.3GB
   ```

2. Run the Docker image as a container. Enable X11 forwarding to view images for visualization.

   ```
   xhost +
   docker run -it --rm -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix lltfi-demo:latest /bin/bash
   ```


## Benchmarks
We provide two benchmarks to run fault injections for this tutorial.
* **Factorial** - This C program computes the factorial of a number. The output is deterministic.
* **MNIST** - This ML model classifies handwritten images of numeric digits (0-9) and returns a list of probability scores for each digit.

We will use **Factorial** for Parts 1 - 3, and **MNIST** for Parts 4 - 5.


## Part 1: Fault Injection
1. Open a terminal and navigate to the factorial folder:
   ```
   cd sample_programs/cpp_sample_programs/factorial/
   ```

2. Compile the program and run LLTFI.
   ```
   ./compileAndRun.sh factorial 6
   ```

3. You should see LLTFI perform 5 fault injection runs on the program, and result in some crashes (i.e., Exit code -11).

4. Examine the baseline (golden) output. You see the correct, expected output of 6! = 720.
   ```
   >>> cat llfi/baseline/golden_std_output
   720
   ```

5. Examine the fault injection traces, using this command: `cat llfi/std_output/std_outputfile-run-0-[run#]`.
   Since you ran fault injection 5 times, the `run#` will start from 0 to 4.
   You may have to try different output `run#` to see the values.
   For instance, we display the fault injection run from the 4th run, which results in 6 for the factorial output - this is an SDC.
   ```
   >>> cat llfi/std_output/std_outputfile-run-0-3
   6
   ```


## Part 2: Specify Injection Targets
1. Using Vim, open the `input.yaml` file and change the target instruction type from `all` to `add` and `sub` instructions only.
   Also, increase the number of runs to 10.

```
compileOption:
    instSelMethod:
      - insttype:
          include:
            - add (Add this line)
            - sub (Add this line)
          exclude:
            - ret

    regSelMethod: regloc
    regloc: dstreg

...

runOption:
    - run:
        numOfRuns: 10 (Change this line)
        fi_type: bitflip
```

2. Execute the compile script to run LLTFI again.
   You should observe that the fault injection runs result in fewer crashes.
   Run the commands in Part 1 to examine the output files.


## Part 3: Trace Analysis and Visualization
1. First, if you look under the `llfi/baseline` folder, you should only see a single `golden_std_output` file.
   ```
   ls llfi/baseline/
   ```

2. Now, navigate to the `input.yaml` file and change `tracingPropagation` so that it is set to True. Tracing is now enabled.

3. Recompile the program and run LLTFI again.
   ```
   ./compileAndRun.sh factorial 6
   ```

4. Examine the golden trace file. You should see the values traced at each executed instruction.
   ```
   vim llfi/baseline/llfi.stat.trace.prof.txt
   ```

5. Analyze trace propagation. Modify the 3 to the desired run# number.
   ```
   /home/LLTFI/tools/tracediff.py llfi/baseline/llfi.stat.trace.prof.txt llfi/llfi_stat_output/llfi.stat.trace.0-3.txt > diffReport.txt
   /home/LLTFI/tools/traceontograph.py diffReport.txt llfi.stat.graph.dot > tracedGraph.dot
   dot -Tpng tracedGraph.dot -o tracedGraph.png
   display tracedGraph.png
   ```


## Part 4: ML Fault Injection
1. Navigate to the MNIST sample program.
   ```
   cd /home/LLTFI/sample_programs/ml_sample_programs/vision_models/mnist/
   ```

2. Change the `input.yaml` file so that fault injection is only ran 20 times. This line `numOfRuns: 1000` should be changed to `numOfRuns: 20`.

3. Compile the pre-trained Convolutional Neural Network into ONNX.
   You should see the `model.ll` file generated - this is the LLVM IR representation of the trained neural network, originally written in TensorFlow.
   ```
   ./compile.sh
   ```

4. Run LLTFI on the LLVM IR file.
   By default, an image of a handwritten eight, `eight.png`, is used as input.
   ```
   ./runllfi.sh
   ```

5. The baseline (golden) run should show the probabilty score for eight with the highest value as shown.
   In this example, `eight.png` is predicted as eight with a probability score of 0.998805.
   ```
   >>> cat llfi/baseline/golden_std_output
   Time taken to execute the model: 0.029147
   Final prediction for eight.png is: 0.000001 0.000000 0.000417 0.000076 0.000000 0.000024 0.000000 0.000001 0.998805 0.000676
   ```

6. Cycle through the run#, and examine the different fault injected runs. Modify the 3 to the desired run# number.
   ```
   cat llfi/std_output/std_outputfile-run-0-3
   ```

## Part 5: Specifying Specific Neural Network Layers
View the original model, `mnist-cnn.py`, written in TensorFlow for reference.
In this exercise, we wish to select specific layers in the model for fault injection.
Previously, `input.yaml` was configured so that many different layers were chosen at random for fault injection.

Reference code for CNN model:
```py
def get_model():
    model = models.Sequential()
    model.add(
        layers.Conv2D(
            32, (5, 5), activation="relu", input_shape=(
                28, 28, 1)))
    model.add(layers.MaxPooling2D((2, 2)))
    model.add(layers.Conv2D(64, (5, 5), activation="relu"))
    model.add(layers.MaxPooling2D((2, 2)))

    model.add(layers.Flatten())
    model.add(layers.Dense(10, activation="softmax"))

    model.compile(
        optimizer="adam",
        loss=losses.SparseCategoricalCrossentropy(),
        metrics=["accuracy"],
    )
    return model
```

1. Modify the `input.yaml` file so that we perform fault injection into the second convolution layer only.
   The number of entries, separated with semicolons, in `layerNo` must match that of the `layerName`.
   To see some observable differences in fault injection, increase the number of `fi_num_bits` to 8 to increase the fault injection intensity.
   Normally, we would run this 1000 times and automatically examine the traces.
   Rerun LLTFI with `./runllfi.sh` - there is no need to recompile again, as the model has not changed.

```
compileOption:
    instSelMethod:
      - customInstselector:
          include:
            - CustomTensorOperator
          options:
            - -layerNo=2 (Change this line)
            - -layerName=conv (Change this line)

    regSelMethod: regloc
    regloc: dstreg

    includeInjectionTrace:
        - forward

    tracingPropagation: False # trace dynamic instruction values.

    tracingPropagationOption:
        maxTrace: 250 # max number of instructions to trace during fault injection run
        debugTrace: False
        mlTrace: False # enable for tracing ML programs
        generateCDFG: True

runOption:
    - run:
        numOfRuns: 20
        fi_type: bitflip
        window_len_multiple_startindex: 1
        window_len_multiple_endindex: 500
        fi_max_multiple: 2
        fi_num_bits: 8 (Add this line)
```

2. Cycle through the run#, analyze the fault injected outputs.
   You may observe that in some runs, the higher probabiliy score is assigned to another class instead.
   In this example, fault injection run #3 resulted in `eight.png` misclassified as two instead of eight.
   ```
   >>> cat llfi/std_output/std_outputfile-run-0-3
   Final prediction for eight.png is: 0.000000 0.000000 1.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000
   ```

3. Modify the `input.yaml` file so that the dense layer is injected instead of the second convolution layer.
   Rerun LLTFI with `./runllfi.sh` - there is no need to recompile again, as the model has not changed.
```
compileOption:
    instSelMethod:
      - customInstselector:
          include:
            - CustomTensorOperator
          options:
            - -layerNo=0 (Change this line)
            - -layerName=matmul (Change this line)

    regSelMethod: regloc
    regloc: dstreg

    includeInjectionTrace:
        - forward

    tracingPropagation: False # trace dynamic instruction values.

    tracingPropagationOption:
        maxTrace: 250 # max number of instructions to trace during fault injection run
        debugTrace: False
        mlTrace: False # enable for tracing ML programs
        generateCDFG: True

runOption:
    - run:
        numOfRuns: 20
        fi_type: bitflip
        window_len_multiple_startindex: 1
        window_len_multiple_endindex: 500
        fi_max_multiple: 2
        fi_num_bits: 8
```
