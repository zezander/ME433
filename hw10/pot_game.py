import pgzrun
import serial

# CHANGE THIS TO YOUR PORT
ser = serial.Serial('/dev/tty.usbmodem101', 115200, timeout=1)

pot = 0

WIDTH = 800
HEIGHT = 600


def update():
    global pot
    print("pot:", pot)
    if ser.in_waiting:
        try:
            line = ser.readline().decode().strip()
            # print("LINE:", line)
            if line:
                pot = int(line)
                # print("POT SET:", pot)
        except:
            pass


def draw():
    screen.clear()

    screen.draw.text(str(pot), (20, 20), fontsize=60)

    radius = int(pot / 4095 * 300)

    screen.draw.text(str(radius), (20, 100), fontsize=60)

    screen.draw.filled_circle((400, 300), radius, "white")


pgzrun.go()