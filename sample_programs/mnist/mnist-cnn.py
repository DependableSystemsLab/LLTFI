from tensorflow.keras import datasets, layers, models, losses, regularizers

def process_images(images):
    images = images.reshape((-1, 28, 28, 1))
    images = images / 255.0
    return images


def get_model():
    model = models.Sequential()
    model.add(
        layers.Conv2D(
            32, (5, 5), activation="relu", input_shape=(
                28, 28, 1)))
    model.add(layers.MaxPooling2D((2, 2)))
    model.add(layers.Conv2D(64, (5, 5), activation="relu"))
    model.add(layers.MaxPooling2D((2, 2)))

    model.add(layers.Flatten())
    model.add(layers.Dense(10, activation="softmax"))

    model.compile(
        optimizer="adam",
        loss=losses.SparseCategoricalCrossentropy(),
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
    modelname = "mnist-cnn"

    #Train and test model
    model.fit(train_images, train_labels, batch_size=100, epochs=5, verbose=2)

    #Save model
    filepath = modelname + ".tf"
    model.save(filepath)


if __name__ == "__main__":
    main()

