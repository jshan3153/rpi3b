'''
GPS Interfacing with Raspberry Pi using Pyhton
http://www.electronicwings.com
'''
import serial               #import serial pacakge

def main():
    ser = serial.Serial(
        port='/dev/ttyS0',
        baudrate=115200,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        timeout=1
        )
        
    try:
        print('1')
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
    main()
