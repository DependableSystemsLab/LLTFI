wget -q https://storage.googleapis.com/bert_models/2018_10_18/uncased_L-12_H-768_A-12.zip
unzip uncased_L-12_H-768_A-12.zip

mkdir squad-1.1 out
wget -O squad-1.1/dev-v1.1.json  https://rajpurkar.github.io/SQuAD-explorer/dataset/dev-v1.1.json

python3 createJsonOut.py $1
