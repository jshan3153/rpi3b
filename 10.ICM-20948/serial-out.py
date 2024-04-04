import serial

ser = serial.Serial()
ser.baudrate = 115200
ser.timeout = None
ser.port = '/dev/ttyS0'
ser.open()

while True:
    ser.write(b"hello\n")
    
