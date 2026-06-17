# DSN 2026 Tutorial Activity - Instructions (Additional NLP Activity)

This README describes an additional tutorial activity - performing fault injection into an NLP model.

## Benchmarks
* **Roberta-Seq-Clasification** - This NLP model performs sentiment analysis to determine whether text inputs are positive or negative.


## Part 3: Fault Injection into an NLP Model (15 min)
1. Navigate to the Roberta-Seq-Clasification sample program.
   ```
   cd /home/LLTFI/sample_programs/ml_sample_programs/nlp_models/roberta-seq-classification-9/
   ```
2. Change `input.yaml` so that fault injection is only ran 100 times. This line `numOfRuns: 1000` should be changed to `numOfRuns: 10`.
   For your convenience, all of these changes have been made in `input1.yaml`. We can simply copy the YAML settings by running these following commands.

   ```
   cp input1.yaml input.yaml
   ```


   ```
   runOption:
       - run:
          numOfRuns: 10 (Change this line)
          fi_type: bitflip
          ...
          fi_max_multiple: 5 (Change this line)
          fi_num_bits: 8 (Add this line)
   ```

3. Compile the pre-trained NLP model from ONNX into LLVM IR. If running this step for the first time, it will automatically download the pre-trained model.
   You should see the `model.ll` file generated - this is the LLVM IR representation of the trained neural network.
   ```
   ./compile.sh
   ```

4. After converting the ONNX model to LLVM IR i.e. after executig the compile.sh script, run fault injection experiment on the required input. Below is the command to execute the model on 'input 0'. A total of 10 different inputs (numbered 0-9) are available and can be specified in the argument.
   ```
   ./runllfiSingleInp.sh 0
   ```

  The task here is sentiment analysis and the prediction for any input is either 'Positive' or 'Negative'.
  In this example, input 0 is chosen, which corresponds to the input: "It is a bright, sunny day".
  The expected sentiment output for this input is **Positive**.


5. Execute the 'createPredFile' python file to generate prediction. The output file generated is 'prediction/PredResult.txt'. Print out the predicted output file to examine the result. You should find at least one erroneous prediction that is negative, in lieu of positive (expected output).
   ```
   python createPredFile.py && cat prediction/PredResult.txt
   ```

   Expected Example Output:
   ```
   Run #0 Prediction: Positive
   Run #1 Prediction: Positive
   Run #2 Prediction: Positive
   Run #3 Prediction: Positive
   Run #4 Prediction: Negative
   Run #5 Prediction: Positive
   Run #6 Prediction: Positive
   Run #7 Prediction: Positive
   Run #8 Prediction: Positive
   Run #9 Prediction: Positive
   ```

