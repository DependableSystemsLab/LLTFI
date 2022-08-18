#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon Aug  3 17:32:40 2020

@author: berk
"""
from tensorflow.keras.preprocessing.image import img_to_array
import scipy.misc
import os
from imutils import paths
import cv2
from tensorflow.keras.models import load_model
import time
from pdb import set_trace

input_shape = (66, 200, 3)
angle=[]
smooth_angle=0
test_ids=[]



f= open("data.txt")                                 #read steering angles from disk and preprocess
data = f.read()
data = data.split()
for i in data:                                      #if the node end with ".jpg" ignore it. It's for collecting angles
    if i[-1]=='g':
        pass
    else:
        angle.append(float(i) * scipy.pi / 180)     #convert rad.
        
model = load_model("model.h5")                      #import our model
model.save('dave2-keras.tf/')

#sahin_direksiyon = cv2.imread("sahin_direksiyon_simiti.png") #read steering image

#set_trace()

#image=cv2.resize(sahin_direksiyon[-150:], (200,66))
#image = image / 255

#image = img_to_array(image)/255
#result = -model.predict(image[None])*180.0/scipy.pi             #make a prediction
#print("Predicted Angle= " + str(-result))

'''
test_paths = list(paths.list_images(os.getcwd()+"/test")) 
#get test images ids (names)
for i in test_paths:
    name = i.split(os.path.sep)[-1]            
    name = name[:-4]
    test_ids.append(int(name))
test_ids.sort()
#test_ids=test_ids[15000:]  #where it start to show


for i in test_ids:
    image = cv2.imread(os.getcwd()+"/test/"+str(i)+".jpg")   #read images from disk
    image_show = image
    image=cv2.resize(image[-150:], (200,66))                
    image = img_to_array(image)/255                   #convert to numpy array
    
    cv2.imshow("Self Driving Car",cv2.resize(image_show,(800,398)))     #show image 
    
    result = -model.predict(image[None])*180.0/scipy.pi             #make a prediction
    print("Actual Angle= {} Predicted Angle= {}".format(str(angle[i]),str(-result)))
    
    #this section just for the smoother rotation of streeing wheel.
    smooth_angle += 0.2 * pow(abs((result - smooth_angle)), 2.0/3.0)*(result - smooth_angle)/abs(result-smooth_angle)
    M=cv2.getRotationMatrix2D((cols/2,rows/2),smooth_angle,1)
    dst= cv2.warpAffine(sahin_direksiyon, M, (cols,rows))
    #sahin_konsol[20:230,30:300]=dst #If you want to show frontside of the car just use this and replace dst with sahin_konsol in the next line. Note: Optional it's just for fun.

    cv2.imshow("Sahin Wheel",dst)
    #small delay for Optimus Prime level computers
    time.sleep(0.02)        
    if cv2.waitKey(1) & 0xFF == ord("q"):
        break
'''


cv2.destroyAllWindows()





