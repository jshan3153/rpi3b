'''
GPS Interfacing with Raspberry Pi using Pyhton
http://www.electronicwings.com
'''
import serial               #import serial pacakge
from time import sleep
import webbrowser           #import package for opening link in browser
import sys                  #import system package
import RPi.GPIO as GPIO
import threading
import http.client
import socket

GPGGA_buffer = 0
NMEA_buff = 0
lat_in_degrees = 0
long_in_degrees = 0
error = 0
led = 0
txtime = 0
txlat = 0
txlon = 0
txready = 0
boot_flag = 0    #1:boot 0: not boot
network_flag = 0 #1:connect 0:not conect
rled = 0
gled = 0
bled = 0

def GPS_Info():
    global NMEA_buff
    global lat_in_degrees
    global long_in_degrees
    global txready
    global txtime, txlat, txlon
    nmea_time = [] #list
    nmea_latitude = []
    nmea_longitude = []
    try:
        nmea_time = NMEA_buff[0]                    #extract time from GPGGA string
        nmea_latitude = NMEA_buff[1]                #extract latitude from GPGGA string
        nmea_longitude = NMEA_buff[3]               #extract longitude from GPGGA string
        satelite = NMEA_buff[6]                     #extract gps satelite from GPGGA string
    except:
        print(NMEA_buff)
        return
    
    #print("NMEA Time: ", nmea_time,'\n')
    #print ("NMEA Latitude:", nmea_latitude,"NMEA Longitude:", nmea_longitude,'\n')
    #print("Satelite : ", satelite, '\n')
    
    lat = float(nmea_latitude)                  #convert string into float for calculation
    longi = float(nmea_longitude)               #convertr string into float for calculation
    
    lat_in_degrees = convert_to_degrees(lat)    #get latitude in degree decimal format
    long_in_degrees = convert_to_degrees(longi) #get longitude in degree decimal format
    
    print(nmea_time, "LAT:", lat_in_degrees, " LON:", long_in_degrees, "[", satelite, "]", '\n')
    
    txtime = nmea_time.replace('.','')
    txtime = txtime.ljust(10,'0')
    #print(nmea_time, txtime)
    
    txlat = nmea_latitude.replace('.','')
    #print(nmea_latitude, txlat)

    txlon = nmea_longitude.replace('.','')
    #print(nmea_longitude, txlon)
    #payload = "8931440400065254618626990010010002000005000120000#"+txtime+txlat+txlon+"20#"
    txready = 1
    #print(payload)

#convert raw NMEA string into degree decimal format   
def convert_to_degrees(raw_value):
    decimal_value = raw_value/100.00
    degrees = int(decimal_value)
    mm_mmmm = (decimal_value - int(decimal_value))/0.6
    position = degrees + mm_mmmm
    position = "%.4f" %(position)
    return position

def send_to_server():
    print('send2server')
    global txready
    global txtime
    global txlat
    global txlon
    global boot_flag
    usim = "89314404000652546186" #"89314404000476684577"
    
    timer=threading.Timer(5, send_to_server)
    timer.start()

    if network_flag:
        conn = http.client.HTTPSConnection("protocol.cleancitynetworks.com")

        headers = {
          'Content-Type': 'text/plain'
        }
        
        if txready == 1:
            payload = usim + "26990010010304"+str(boot_flag)+"00000800251000#"+txtime+'+'+txlat+'+'+txlon+"20#"
            #print(payload)
            txready = 0
        else:
            payload = usim + "26990010010304"+str(boot_flag)+"00000800251000##"    
            print('tx not ready')

        conn.request("POST", "/ttk", payload, headers)
        res = conn.getresponse()
        data = res.read()
        response = data.decode("utf-8")
        print(response)
        if boot_flag == 0:
            if response[0] == '2' and response[1] == '0' and response[2] == '0':
                boot_flag = 1
                print("boot complete")
    else:
        print("not send")

    return

def check_to_network():
    global network_flag
    print("check network")
    '''
    if not connect internet
    return 127.0.0.1
    '''
    #ipAddress = socket.gethostbyname(socket.gethostname())
    hostname = 'www.naver.com'
    
    try:
        ipAddress = socket.gethostbyname(hostname)
        print("IP ADDRESS {} is : {}".format(hostname, ipAddress))
        print("IP ADDRESS " + hostname + " is : " + ipAddress)
        network_flag = 1
        print(ipAddress)
    except socket.gaierror:
        print("ignoreing failed address lookup")
        ipAddress = '127.0.0.1'
        network_flag = 0
        
    timer=threading.Timer(10, check_to_network)
    timer.start()

    return network_flag

