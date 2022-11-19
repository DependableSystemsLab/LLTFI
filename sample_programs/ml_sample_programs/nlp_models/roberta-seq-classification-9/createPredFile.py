import numpy as np
import glob
import os
import json

ROOT = os.getcwd()
LLFI_OUT = os.path.join(ROOT, 'llfi')
PROG_OUT = os.path.join(LLFI_OUT, 'prog_output')

def main():
	# Get LLTFI outputs in listResArr
	listResArr = []
	list_of_files = sorted( filter( lambda x: os.path.isfile(os.path.join(PROG_OUT, x)),
							os.listdir(PROG_OUT) ) )

	for i in range(len(list_of_files)):
		list_of_files[i] = os.path.join(PROG_OUT, list_of_files[i])

	for filename in list_of_files:
		with open(filename, "r") as read_file:
			resultJson = json.load(read_file)

			for key, value in resultJson.items():
				listResArr.append(value['Data'])
				
	listPreds = []
	# Get Sentiment (positive/negative) prediction
	for resIndex in range(len(listResArr)):
		elem = listResArr[resIndex]
		pred = np.argmax(elem)
		if(pred == 0):
			listPreds.append(f"Run #{resIndex} Prediction: Negative\n")
		elif(pred == 1):
			listPreds.append(f"Run #{resIndex} Prediction: Positive\n")
			
	myfile = open('prediction/PredResult.txt', 'w')
	myfile.writelines(listPreds)
	myfile.close()


if __name__ == "__main__":
    main()
