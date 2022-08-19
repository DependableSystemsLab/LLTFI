## Copying the input.yaml file and executing the compile script
# Vision models
cd vision_models/

cd bvlcalexnet-12/
mv input.yaml input_orig.yaml
cp ../../input.yaml .
sh compile.sh

cd ../cnn-fmnist/
mv input.yaml input_orig.yaml
cp ../../input.yaml .
sh compile.sh

cd ../dave_keras/
mv input.yaml input_orig.yaml
cp ../../input.yaml .
sh compile.sh

cd ../googlenet-12/
mv input.yaml input_orig.yaml
cp ../../input.yaml .
sh compile.sh

cd ../inception-v1-12/
mv input.yaml input_orig.yaml
cp ../../input.yaml .
sh compile.sh

cd ../lenet-fmnist/
mv input.yaml input_orig.yaml
cp ../../input.yaml .
sh compile.sh

cd ../lenet-mnist/
mv input.yaml input_orig.yaml
cp ../../input.yaml .
sh compile.sh

cd ../mnist/
mv input.yaml input_orig.yaml
cp ../../input.yaml .
sh compile.sh

cd ../resnet50-v1-12/
mv input.yaml input_orig.yaml
cp ../../input.yaml .
sh compile.sh

cd ../shufflenetv2_10/
mv input.yaml input_orig.yaml
cp ../../input.yaml .
sh compile.sh

cd ../squeezenet1.0-12/
mv input.yaml input_orig.yaml
cp ../../input.yaml .
sh compile.sh

cd ../vgg16-12/
mv input.yaml input_orig.yaml
cp ../../input.yaml .
sh compile.sh

cd ../yolo3/
mv input.yaml input_orig.yaml
cp ../../input.yaml .
sh compile.sh


# NLP models
cd ../../nlp_models/

cd bert
mv input.yaml input_orig.yaml
cp ../../input.yaml .
sh compile.sh

cd ../gpt2
mv input.yaml input_orig.yaml
cp ../../input.yaml .
sh compile.sh

cd ../roberta-base-11
mv input.yaml input_orig.yaml
cp ../../input.yaml .
sh compile.sh

cd ../t5-encoder
mv input.yaml input_orig.yaml
cp ../../input.yaml .
sh compile.sh

cd ../..

## Executing the runllfi script
# Vision models
cd vision_models/

cd bvlcalexnet-12/
sh runllfi.sh
rm -rf input.yaml
mv input_orig.yaml input.yaml

cd ../cnn-fmnist/
sh runllfi.sh
rm -rf input.yaml
mv input_orig.yaml input.yaml

cd ../dave_keras/
sh runllfi.sh
rm -rf input.yaml
mv input_orig.yaml input.yaml

cd ../googlenet-12/
sh runllfi.sh
rm -rf input.yaml
mv input_orig.yaml input.yaml

cd ../inception-v1-12/
sh runllfi.sh
rm -rf input.yaml
mv input_orig.yaml input.yaml

cd ../lenet-fmnist/
sh runllfi.sh
rm -rf input.yaml
mv input_orig.yaml input.yaml

cd ../lenet-mnist/
sh runllfi.sh
rm -rf input.yaml
mv input_orig.yaml input.yaml

cd ../mnist/
sh runllfi.sh
rm -rf input.yaml
mv input_orig.yaml input.yaml

cd ../resnet50-v1-12/
sh runllfi.sh
rm -rf input.yaml
mv input_orig.yaml input.yaml

cd ../shufflenetv2_10/
sh runllfi.sh
rm -rf input.yaml
mv input_orig.yaml input.yaml

cd ../squeezenet1.0-12/
sh runllfi.sh
rm -rf input.yaml
mv input_orig.yaml input.yaml

cd ../vgg16-12/
sh runllfi.sh
rm -rf input.yaml
mv input_orig.yaml input.yaml

cd ../yolo3/
sh runllfi.sh
rm -rf input.yaml
mv input_orig.yaml input.yaml


# NLP models
cd ../../nlp_models/
cd bert
sh runllfi.sh
rm -rf input.yaml
mv input_orig.yaml input.yaml

cd ../gpt2
sh runllfi.sh
rm -rf input.yaml
mv input_orig.yaml input.yaml

cd ../roberta-base-11
sh runllfi.sh
rm -rf input.yaml
mv input_orig.yaml input.yaml

cd ../t5-encoder
sh runllfi.sh
rm -rf input.yaml
mv input_orig.yaml input.yaml

cd ../..
