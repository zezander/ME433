import time
import board
import pwmio
from analogio import AnalogIn

# start by setting up pwm pin
pwm = pwmio.PWMOut(board.GP16, frequency=50)
# set pin 26 as analog pin
adc = AnalogIn(board.GP26)
# my setServo func
def setServo(angle):
    # normalize angle (0–180)
    pwm.duty_cycle = int((0.05 +(angle/180.0)*0.05)*60000) # create duty percent

# the 'main' function
while True:
    for angle in range(10, 170): # sweep angles from 10 to 170 incrementing
        setServo(angle) # for each angle i change duty cycle to my angle
        print("Angle:", angle)
        time.sleep(0.015)
    # decreasing angles
    for angle in range(170, 10, -1): # sweep angles from 170 to 10 decreasing
        setServo(angle)
        print("Angle:", angle)
        time.sleep(0.015)