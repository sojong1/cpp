import pygame
import sys
import time

SCREEN_WIDTH = 640
SCREEN_HEIGHT = 480

white = (255, 255, 255)
black = (0, 0, 0)

pygame.init()
pygame.display.set_caption("Mouse track")
screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
img = pygame.image.load('/Users/hyodori/temp/python_interview/소종/mouse.png')
img = pygame.transform.scale(img, (555/6, 976/6))

pos_x = []
pos_y = []
rotate = []
index =0

clock = pygame.time.Clock()

f = open("/Users/hyodori/temp/python_interview/소종/output2.txt", 'r')
lines = f.readlines()


for line in lines:
    temp = list(map(float, line.split()))
    pos_x.append(temp[1])
    pos_y.append(temp[2])
    rotate.append(temp[3])
    if not line: break
f.close()
while True:
    clock.tick(1000)
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            sys.exit()
            
    screen.fill(white)
    if(index > 0 ) :
        rotated = pygame.transform.rotate(img, -rotate[index])
        print(rotate[index])
        player_rect =img.get_rect()
        current_pos= (SCREEN_WIDTH / 4 + pos_x[index]/100, SCREEN_HEIGHT / 4 + pos_y[index]/100)
        screen.blit(rotated, current_pos)
        pygame.display.update()
    if(index < len(pos_x) -2 ) :
        index +=1
        

        