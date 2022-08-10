from tensorflow.keras import datasets, layers, models, losses


def process_images(images):
        images = images.reshape((-1, 28, 28, 1))
        images = images / 255.0
        return images

(train_images, train_labels), (test_images, test_labels) = datasets.fashion_mnist.load_data()
train_images = process_images(train_images)
test_images = process_images(test_images)

model = models.Sequential()
model.add(layers.Conv2D(filters=6, kernel_size=(5, 5), activation='relu', input_shape=(28,28,1)))
model.add(layers.AveragePooling2D())
model.add(layers.Conv2D(filters=16, kernel_size=(5, 5), activation='relu'))
model.add(layers.AveragePooling2D())
model.add(layers.Flatten())
model.add(layers.Dense(units=120, activation='relu'))
model.add(layers.Dense(units=84, activation='relu'))
model.add(layers.Dense(units=10, activation = 'softmax'))

model.compile(optimizer='adam',
              loss=losses.SparseCategoricalCrossentropy(from_logits=True),
              metrics=['accuracy'])


# Save the untrained weights for future training with modified dataset
model.fit(train_images, train_labels, batch_size=100, epochs=10,
	validation_data=(test_images, test_labels))

model.save('./lenet-fmnist.tf')