def led(color, ctrl):
    global rled, gled, bled
    status = 0
    
    if ctrl == "toggle":
        if color == 'r':
            if rled == 1:
                rled = 0
                GPIO.output(36, GPIO.HIGH)  #R
                GPIO.output(38, GPIO.LOW)  #B
                GPIO.output(40, GPIO.LOW) #G
                status = 1
            else:
                rled = 1
        elif color == 'g':
            if gled == 1:
                gled = 0
                GPIO.output(36, GPIO.LOW)  #R
                GPIO.output(38, GPIO.LOW)  #B
                GPIO.output(40, GPIO.HIGH) #G
                status = 1
            else:
                gled = 1
    elif ctrl == "on":
        if color == 'r':
            GPIO.output(36, GPIO.HIGH)  #R
            GPIO.output(38, GPIO.LOW)  #B
            GPIO.output(40, GPIO.LOW) #G
            status = 1
        elif color == 'g':
            GPIO.output(36, GPIO.LOw)  #R
            GPIO.output(38, GPIO.LOW)  #B
            GPIO.output(40, GPIO.HIGH) #G
            status = 1
        else :
            GPIO.output(36, GPIO.LOw)  #R
            GPIO.output(38, GPIO.HIGH)  #B
            GPIO.output(40, GPIO.LOW) #G
            status = 1
    else:
        GPIO.output(36, GPIO.LOW)  #R
        GPIO.output(38, GPIO.LOW)  #B
        GPIO.output(40, GPIO.LOW) #G
        
    if status == 0:
        GPIO.output(36, GPIO.LOW)  #R
        GPIO.output(38, GPIO.LOW)  #B
        GPIO.output(40, GPIO.LOW) #G

def main():
    global ser
    global lat_in_degrees
    global long_in_degrees
    global NMEA_buff

    error = 0
    
    ser = serial.Serial(
        port='/dev/ttyS0',
        baudrate=9600,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        timeout=1
        )
        
    # GPIO setup
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(36, GPIO.OUT)
    GPIO.setup(37, GPIO.OUT)
    GPIO.setup(38, GPIO.OUT) #blue
    GPIO.setup(40, GPIO.OUT)

    GPIO.output(37, GPIO.HIGH)

    GPIO.output(36, GPIO.LOW)  #R
    GPIO.output(38, GPIO.LOW)  #B
    GPIO.output(40, GPIO.LOW) #G

    #check network
    status = check_to_network()
    
    if status == 1:
        send_to_server()

    try:
        while True:
            received_data = (str)(ser.readline())                   #read NMEA string received
            #print(received_data)
            #received_data = 0
        
            GPGGA_data_available = received_data.find("$GPGGA,")   #check for NMEA GPGGA string                 
        
            if (GPGGA_data_available>0):
                comma_count = received_data.count(',')
                
                if(comma_count<13):
                    error += 1
                    print('error %d' % error)
                else:
                    GPGGA_buffer = received_data.split("$GPGGA,",1)[1]  #store data coming after "$GPGGA," string
                    NMEA_buff = (GPGGA_buffer.split(','))               #store comma separated data in buffer
                    #check null buffer
                    if not NMEA_buff[5]:
                        print('no fix flag\n')
                        error += 1
                    elif NMEA_buff[5] != '0':
                        GPS_Info()                                          #get time, latitude, longitude
                        #print("lat in degrees:", lat_in_degrees," long in degree: ", long_in_degrees, '\n')
                        #map_link = 'http://maps.google.com/?q=' + lat_in_degrees + ',' + long_in_degrees    #create link to plot location on Google map
                        #print("<<<<<<<<press ctrl+c to plot location on google maps>>>>>>\n")               #press ctrl+c to plot on map and exit 
                        #print("------------------------------------------------------------\n")
                        led('g', "toggle")
                    else:
                        print('not fixed :', NMEA_buff)
                        led('r', "toggle")
                            
                GPGGA_data_available = 0
            
                sleep(1)
               
            received_data = 0

    except KeyboardInterrupt:
        #webbrowser.open(map_link)        #open current position information in google map
        GPIO.cleanup()
        sys.exit(0)
        
if __name__ == '__main__':
    main()
