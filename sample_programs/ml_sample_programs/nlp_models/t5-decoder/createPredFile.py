import torch
import torch.nn.functional as F
from tqdm import trange
from transformers import T5Tokenizer
from onnxruntime import InferenceSession
from onnxt5.api import get_encoder_decoder_tokenizer
import onnxt5
from onnxt5 import GenerativeT5
import numpy as np
from onnx import numpy_helper
import glob
import os
import json
import sys

prompts = []
prompts.append('translate English to German: I was a victim of accidents')
prompts.append('translate English to German: Today is a bright, sunny day')

prev_dec_out = []
prev_dec_out.append(torch.tensor([[0, 1674]]))
prev_dec_out.append(torch.tensor([[    0,     3, 16239,   229]]))

prev_out = []
prev_out.append("Ich")
prev_out.append("Heute ist")

ROOT = os.getcwd()
LLFI_OUT = os.path.join(ROOT, 'llfi')
PROG_OUT = os.path.join(LLFI_OUT, 'prog_output')


# Ref: https://github.com/abelriboulot/onnxt5
### Helper class to get decoder output
class GenerativeT5_decoder(torch.nn.Module):
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
            cuda (bool): whether to use cuda or the cpu
    """
    def __init__(self, encoder_hidden_state, dec_outputs, tokenizer, onnx=False, cuda=False):
        super().__init__()
        self.encoder_hidden_state = encoder_hidden_state
        self.dec_outputs = dec_outputs
        self.tokenizer = tokenizer
        self.onnx = onnx
        self.cuda = cuda

    def forward(self, max_length, prev_decOut_tensor, temperature=1., repetition_penalty=1., top_k=50, top_p=0, max_context_length=512):
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

            generated = prev_decOut_tensor
            if self.cuda and not self.onnx:
                generated = generated.cuda()
            for _ in trange(max_length):
                outputs = self.dec_outputs[0]
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


def main(inpSample):

    encoder_sess = InferenceSession('t5-encoder-12.onnx')
    max_context_length = 512
    _, _, tokenizer = get_encoder_decoder_tokenizer()
    with torch.no_grad():
        generated = torch.tensor(tokenizer(prompts[inpSample])['input_ids'])[:max_context_length - 1].unsqueeze(0)
        encoder_hidden_state = encoder_sess.run(None, {"input_ids": generated.cpu().numpy()})[0]
    
    # Convert lltfi output to text        
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
            output_dec_np = np.asarray(elem[0], dtype=np.float32)
            output_dec_np = output_dec_np.reshape(1,-1,32128)
            output_dec_tensor = torch.from_numpy(output_dec_np)
            list_output_np.append(output_dec_tensor) 
            
    # Get predictions
    final_out_list = []
    for elemIndex in range(len(list_output_np)):
        _, _, tokenizer = get_encoder_decoder_tokenizer()
        generative_t5 = GenerativeT5_decoder(encoder_hidden_state, list_output_np[elemIndex], tokenizer, onnx=True)
        tokens, logits = generative_t5(1, prev_dec_out[inpSample], temperature=0.)
        final_out = prev_out[inpSample] + " " + tokens
        final_out_list.append(f"Run #{elemIndex} Prediction:{final_out}\n")
        
    myfile = open('prediction/PredResult.txt', 'w')
    myfile.writelines(final_out_list)
    myfile.close()
    
if __name__ == "__main__":
    inpSample = int(sys.argv[1])
    main(inpSample)
        
