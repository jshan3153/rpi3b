# SPDX-FileCopyrightText: 2021 ladyada for Adafruit Industries
# SPDX-License-Identifier: MIT

import time
import board
import adafruit_icm20x
import serial

global ser

ser = serial.Serial()
ser.baudrate = 115200
ser.timeout = None
ser.port = '/dev/ttyS0'
ser.open()


i2c = board.I2C()  # uses board.SCL and board.SDA
# i2c = board.STEMMA_I2C()  # For using the built-in STEMMA QT connector on a microcontroller
icm = adafruit_icm20x.ICM20948(i2c)

while True:
    print("Acceleration: X:%.2f, Y: %.2f, Z: %.2f m/s^2" % (icm.acceleration))
    print("Gyro X:%.2f, Y: %.2f, Z: %.2f rads/s" % (icm.gyro))
    #serial_data = "%.2f, %.2f, %.2f \n" % (icm.gyro)
    #ser.write(serial_data.encode("utf-8"))
    ser.write(b"%.2f, %.2f, %.2f \n" % (icm.gyro))
    print("Magnetometer X:%.2f, Y: %.2f, Z: %.2f uT" % (icm.magnetic))
    print("")
    time.sleep(0.5)

