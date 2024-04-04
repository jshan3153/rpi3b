import serial
import datetime
import time

baudrate = 115200
portaddr = "/dev/ttyS0"
filename = "temperatureData.txt" # output file
filemode = 'a'   #append


text_file = open(filename,filemode)
text_file.write("Timestamp,value\r")

ser = serial.Serial(portaddr,baudrate)

# read and discard first line of data
x=ser.readline() # first line of data is not complete

while 1:
    #ts = time.time()
    #st = datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S')
    x=ser.readline()
    #print(st + "," + str(x))  # print to screen
    #text_file.write(st + "," + str(x) +'\r\n')
    text_file.write(str(x)+'\n')
    text_file.flush()
    x=''

text_file.close()
ser.close