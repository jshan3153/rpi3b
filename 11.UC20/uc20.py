'''
GPS Interfacing with Raspberry Pi using Pyhton
http://www.electronicwings.com
'''
import serial               #import serial pacakge
import RPi.GPIO as GPIO
from time import sleep
import sys

def main():
    print("main+\r\n")
    ser = serial.Serial(
        port='/dev/ttyS0',
        baudrate=115200,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        timeout=1
        )
        
    try:
        while True:
            try:
                received_data = ser.readline()                   #read NMEA string received
            except AttributeError:
                received_data = ''
            
            print(received_data)
        
        print('2')
    except KeyboardInterrupt:
        print('3')
        sys.exit(0)
        
if __name__ == '__main__':
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(11, GPIO.OUT) #pwr key
    GPIO.setup(12, GPIO.OUT) #rst key
    p13 = GPIO.setup(13, GPIO.IN) #status pin
    #GPIO.setup(40, GPIO.OUT)
    
    if (p13 != False):
        GPIO.output(12, GPIO.LOW)
        GPIO.output(11, GPIO.HIGH) #pwr key
        sleep(1)
        GPIO.output(11, GPIO.LOW)  #pwr key
        sleep(0.6)
        GPIO.output(11, GPIO.HIGH) #pwr key
        print("modem pwr on\r\n")
    else:
        print("modem already on\r\n")
    
    main()

                                        