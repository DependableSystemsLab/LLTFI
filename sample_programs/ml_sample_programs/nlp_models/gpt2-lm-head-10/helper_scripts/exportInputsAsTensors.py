from transformers import AutoTokenizer, GPT2Tokenizer
import os, pdb
from onnx import numpy_helper
import numpy as np

inputs = []
inputs.append("This chair is white and the table is")
inputs.append("It is bright and")
inputs.append("I am a doctor and I work at a")
inputs.append("I like playing with my")
inputs.append("A rose by any other name would smell as")
inputs.append("US-led coalition air strikes on a jail run by the Islamic State group in eastern Syria killed")
inputs.append("A magazine supplement with an image of Adolf Hitler and the title 'The Unreadable Book' is pictured in")
inputs.append("Winter isn't done with us yet. Ottawa can expect another 10 to 15 centimetres of")
inputs.append("Refined mansion tax proposal being fed into debate on abolishing 50p tax rate for those earning more than")
inputs.append("Ghazala Khan, the mother of a fallen U.S. soldier of Muslim faith, is responding to Donald Trump’s")

tokenizer = GPT2Tokenizer.from_pretrained('gpt2')

for index in range(0, len(inputs)):
  tokens = np.array(tokenizer.encode(inputs[index])) # Shape : (len,)
  input_arr = tokens.reshape(1,1,-1) # Shape : (1,1,len)
  input_tensor = numpy_helper.from_array(input_arr)
  with open("input_{}.pb".format(index), 'wb') as file:
      file.write(input_tensor.SerializeToString())
