'''
Server side python script
In order to make it run on your own PC,
you must set up you own AWS credentials via CLI !!
'''

import boto3
import cv2
import time
import serial
import os
from threading import Lock, Thread
from twilio.rest import Client
#import sys
#import select
#import tty
#import termios

face_detector = cv2.CascadeClassifier('haarcascade_frontalface_default.xml')
ser = serial.Serial('/dev/ttyUSB0',115200, timeout=1)
mutex = Lock()
account_sid = 'AC3ef7c4a2f16a00932f28931c0aa79ab4'
auth_token = '8a83686be89cffb70b1eb3b6500a99f6'
sendFrom = '+12537859811'
sendTo = '+12066943165'
bucket = "css427facialtest"
securityFlag = False
securityMsg = "SOME ONE BREAK IN YOUR ROOM!!!"
fireFlag = False
fireMsg = "THE ROOM IS ON FIRE!!!"

'''
takeOneFace() used in a exhibition hall which many people may show up together
'''
def takeOneFace():
  # capture a pciture
  vid_cam = cv2.VideoCapture(0)
  ret, frame = vid_cam.read()
  
  if not ret:
    return

  # write to disk
  currentTime = str(time.asctime())
  pName = "AAA/" + currentTime + ".jpg"
  cv2.imwrite(pName, frame)

  vid_cam.release()

  return pName

'''
takeTwoFaces() used when people come one by one
it will capture the face, convert to gray scale and trim the extra picture
so that reduce the pciture size
'''
'''
def takeTwoFaces():
  # capture a picture
  vid_cam = cv2.VideoCapture(0)
  ret, image_frame = vid_cam.read()

  # conver to gray scale to compress size
  gray = cv2.cvtColor(image_frame, cv2.COLOR_BGR2GRAY)
  
  # Detect frames of different sizes, list of faces rectangles
  faces = face_detector.detectMultiScale(gray, 1.3, 5)

  # Crop the image frame into rectangle and write to disk
  for (x,y,w,h) in faces:
    cv2.rectangle(image_frame, (x,y), (x+w,y+h), (255,0,0), 2)
    cv2.imwrite(str(2) + ".jpg", gray[y:y+h,x:x+w])
    
  vid_cam.release()
'''
  
def uploadToS3(pName):
  # check if the webcam successfully capture the picture
  if (not os.path.isfile(pName)):
      prnit("no file exist, will not compare")
      return 0
      
  # connect to the server
  s3 = boto3.resource('s3')

  # upload picture
  BUCKET = "css427facialtest"
  s3.Bucket(BUCKET).upload_file(pName, pName[4:])
  return 1
  

def compare_faces(bucket, targetPic, threshold=80, region="us-west-2"):
  # record totaly number of ppl recognize in the pricture 
  knownPplCount = 0;
      
  # data base pciture
  source1 = "target0.jpg"
  source2 = "target1.jpg"
  
  # target picture to detect
  target1 = targetPic
  print("Comparing database with: " + target1)
  target2 = "2.jpg"
  
  # connect to rekognition server
  rekognition = boto3.client("rekognition", region)
  
  # compare to database 1
  response1 = rekognition.compare_faces(
    SourceImage={
      "S3Object": {
        "Bucket": bucket,
        "Name": source1,
      }
    },
      
    TargetImage={
      "S3Object": {
        "Bucket": bucket,
        "Name": target1,
      }
    },
    SimilarityThreshold=threshold,
  )

  ''' 
  # compare to database 2
  response1 = rekognition.compare_faces(
    SourceImage={
      "S3Object": {
        "Bucket": bucket,
        "Name": source2,
      }
    },
    
    TargetImage={
      "S3Object": {
        "Bucket": bucket,
        "Name": target2,
      }
    },
    SimilarityThreshold=threshold,
  )
  '''
  
  # get JSON result 
  result1 = response1['FaceMatches']

  # check each faces in result
  for match in result1:
      if (float(match['Similarity']) > 80):
        
        print("\n*******Alex enter the room*******\n\n")
        knownPplCount+=1

  '''
  # get result 2
  result1 = response1['FaceMatches']
  for match in result1:
      if (float(match['Similarity']) > 80):
        print("Rathtana enter the room")
        knownPplCount+=1
  '''
  
  if knownPplCount == 0:
    print("\n*******Unknown person enter the room*******\n\n")
    

'''
call the facial recognition process sequence 
'''
def makeFunctionCall():
  takeOneFace()
  pic = takeOneFace()
  #takeTwoFaces()
  #takeTwoFaces()
  result = uploadToS3(pic)
  if result  == 1:
    try:
      compare_faces(bucket, pic[4:])
    except:
      # when there is some one come in, but camera can not capture the face
      print("\n*******A warm ghost enter the room*******\n\n")


