import RPi.GPIO as GPIO
from time import sleep

print("pwm example")

GPIO.setwarnings(False)  #disable warnings
GPIO.setmode(GPIO.BOARD) #set pin numbering system
GPIO.setup(32,GPIO.OUT)  #PWM pin connected to LED
GPIO.setup(37,GPIO.OUT)
GPIO.setup(38,GPIO.OUT)
GPIO.setup(40,GPIO.OUT)

GPIO.output(37, GPIO.HIGH) #LED POWER PIN 
GPIO.output(38, GPIO.LOW)
GPIO.output(40, GPIO.LOW)

pi_pwm = GPIO.PWM(32,1000) #create PWM instance with frequency
pi_pwm.start(0)            #start PWM of required Duty Cycle 
try:
    while True:
        for duty in range(0,101,1):
            pi_pwm.ChangeDutyCycle(duty) #provide duty cycle in the range 0-100
            sleep(0.01)
        sleep(0.5)
    
        for duty in range(100,0,-1):
            pi_pwm.ChangeDutyCycle(duty)
            sleep(0.01)
        
        sleep(0.5)
    
except KeyboardInterrupt:
    GPIO.cleanup()