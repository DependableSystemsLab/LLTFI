# File to download the ONNX files for the fine-tuned models.
from datasets import load_dataset
from transformers import AutoTokenizer
import os, pdb
from transformers import DefaultDataCollator
from transformers import AutoModelForMaskedLM, TrainingArguments, Trainer
from onnx import numpy_helper
from transformers import DataCollatorForLanguageModeling

models = [
"aajrami/bert-sr-base",
"aajrami/bert-sr-medium",
"aajrami/bert-sr-small",
"aajrami/bert-mlm-base",
"aajrami/bert-mlm-medium",
"aajrami/bert-mlm-small",
"aajrami/bert-fc-base",
"aajrami/bert-fc-medium",
"aajrami/bert-fc-small",
"aajrami/bert-ascii-base",
"aajrami/bert-ascii-medium",
"aajrami/bert-ascii-small",
"aajrami/bert-rand-base",
"aajrami/bert-rand-medium",
"aajrami/bert-rand-small"
]

inputs = [
    "Balance and general perception of movement is handled by the <mask> in your inner ear",
    "The <mask> is the largest organ in the human body",
    "One of the defining features of <mask> (the phylum that sea urchins belong to) is radial symmetry.",
    "Not my field, but let me offer one possible point of <mask>:",
    "Inertial reference frames are, by definition, inertial. Rotation is a kind of <mask>."
]

block_size = 128

def group_texts(examples):
    # Concatenate all texts.
    concatenated_examples = {k: sum(examples[k], []) for k in examples.keys()}
    total_length = len(concatenated_examples[list(examples.keys())[0]])
    # We drop the small remainder, we could add padding if the model supported it instead of this drop, you can
    # customize this part to your needs.
    if total_length >= block_size:
        total_length = (total_length // block_size) * block_size
    # Split by chunks of block_size.
    result = {
        k: [t[i : i + block_size] for i in range(0, total_length, block_size)]
        for k, t in concatenated_examples.items()
    }
    result["labels"] = result["input_ids"].copy()
    return result

def preprocess_function(examples):
    return tokenizer([" ".join(x) for x in examples["answers.text"]])

eli5 = load_dataset("eli5", split="train_asks[:5000]")
eli5 = eli5.train_test_split(test_size=0.2)
eli5 = eli5.flatten()

for ii in range(0, len(models)):
    modelName = models[ii]
    tokenizer = AutoTokenizer.from_pretrained(modelName)

    tokenized_eli5 = eli5.map(
        preprocess_function,
        batched=True,
        num_proc=4,
        remove_columns=eli5["train"].column_names,
    )

    lm_dataset = tokenized_eli5.map(group_texts, batched=True, num_proc=4)

    tokenizer.pad_token = tokenizer.eos_token
    data_collator = DataCollatorForLanguageModeling(tokenizer=tokenizer, mlm_probability=0.15)
    model = AutoModelForMaskedLM.from_pretrained(modelName).to("cuda")

    training_args = TrainingArguments(
        output_dir="udit-models",
        evaluation_strategy="epoch",
        learning_rate=2e-5,
        per_device_train_batch_size=16,
        per_device_eval_batch_size=16,
        num_train_epochs=10,
        weight_decay=0.01
    )

    trainer = Trainer(
        model=model,
        args=training_args,
        train_dataset=lm_dataset["train"],
        eval_dataset=lm_dataset["test"],
        tokenizer=tokenizer,
        data_collator=data_collator,
    )

    trainer.train()

    modelName = modelName.replace('/', '-') + 'fine-tuned'

    tokenizer.save_pretrained(modelName)
    model = model.to("cpu")
    model.save_pretrained(modelName)

    # Convert the model to ONNX
    os.system("python3 -m transformers.onnx --model=" + modelName + ' --feature=masked-lm ' + str(modelName + '-onnx'))
    os.system("rm -rf " + str(modelName))
    os.chdir(str(modelName + '-onnx'))
    tokenizer = AutoTokenizer.from_pretrained(".")

    # Convert inputs to .pb file
    for j in range(0, len(inputs)):
        inp = inputs[j]
        inp = tokenizer(inp, return_tensors="np")
        #pdb.set_trace()
        input_ids = numpy_helper.from_array(inp['input_ids'])
        attention_mask = numpy_helper.from_array(inp['attention_mask'])

        # Convert to torch tensors
        with open(os.path.join("", f"input{j}_0.pb"), "wb") as f:
            f.write(input_ids.SerializeToString())

        with open(os.path.join("", f"input{j}_1.pb"), "wb") as f:
            f.write(attention_mask.SerializeToString())

    os.chdir('..')