'''
Send command from server side to client side (one thread)
'''
def writeToESP():
  global securityFlag
  
  while True:
    
    #mutex.acquire()
    command = raw_input()

    # for change sample rate
    
    if command == "6":
      # input check
      while True:
        try:
          sampleRate = float(raw_input("Input the new sample rate from 0.1 to 10Hz: \n"))
        except:
          print("Invalid input! \n")
          continue
        if sampleRate >= 0.1 and sampleRate <= 10:
          break
        else:
          print("Invalid input! \n")

      # send new smaple rate to server
      ser.write(str(sampleRate) + "R")

      print("New sample rate is: " + str(sampleRate) + "Hz\n\n")

    # on-demand query
    elif command == "1":
      #print("Asking for a query...")
      ser.write("1")
      
      '''
      flag = True
      while flag:
        queryCommand = raw_input("now is in on-demand mode.\nPress any key for query, press p back to periodical mode: \n")  
        if queryCommand == "0":
          ser.write("change to periodical")
          print("Now is in periodical mode")
          flag = False
        else:
          ser.write("ask a query")
          continue
      '''

    # enable periodical mode 
    elif command == "2":
      ser.write("2")
      print("Periodical mode is enabled.\n\n")
      
    # disable periodical mode 
    elif command == "3":
      ser.write("3")
      print("Periodical mode is disabled.\n\n")

    # enable security mode     
    elif command == "4":
      securityFlag = True
      ser.write("4")
      print("Security mode is enabled.\n\n")

    # disable security mode
    elif command == "5":
      securityFlag = False
      ser.write("5")
      print("Security mode is disabled.\n\n")
    #mutex.release()


'''
Send break in msg to phone
'''
def sendWarningMSG():
  client = Client(account_sid, auth_token)
  message = client.messages \
            .create(
                body= securityMsg,
                from_= sendFrom,
                to= sendTo
             )
  print("MSG sent \n")


'''
send fire alarm msg to phone
'''
def sendWarningMSG1():
  client = Client(account_sid, auth_token)
  message = client.messages \
            .create(
                body= fireMsg,
                from_= sendFrom,
                to= sendTo
             )
  print("MSG sent \n")
  
  
'''
Server listening client all the time
'''
def readFromESP():
    #mutex.acquire()
    #print("START")
    receive = ser.readline()
    
    # when get ack knowl
    if "Client" in receive:
      print("Client receive the message !!!")    
      return 
    
    # when receive msg from client
    if (";" in receive):
      
      # parse ';' as end line character
      dataMsg = receive.split(";")

      msgLen = len(dataMsg)

      
      # means ppl come in or leave, only 3 info
      if msgLen > 2 and msgLen < 6:

        newData = "Sensor type: distance sensor\n"
        newData += "Message ID: " + dataMsg[1] + "\n"
        newData += "New head count: " + dataMsg[2] + "\n"
        
        # ppl enter
        if (dataMsg[0] == "1"):
          print(newData)
          print("Some one is entering the room... doing analyze...\n")

          # if ppl come in and security mode on
          if (securityFlag):
            sendWarningMSG()

          # facial recognition
          makeFunctionCall()

        # ppl leave   
        elif (dataMsg[0] == "0"):
          print("Some on is leaving the room...updating head count...\n")
          print(newData)
        

        print("====================END OF THE MESSAGE====================\n")   

      # means query or periodical mode data income 
      elif msgLen > 6:
        
        # for msg number
        print("Message ID: " + dataMsg[1])

        # for mode
        if ("0" in dataMsg[0]):
          print("Mode: On-demand mode")
        elif ("1" in dataMsg[0]):
          print("Mode: Periodical mode")

        # for sensors
        print("PIR sensor 1: " + dataMsg[2])
        print("PIR sensor 2: " + dataMsg[3])
        print("Distance Sensor 1: " + dataMsg[4])
        print("Distance Sensor 2: " + dataMsg[5])
        print("Photoresistor Sensor: " + dataMsg[6])
        #print("Temperature Sensor: " + dataMsg[8])
        
        # head ocunt
        print("Number of people in the room: " + dataMsg[7])
        print("====================END OF THE MESSAGE====================\n")   
          
      # when msgLen = 1 and contains "F" means temperature sensor triggered
      elif msgLen > 0 and msgLen < 2 and "F" in dataMsg:
        sendWarningMSGF()
        
    #print("RELEASE")
    #mutex.release()


def isData():
    return select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], [])


'''
Block main: when user input, the server will stop listening (if want to use this main, the import part must be uncomment!)
'''
'''
if __name__ == "__main__":
  
  old_settings = termios.tcgetattr(sys.stdin)
  try:
    tty.setcbreak(sys.stdin.fileno())
    
    while True:
      readFromESP()
      if isData():
        c = sys.stdin.read(1)
        if c == '\x1b':         # ESC for quit
          print("Exist")
          break;
        if command == "s":
          sampleRate = input("Input the new sample rate from 1s to 3600s: \n")
          print("New sample rate is: " + str(sampleRate) + "s")
          ser.write("s" + str(sampleRate))
      
        elif command == "o":
          ser,write("change to on-demand")
          flag = True
          while flag:
            queryCommand = input("now is in on-demand mode. Press 1 for query, press p back to periodical mode")
            if queryCommand == "1":
              ser.write("ask a query")
            elif queryCommand == "o":
              ser,write("change to periodical")
              print("Now is in periodical mode")
              flag = False
            else:
              conitnue
          
        elif command == "p":
          ser,write("change to periodical")
          print("now is in periodical mode.")
  finally:
    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
'''  

'''
Multithread main: when people input command, server still can listening
'''
if __name__ == "__main__":
  # introduce message
  print("-------------------------------------------------")
  print("System start")
  print("press '1' for on-demand query")
  print("press '2' for enable periodical mode")
  print("press '3' for disable periodcial mode")
  print("press '4' for enable security mode")
  print("press '5' for disable security mode")
  print("press '6' for change sample rate")
  print("-------------------------------------------------")
  
  # initial and enable the user input thread 
  t1 = Thread(target=writeToESP, args=())
  t1.start()
  
  # start listenning from serial monitor
  while True:
    readFromESP()
  
  

