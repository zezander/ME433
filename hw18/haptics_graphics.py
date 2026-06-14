import pygame
import serial
import math

PORT = "/dev/cu.usbmodem1101"
BAUD = 115200

ser = serial.Serial(PORT, BAUD, timeout=0.01)

pygame.init()
screen = pygame.display.set_mode((900, 600))
pygame.display.set_caption("Haptic Lever Graphics")
clock = pygame.time.Clock()

center_x = 450
center_y = 320
arm_length = 220

angle_deg = 173.0
load_raw = 0
x_pos = 0.0
u = 0.0

font = pygame.font.SysFont(None, 32)

running = True
while running:
    screen.fill((245, 245, 245))

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    # Read Pico serial
    try:
        line = ser.readline().decode().strip()

        if line and not line.startswith("angle"):
            parts = line.split(",")

            if len(parts) >= 4:
                angle_deg = float(parts[0])
                load_raw = float(parts[1])
                x_pos = float(parts[2])
                u = float(parts[3])

    except:
        pass

    # Convert your centered angle to screen angle
    # x_pos is already -1 to +1 over your 45 deg each side
    display_angle_deg = x_pos * 45.0
    display_angle_rad = math.radians(display_angle_deg)

    end_x = center_x + arm_length * math.sin(display_angle_rad)
    end_y = center_y - arm_length * math.cos(display_angle_rad)

    # Draw pivot
    pygame.draw.circle(screen, (0, 0, 0), (center_x, center_y), 18)

    # Draw lever
    pygame.draw.line(
        screen,
        (30, 80, 200),
        (center_x, center_y),
        (end_x, end_y),
        14
    )

    # Draw lever end
    pygame.draw.circle(screen, (200, 50, 50), (int(end_x), int(end_y)), 22)

    # Draw center reference line
    pygame.draw.line(
        screen,
        (150, 150, 150),
        (center_x, center_y),
        (center_x, center_y - arm_length),
        3
    )

    # Draw force/motor command bar
    bar_x = 80
    bar_y = 500
    bar_width = 300
    bar_height = 30

    pygame.draw.rect(screen, (180, 180, 180), (bar_x, bar_y, bar_width, bar_height), 2)

    fill = int((u + 1) / 2 * bar_width)
    pygame.draw.rect(screen, (80, 180, 80), (bar_x, bar_y, fill, bar_height))

    # Text
    text1 = font.render(f"Angle raw deg: {angle_deg:.1f}", True, (0, 0, 0))
    text2 = font.render(f"Position x: {x_pos:.2f}", True, (0, 0, 0))
    text3 = font.render(f"Load raw: {load_raw:.0f}", True, (0, 0, 0))
    text4 = font.render(f"Motor command u: {u:.2f}", True, (0, 0, 0))

    screen.blit(text1, (30, 30))
    screen.blit(text2, (30, 65))
    screen.blit(text3, (30, 100))
    screen.blit(text4, (30, 135))

    pygame.display.flip()
    clock.tick(60)

ser.close()
pygame.quit()