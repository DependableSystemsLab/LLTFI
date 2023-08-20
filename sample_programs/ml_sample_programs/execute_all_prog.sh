if [ "$1" = "" ]; then
    echo "ERROR: Argument missing. Pass either 'compile' or 'run' option"
fi

## Downloading and building the models
if [ "$1" = "compile" ] || [ "$1" = "Compile" ]; then
	## Vision Models
	cd vision_models/

	cd bvlcalexnet-12/
	sh compile.sh

	cd ../cnn-fmnist/
	sh compile.sh

	cd ../dave_keras/
	sh compile.sh

	cd ../googlenet-12/
	sh compile.sh

	cd ../inception-v1-12/
	sh compile.sh

	cd ../lenet-fmnist/
	sh compile.sh

	cd ../lenet-mnist/
	sh compile.sh

	cd ../mnist/
	sh compile.sh

	cd ../resnet50-v1-12/
	sh compile.sh

	cd ../shufflenetv2_10/
	sh compile.sh

	cd ../squeezenet1.0-12/
	sh compile.sh

	cd ../vgg16-12/
	sh compile.sh

	cd ../yolo3/
	sh compile.sh


	# NLP models
	cd ../../nlp_models/

	cd bert
	sh compile.sh

	cd ../gpt2
	sh compile.sh

	cd ../roberta-base-11
	sh compile.sh

	cd ../t5-encoder
	sh compile.sh

	cd ../..
fi


## Copying the input YAML file and executing the runllfi script
if [ "$1" = "run" ] || [ "$1" = "Run" ]; then
	# Vision models
	cd vision_models/

	cd bvlcalexnet-12/
	if [ -f model.ll ]; then
		mv input.yaml input_orig.yaml
		cp ../../input.yaml .
		sh runllfi.sh
		rm -rf input.yaml
		mv input_orig.yaml input.yaml

	else
		echo "ERROR: Model not built. Execute the script with 'compile' option"
		exit 1
	fi

	cd ../cnn-fmnist/
	if [ -f model.ll ]; then
		mv input.yaml input_orig.yaml
		cp ../../input.yaml .
		sh runllfi.sh
		rm -rf input.yaml
		mv input_orig.yaml input.yaml

	else
		echo "ERROR: Model not built. Execute the script with 'compile' option"
		exit 1
	fi

	cd ../dave_keras/
	if [ -f model.ll ]; then
		mv input.yaml input_orig.yaml
		cp ../../input.yaml .
		sh runllfi.sh
		rm -rf input.yaml
		mv input_orig.yaml input.yaml

	else
		echo "ERROR: Model not built. Execute the script with 'compile' option"
		exit 1
	fi

	cd ../googlenet-12/
	if [ -f model.ll ]; then
		mv input.yaml input_orig.yaml
		cp ../../input.yaml .
		sh runllfi.sh
		rm -rf input.yaml
		mv input_orig.yaml input.yaml

	else
		echo "ERROR: Model not built. Execute the script with 'compile' option"
		exit 1
	fi

	cd ../inception-v1-12/
	if [ -f model.ll ]; then
		mv input.yaml input_orig.yaml
		cp ../../input.yaml .
		sh runllfi.sh
		rm -rf input.yaml
		mv input_orig.yaml input.yaml

	else
		echo "ERROR: Model not built. Execute the script with 'compile' option"
		exit 1
	fi

	cd ../lenet-fmnist/
	if [ -f model.ll ]; then
		mv input.yaml input_orig.yaml
		cp ../../input.yaml .
		sh runllfi.sh
		rm -rf input.yaml
		mv input_orig.yaml input.yaml

	else
		echo "ERROR: Model not built. Execute the script with 'compile' option"
		exit 1
	fi

	cd ../lenet-mnist/
	if [ -f model.ll ]; then
		mv input.yaml input_orig.yaml
		cp ../../input.yaml .
		sh runllfi.sh
		rm -rf input.yaml
		mv input_orig.yaml input.yaml

	else
		echo "ERROR: Model not built. Execute the script with 'compile' option"
		exit 1
	fi

	cd ../mnist/
	if [ -f model.ll ]; then
		mv input.yaml input_orig.yaml
		cp ../../input.yaml .
		sh runllfi.sh
		rm -rf input.yaml
		mv input_orig.yaml input.yaml

	else
		echo "ERROR: Model not built. Execute the script with 'compile' option"
		exit 1
	fi

	cd ../resnet50-v1-12/
	if [ -f model.ll ]; then
		mv input.yaml input_orig.yaml
		cp ../../input.yaml .
		sh runllfi.sh
		rm -rf input.yaml
		mv input_orig.yaml input.yaml

	else
		echo "ERROR: Model not built. Execute the script with 'compile' option"
		exit 1
	fi

	cd ../shufflenetv2_10/
	if [ -f model.ll ]; then
		mv input.yaml input_orig.yaml
		cp ../../input.yaml .
		sh runllfi.sh
		rm -rf input.yaml
		mv input_orig.yaml input.yaml

	else
		echo "ERROR: Model not built. Execute the script with 'compile' option"
		exit 1
	fi

	cd ../squeezenet1.0-12/
	if [ -f model.ll ]; then
		mv input.yaml input_orig.yaml
		cp ../../input.yaml .
		sh runllfi.sh
		rm -rf input.yaml
		mv input_orig.yaml input.yaml

	else
		echo "ERROR: Model not built. Execute the script with 'compile' option"
		exit 1
	fi

	cd ../vgg16-12/
	if [ -f model.ll ]; then
		mv input.yaml input_orig.yaml
		cp ../../input.yaml .
		sh runllfi.sh
		rm -rf input.yaml
		mv input_orig.yaml input.yaml

	else
		echo "ERROR: Model not built. Execute the script with 'compile' option"
		exit 1
	fi

	cd ../yolo3/
	if [ -f model.ll ]; then
		mv input.yaml input_orig.yaml
		cp ../../input.yaml .
		sh runllfi.sh
		rm -rf input.yaml
		mv input_orig.yaml input.yaml

	else
		echo "ERROR: Model not built. Execute the script with 'compile' option"
		exit 1
	fi


	# NLP models
	cd ../../nlp_models/

	cd bert
	if [ -f model.ll ]; then
		mv input.yaml input_orig.yaml
		cp ../../input.yaml .
		sh runllfi.sh
		rm -rf input.yaml
		mv input_orig.yaml input.yaml

	else
		echo "ERROR: Model not built. Execute the script with 'compile' option"
		exit 1
	fi

	cd ../gpt2
	if [ -f model.ll ]; then
		mv input.yaml input_orig.yaml
		cp ../../input.yaml .
		sh runllfi.sh
		rm -rf input.yaml
		mv input_orig.yaml input.yaml

	else
		echo "ERROR: Model not built. Execute the script with 'compile' option"
		exit 1
	fi

	cd ../roberta-base-11
	if [ -f model.ll ]; then
		mv input.yaml input_orig.yaml
		cp ../../input.yaml .
		sh runllfi.sh
		rm -rf input.yaml
		mv input_orig.yaml input.yaml

	else
		echo "ERROR: Model not built. Execute the script with 'compile' option"
		exit 1
	fi

	cd ../t5-encoder
	if [ -f model.ll ]; then
		mv input.yaml input_orig.yaml
		cp ../../input.yaml .
		sh runllfi.sh
		rm -rf input.yaml
		mv input_orig.yaml input.yaml

	else
		echo "ERROR: Model not built. Execute the script with 'compile' option"
		exit 1
	fi

	cd ../..
fi
