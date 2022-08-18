from tensorflow.keras import datasets, layers, models, losses

def process_images(images):
        images = images.reshape((-1, 28, 28, 1))
        images = images / 255.0
        return images

(train_images, train_labels), (test_images, test_labels) = datasets.fashion_mnist.load_data()
train_images = process_images(train_images)
test_images = process_images(test_images)

model = models.Sequential()
model.add(layers.Conv2D(32, (5, 5), activation="relu", input_shape=(28, 28, 1)))
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

# Save the untrained weights for future training with modified dataset
model.fit(train_images, train_labels, batch_size=100, epochs=10,
	validation_data=(test_images, test_labels))

model.save('./cnn-fmnist.tf')

