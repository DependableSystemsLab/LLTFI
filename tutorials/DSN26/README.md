# DSN 2026 Tutorial Activity - Instructions

## Setup (10 min)

You may choose from one of three choices to use LLTFI for this tutorial.
You are also welcome to use your own installation of LLTFI, but these instructions assume you are using the Docker or Docker on VM image with specific experiment folder paths.

> [!NOTE]
> Please have at least **18 GB** of free disk space available for this tutorial.


1. If you are using either Linux or Mac OS, please use [Docker](https://docs.docker.com/engine/install/).
   If you are using Ubuntu, please follow [these specific instructions](https://docs.docker.com/engine/install/ubuntu/#install-using-the-repository).
   You will need sudo power to run Docker, unless you have been added to a [Docker group](https://docs.docker.com/engine/install/linux-postinstall/).

   This Docker image is about 5GB (compressed) on Docker Hub, which will need to be downloaded over the network. Once downloaded locally, it will be extracted and expand to 18GB.

   ```
   sudo docker image pull abrahamchan/lltfi-dsn26
   sudo docker tag abrahamchan/lltfi-dsn26 lltfi-dsn26
   ```


## Running LLTFI (1 min)

1. Once you have installed Docker image, please check that the Docker image is successfully loaded by typing `sudo docker images` in the terminal.

   ```
   >>> sudo docker images
   REPOSITORY   TAG       IMAGE ID       CREATED        SIZE
   lltfi-dsn26  latest    b0c7d6e417c2   7 days ago     16.6GB
   ```

2. Run the Docker image as a container.

   ```
   sudo docker run -it lltfi-dsn26 /bin/bash
   ```


## Benchmarks
We provide a benchmark to run fault injections for this tutorial.
* **MNIST** - This vision ML model classifies handwritten images of numeric digits (0-9) and returns a list of probability scores for each digit.


## Part 1: Fault Injection into a Vision ML Model (10 min)
1. Navigate to the MNIST sample program.
   ```
   cd /home/LLTFI/sample_programs/ml_sample_programs/vision_models/mnist/
   ```

2. Print out the `input.yaml` file to see the default fault injection options. Originally, the number of runs is set to 1000 times.
   ```
   cat input.yaml
   ```

   Snippet from `input.yaml` file:

   ```yaml
   runOption:
      - run:
            numOfRuns: 1000
            fi_type: bitflip
   ```

3. We modify the `input.yaml` file so that the number of runs is reduced to 100 times. The new version of this file can be copied in at `input1.yaml`.
   ```
   cp input1.yaml input.yaml
   ```

   Snippet from `input.yaml` file:
   ```
   runOption:
      - run:
            numOfRuns: 100 (This line has changed from 1000 to 100.)
            fi_type: bitflip
   ```

4. Compile the pre-trained Convolutional Neural Network into ONNX.
   You should see the `model.ll` file generated - this is the LLVM IR representation of the trained neural network, originally written in TensorFlow.
   ```
   ./compile.sh
   ```

> [!NOTE]
> You may initially see a compile error, because LLTFI attempts to compile it with `clang`, a C compiler.
> Once that initial compile fails, it will recompile with the C++ compiler, `clang++`, which should succeed.


5. Run LLTFI on the LLVM IR file.
   By default, an image of a handwritten eight, `eight.png`, is used as input.
   ```
   ./runllfi.sh
   ```

6. The baseline (golden) run should show the probabilty score for eight with the highest value as shown.
   In this example, `eight.png` is predicted as eight with a probability score of 0.998805.
   ```
   >>> cat llfi/baseline/golden_std_output
   Time taken to execute the model: 0.029147
   Final prediction for eight.png is: 0.000001 0.000000 0.000417 0.000076 0.000000 0.000024 0.000000 0.000001 0.998805 0.000676
   ```

7. Compute the number of SDCs and critical SDCs encountered. SDCs are those that lead to deviations in the output, while critical SDCs are a subset of SDCs that lead to a different ML prediction.
   Note that with the default fault injection settings, it is possible to see no erroneous outputs - single bit flips may be masked by ML models.
   ```
   >>> ./check_sdc.sh
   Total Number of Runs: 100
   Number of SDCs: 14
   ```

   ```
   >>> python check_critical_sdc.py
   Total Number of Runs: 100
   Number of Critical SDCs: 3
   ```

   You can also cycle through the run#, and examine the different fault injected runs. Modify the 3 to the desired run# number.
   In this example, `eight.png` is mispredicted as 2, with a probabilility score of 1.000000.
   ```
   >>> cat llfi/std_output/std_outputfile-run-0-3
   Time taken to execute the model: 0.026183
   Final prediction for eight.png is: 0.000000 0.000000 1.00000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000 0.000000
   ```

> [!NOTE]
> If you do not see any SDCs in any of the runs, you may rerun the experiment by calling `./runllfi.sh` (Step 5).
> Each time you run `./runllfi.sh`, it will perform a new independent set of fault injection runs (using a new random seed) and overwrite the generated files from prior runs.
> The number of SDCs will always be greater or equal to the number of critical SDCs. Critical SDCs are a subset of SDCs.

8. Compare the output between the golden baseline and the fault injected run of interest. Modify the 3 to the desired run# number as above.
   You may ignore everything except for the final line. The previous lines include logging information such as time elapsed that expectedly changes between runs.
   ```
   vimdiff llfi/baseline/golden_std_output llfi/std_output/std_outputfile-run-0-3
   ```

## Part 2: Specifying Specific Neural Network Layers and Increasing Fault Intensity (5 min)

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

   For your convenience, all of these changes have been made in `input2.yaml`. We can simply copy the YAML settings by running these following commands.
   ```
   cp input2.yaml input.yaml
   ./runllfi.sh
   ```

> [!CAUTION]
> Each time you run `./runllfi.sh`, it will perform a new independent set of fault injection runs and overwrite the generated files from prior runs.


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
        numOfRuns: 100
        fi_type: bitflip
        window_len_multiple_startindex: 1
        window_len_multiple_endindex: 500
        fi_max_multiple: 5 (Change this line)
        fi_num_bits: 8 (Add this line)
```

2. Analyze the number of SDCs and critical SDCs under increased fault intensity. The values for SDCs and critical SDCs should increase compared to Part 1.
   ```
   >>> ./check_sdc.sh
   ...
   Total Number of Runs: 100
   Number of SDCs: 28
   ```

   ```
   >>> python check_critical_sdc.py
   ...
   Total Number of Runs: 100
   Number of Critical SDCs: 11
   ```

