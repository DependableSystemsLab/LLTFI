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
from onnx import numpy_helper

# WMT19 Dataset from huggingface
prompts = []
prompts.append('translate English to German: I declare resumed the session of the European Parliament adjourned on Friday, 15 December 2000.')
prompts.append('translate English to German: Statements by the President')
prompts.append('translate English to French: Statements by the President')
prompts.append('translate English to French: I declare resumed the session of the European Parliament adjourned on Friday, 15 December 2000.')
prompts.append('translate English to German: Ladies and gentlemen, on Saturday, as you know, an earthquake struck Central America once again, with tragic consequences. This is an area which has already been seriously affected on a number of occasions since the beginning of the twentieth century.')
prompts.append('translate English to French: Ladies and gentlemen, on Saturday, as you know, an earthquake struck Central America once again, with tragic consequences. This is an area which has already been seriously affected on a number of occasions since the beginning of the twentieth century.')
prompts.append('''translate English to German: (The House rose and observed a minute's silence)''')
prompts.append('''translate English to French: (The House rose and observed a minute's silence)''')
prompts.append('translate English to German: I should like, on behalf of the European Parliament, to express our sympathy to the parents and families of the victims.')
prompts.append('translate English to French: I should like, on behalf of the European Parliament, to express our sympathy to the parents and families of the victims.')


class GenerativeT5_custom_encoder(torch.nn.Module):
    """ Code Ref: https://github.com/abelriboulot/onnxt5
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
  global prompts

  encoder_input = []
  max_context_length = 512
  _, _, tokenizer = get_encoder_decoder_tokenizer()
  # Tokenize the prompts
  for prompt in prompts:
    encoder_input.append(torch.tensor(tokenizer(prompt)['input_ids'])[:max_context_length - 1].unsqueeze(0))

  for index in range(len(encoder_input)):
    tensor = numpy_helper.from_array(encoder_input[index].cpu().numpy())
    with open("input_{}.pb".format(index), 'wb') as file:
        file.write(tensor.SerializeToString())


if __name__ == "__main__":
    main()
