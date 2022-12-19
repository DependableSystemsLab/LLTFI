import torch
import torch.nn.functional as F
from tqdm import trange
from transformers import T5Tokenizer
from onnxruntime import InferenceSession
from onnxt5.api import get_encoder_decoder_tokenizer
import numpy as np
import glob
import os
import json

ROOT = os.getcwd()
LLFI_OUT = os.path.join(ROOT, 'llfi')
PROG_OUT = os.path.join(LLFI_OUT, 'prog_output')

prompts = []
prompts.append('translate English to German: I was a victim of accidents')
prompts.append('translate English to French: Today is a bright, sunny day')
prompts.append('translate English to German: Vancouver is a beautiful city')
prompts.append('translate English to French: She sings beautifully')
prompts.append('translate English to German: The cake is very tasty')


class GenerativeT5_custom_encoder(torch.nn.Module):
    """ This wrapper utility function implements a single beam search to generate efficiently text.
        A lot of the credit goes to the huggingface team and its chief scientist Thomas Wolf whose implementation I based
        myself off.
        Args:
            encoder: huggingface encoder or onnx session for the encoder of T5. Can be obtained with the
                create_t5_encoder_decoder utility function for pytorch, see examples below.
            decoder_with_lm_head: decoder with language model head on top. Can be obtained with the
                create_t5_encoder_decoder utility function for pytorch, see examples below.
            tokenizer: huggingface tokenizer
            onnx (bool): whether to use onnx or the default pytorch
            cuda (bool): whether to use cuda or the cpu"""
    def __init__(self, encoder_outputs_prompt, decoder_with_lm_head, tokenizer, cuda=False):
        super().__init__()
        self.encoder_outputs_prompt = encoder_outputs_prompt
        self.decoder_with_lm_head = decoder_with_lm_head
        self.tokenizer = tokenizer
        self.cuda = cuda

    def forward(self, max_length, temperature=1., repetition_penalty=1., top_k=50, top_p=0, max_context_length=512):
        """ Forward function to generate text after a prompt
            Args:
                prompt: str to run (don't forget to add at the beginning the task to run such as "summarize:"
                        or "translate English to German:"
                max_context_length: maximum number of tokens to use as context
        """
        with torch.no_grad():
            new_tokens = torch.tensor(())
            new_logits = []

            repetition_penalty = repetition_penalty
            top_k = top_k
            top_p = top_p

            # The sequence now needs to start with a
            generated = torch.zeros((1,1), dtype=torch.long)
            if self.cuda and not self.onnx:
                generated = generated.cuda()


            for _ in range(max_length):
                outputs = torch.tensor(self.decoder_with_lm_head.run(None, {"input_ids": generated.cpu().numpy(),
                                                   "encoder_hidden_states": self.encoder_outputs_prompt}))
                outputs = outputs[0][0]
                next_token_logits = outputs[-1, :] / (temperature if temperature > 0 else 1.0)
                if int(next_token_logits.argmax()) == 1:
                    break
                new_logits.append(next_token_logits)
                for _ in set(generated.view(-1).tolist()):
                    next_token_logits[_] /= repetition_penalty
                if temperature == 0:  # greedy sampling:
                    next_token = torch.argmax(next_token_logits).unsqueeze(0)
                else:
                    filtered_logits = top_k_top_p_filtering(next_token_logits, top_k=top_k, top_p=top_p)
                    next_token = torch.multinomial(F.softmax(filtered_logits, dim=-1), num_samples=1)
                generated = torch.cat((generated, next_token.unsqueeze(0)), dim=1)
                new_tokens = torch.cat((new_tokens, next_token), 0)

            return self.tokenizer.decode(new_tokens), new_logits


def main():
    # Get LLTFI outputs in listResArr
    listResArr = []
    list_of_files = sorted( filter( lambda x: os.path.isfile(os.path.join(PROG_OUT, x)),
                            os.listdir(PROG_OUT) ) )

    for i in range(len(list_of_files)):
        list_of_files[i] = os.path.join(PROG_OUT, list_of_files[i])

    for filename in list_of_files:
        resforSingleInput = []
        with open(filename, "r") as read_file:
            resultJson = json.load(read_file)

        for key, value in resultJson.items():
            resforSingleInput.append(value['Data'])
        listResArr.append(resforSingleInput)
                    
    list_output_np = []
    # Reshape the output and store as numpy array
    for elem in listResArr:
        output_np = np.asarray(elem[0], dtype=np.float32)
        output_np = output_np.reshape(1,-1,768)
        list_output_np.append(output_np) 

    # Script to convert numpy output to text
    listPreds = []
    decoder_sess = InferenceSession('t5-decoder-with-lm-head-12.onnx')
    _, _, tokenizer = get_encoder_decoder_tokenizer()
    for elemIndex in range(len(list_output_np)):
        generative_t5 = GenerativeT5_custom_encoder(list_output_np[elemIndex], decoder_sess, tokenizer)
        output_text = generative_t5(30, temperature=0.)[0]
        listPreds.append(f"Run #{elemIndex} Prediction:{output_text}\n")

    myfile = open('prediction/PredResult.txt', 'w')
    myfile.writelines(listPreds)
    myfile.close()

if __name__ == "__main__":
    main()
