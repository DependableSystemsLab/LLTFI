import torch
import torch.nn.functional as F
from tqdm import trange
from transformers import T5Tokenizer
from onnxruntime import InferenceSession
from onnxt5.api import get_encoder_decoder_tokenizer
import numpy as np
import glob
import os, pdb
import json
from onnx import numpy_helper

# WMT19 Dataset from huggingface
prompts = []
prompts.append('translate English to German: I declare resumed the session of the European Parliament.')
prompts.append('translate English to German: Statements by the President')
prompts.append('translate English to French: Statements by the President')
prompts.append('translate English to French: I declare resumed the session of the European Parliament.')
prompts.append('translate English to German: Ladies and gentlemen, on Saturday, as you know, an earthquake')
prompts.append('translate English to French: Ladies and gentlemen, on Saturday, as you know, an earthquake')
prompts.append('''translate English to German: (The House rose and observed a minute's silence)''')
prompts.append('''translate English to French: (The House rose and observed a minute's silence)''')
prompts.append('translate English to German: I should like, on behalf of the European Parliament, to express')
prompts.append('translate English to French: I should like, on behalf of the European Parliament, to express')

class GenerativeT5_pytorch(torch.nn.Module):
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
        Examples:
            For pytorch:
            >>> from transformers import T5Tokenizer
            >>> from onnxt5 import create_t5_encoder_decoder, GenerativeT5
            >>> pretrained_model = 't5-base' # This can be a pretrained version, or the path to a huggingface model
            >>> simplified_encoder, decoder_with_lm_head = create_t5_encoder_decoder(pretrained_model)
            >>> tokenizer = T5Tokenizer.from_pretrained(pretrained_model)
            >>> generative_t5 = GenerativeT5(simplified_encoder, decoder_with_lm_head, tokenizer)
            >>> generative_t5('translate English to French: I was a victim of a series of accidents.', 16, temperature=0.)[0]
            >>> # Output: "Je suis victime d'une série d'accidents."
            For onnx:
            >>> from transformers import T5Tokenizer
            >>> from onnxruntime import InferenceSession
            >>> from onnxt5 import GenerativeT5
            >>> decoder_sess = InferenceSession('~/t5-decoder-with-lm-head.onnx')
            >>> encoder_sess = InferenceSession('~/t5-encoder.onnx')
            >>> tokenizer = T5Tokenizer.from_pretrained(pretrained_model)
            >>> generative_t5 = GenerativeT5(encoder_sess, decoder_sess, tokenizer, onnx=True)
            >>> generative_t5('translate English to French: I was a victim of a series of accidents.', 16, temperature=0.)[0]
            >>> # Output: "Je suis victime d'une série d'accidents."
    """
    def __init__(self, encoder, decoder_with_lm_head, tokenizer, onnx=False, cuda=False):
        super().__init__()
        self.encoder = encoder
        self.decoder_with_lm_head = decoder_with_lm_head
        self.tokenizer = tokenizer
        self.onnx = onnx
        self.cuda = cuda

    def forward(self, prompt, max_length, temperature=1., repetition_penalty=1., top_k=50, top_p=0, max_context_length=512):
        """ Forward function to generate text after a prompt
            Args:
                prompt: str to run (don't forget to add at the beginning the task to run such as "summarize:"
                        or "translate English to German:"
                max_context_length: maximum number of tokens to use as context
        """
        with torch.no_grad():
            new_tokens = torch.tensor(())
            new_logits = []
            generated = torch.tensor(self.tokenizer(prompt)['input_ids'])[:max_context_length - 1].unsqueeze(0)
            if self.cuda and not self.onnx:
                generated = generated.cuda()
            temperature = temperature
            # Getting encoder past
            if self.onnx:
                encoder_outputs_prompt = self.encoder.run(None, {"input_ids": generated.cpu().numpy()})[0]
            else:
                encoder_outputs_prompt = self.encoder(generated)
            print(encoder_outputs_prompt.shape)
            repetition_penalty = repetition_penalty
            top_k = top_k
            top_p = top_p

            # The sequence now needs to start with a
            generated = torch.zeros((1,1), dtype=torch.long)
            if self.cuda and not self.onnx:
                generated = generated.cuda()

            for _ in range(max_length):
                if self.onnx:
                    outputs = torch.tensor(self.decoder_with_lm_head.run(None, {"input_ids": generated.cpu().numpy(),
                                                   "encoder_hidden_states": encoder_outputs_prompt})[0][0])
                else:
                    outputs = self.decoder_with_lm_head(input_ids=generated,
                                                        encoder_hidden_states=encoder_outputs_prompt)[0]
                print(generated)
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
                print(self.tokenizer.decode(new_tokens))

            return self.tokenizer.decode(new_tokens), new_logits


prev_dec_out = []
prev_dec_out.append(np.array([[0, 1674]])) #Ich
prev_dec_out.append(np.array([[0, 28019]]))
prev_dec_out.append(np.array([[0, 4829]]))
prev_dec_out.append(np.array([[0, 1022]]))
prev_dec_out.append(np.array([[0, 18843]]))
prev_dec_out.append(np.array([[0, 10162]]))
prev_dec_out.append(np.array([[0, 41]]))
prev_dec_out.append(np.array([[0, 41]]))
prev_dec_out.append(np.array([[0, 1674]]))
prev_dec_out.append(np.array([[0, 1957]]))

# Get encoder hidden state
def createInp(inpSample):
  global prompts, prev_dec_out

  encoder_sess = InferenceSession('t5-encoder-12.onnx')
  max_context_length = 2048
  _, _, tokenizer = get_encoder_decoder_tokenizer()
  with torch.no_grad():
      generated = torch.tensor(tokenizer(prompts[inpSample])['input_ids'])[:max_context_length - 1].unsqueeze(0)
      encoder_hidden_state = encoder_sess.run(None, {"input_ids": generated.cpu().numpy()})[0]

  inp_1_tensor = numpy_helper.from_array(prev_dec_out[inpSample])
  with open("input{}_0.pb".format(inpSample), 'wb') as file:
      file.write(inp_1_tensor.SerializeToString())

  encoder_hidden_state_tensor = numpy_helper.from_array(encoder_hidden_state)
  with open("input{}_1.pb".format(inpSample), 'wb') as file:
      file.write(encoder_hidden_state_tensor.SerializeToString())

def main():
  global prompts
  for i in range(0, 10):
    # Use encoder and decoder to generate output text
    decoder_sess = InferenceSession('t5-decoder-with-lm-head-12.onnx')
    encoder_sess = InferenceSession('t5-encoder-12.onnx')
    _, _, tokenizer = get_encoder_decoder_tokenizer()
    generative_t5 = GenerativeT5_pytorch(encoder_sess, decoder_sess, tokenizer, onnx=True)
    token, logit = generative_t5(prompts[i], 16, temperature=0.)
    #pdb.set_trace()
    createInp(i)

if __name__ == "__main__":
    main()
