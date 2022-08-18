#Disable excessive logging
import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'

#Setup GPU environment
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import datasets, layers, models, losses, regularizers
import numpy as np

physical_devices = tf.config.list_physical_devices('GPU')

for device in physical_devices:
    tf.config.experimental.set_memory_growth(device, True)


def process_images(images):
    images = images / 255.0
    return images


def get_model():
    model = models.Sequential(
                [
                    tf.keras.layers.Flatten(input_shape=(28, 28)),
                    tf.keras.layers.Dense(128, activation="relu"),
                    tf.keras.layers.Dropout(0.2),
                    tf.keras.layers.Dense(10, activation="softmax"),
                ]
            )

    model.compile(
        optimizer="adam",
        loss=tf.keras.losses.SparseCategoricalCrossentropy(),
        metrics=["accuracy"],
    )
    return model


def main():
    #Load training data
    (train_images, train_labels), (test_images, test_labels) = datasets.mnist.load_data()
    train_images = process_images(train_images)
    test_images = process_images(test_images)

    #Setup model
    model = get_model()
    modelname = "mnist-nn"

    #Train and test model
    model.fit(train_images, train_labels, batch_size=100, epochs=5, verbose=2)

    test_loss, test_acc = model.evaluate(test_images, test_labels, verbose=0)
    print("Accuracy before faults for ", modelname, "is ", test_acc)

    #Save model
    filepath = modelname + ".tf"
    model.save(filepath)


if __name__ == "__main__":
    main()

