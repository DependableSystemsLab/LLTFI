#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Jul 29 08:17:07 2020

@author: berk
"""


from tensorflow.keras.models import Sequential 
from tensorflow.keras.layers import Conv2D,Dense,Flatten,Dropout
import tensorflow as tf
import scipy.misc
import os
from imutils import paths
from datagen import DataGenerator

INIT_LR = 1e-4
input_shape = (66, 200, 3)



#------------------------------------------------------
#This section for reduce some errors related to GPU allocation on my system.
#it may not neccesary for yours. If it is, removing this part may increase the performance.
#from tensorflow import Session,ConfigProto
#from keras.backend.tensorflow_backend import set_session
#config = ConfigProto()
#config.gpu_options.per_process_gpu_memory_fraction = 0.3
#set_session(Session(config=config))
#--------------------------------------------------------



def atan_layer(x):
    return tf.multiply(tf.atan(x), 2)

def atan_layer_shape(input_shape):
    return input_shape


#our model is Nvidia Dave-2 where you can find here: https://arxiv.org/pdf/1604.07316.pdf
def defineModel():
    model = Sequential()

    # 5x5 Convolutional layers with stride of 2x2
    model.add(Conv2D(24, (5, 5), strides=(2, 2),activation='elu',input_shape=input_shape))
    model.add(Conv2D(36, (5, 5), strides=(2, 2),activation='elu'))
    model.add(Conv2D(48, (5, 5), strides=(2, 2),activation='elu'))
    
    # 3x3 Convolutional layers with stride of 1x1
    model.add(Conv2D(64, (3, 3),activation='elu'))
    model.add(Conv2D(64, (3, 3),activation='elu'))
    
    # Flatten before passing to the fully connected layers
    model.add(Flatten())
    # Three fully connected layers
    model.add(Dense(100,activation='elu'))
    model.add(Dropout(.25))
    model.add(Dense(50,activation='elu'))
    model.add(Dropout(.25))
    model.add(Dense(10,activation='elu'))
    model.add(Dropout(.25))
    
    # Output layer with linear activation 
    model.add(Dense(1,activation="linear"))
    
    return model


   

angle=[]

f= open("data.txt")                                 #read steering angles from disk and preprocess
data = f.read()
data = data.split()
for i in data:                                      #if the node end with ".jpg" ignore it. what we need is only angles
    if i[-1]=='g':
        pass
    else:
        angle.append(float(i))


paths= list(paths.list_images(os.getcwd()+"/data")) #collect the image ids (name) to send datagen.py
ids=[]
for i in paths:
    name = i.split(os.path.sep)[-1]             
    name = name[:-4]
    ids.append(int(name))    
ids.sort()                  



#train and test set ratio. We create two dictionary for data batch generator. 
#partition consist of two list that holds the train and validation image ids. labels hold the angles..
partition={'train':ids[:int(len(ids)*.8)],'validation':ids[-int(len(ids)*.2):]}
labels={}
for i in partition["train"]:
    labels[i]=float(angle[i])* scipy.pi / 180
for i in partition["validation"]:
    labels[i]=float(angle[i])* scipy.pi / 180




    
# Parameters for datagen.py
params = {'dim': (66,200,3),
          'batch_size': 2,
          'shuffle': True}

# Generators
training_generator = DataGenerator(partition["train"], labels, **params)
validation_generator = DataGenerator(partition["validation"], labels, **params)


#defining our model and compile with adam optimizer and mean squere error.
model=defineModel()
model.compile(optimizer='adam', loss="mse")
    
#train it for 10 epochs
model.fit_generator(generator=training_generator,
                    epochs=10,   
                    validation_data=validation_generator)    

#save trained model.
model.save("model.h5")


