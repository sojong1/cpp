import pygame
import sys
import time

import socket 
SCREEN_WIDTH = 640 * 2
SCREEN_HEIGHT = 480 * 2

white = (255, 255, 255)
black = (0, 0, 0)

pygame.init()
pygame.display.set_caption("Mouse track")
screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
img = pygame.image.load('C:/Users/KimDongWoo/Downloads/cpp-master/mouse.png')
img = pygame.transform.scale(img, (555/6, 976/6))


clock = pygame.time.Clock()
screen.fill(white)

if __name__ == '__main__':
 
    serverAddressPort = ("127.0.0.1", 61557) 
    bufferSize = 1024 
    # 클라이언트 쪽에서 UDP 소켓 생성

    UDPClientSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
    UDPClientSocket.bind(serverAddressPort) 



    while True:
        clock.tick(1000)
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                sys.exit()
        msgFromServer = UDPClientSocket.recvfrom(bufferSize)
        msg = msgFromServer[0].decode('unicode_escape').encode('utf-8')
        msg = msg.decode('utf-8')
        temp = msg.split(' ')
        screen.fill(white)
        if(msg) :
            pos_x, pos_y , rotate = float(temp[0]), float(temp[1]), float(temp[2])
            clock.tick(1000)
            rotated = pygame.transform.rotate(img, -rotate)
            player_rect =img.get_rect()
            current_pos= (SCREEN_WIDTH / 4 + pos_x/500, SCREEN_HEIGHT / 4 + pos_y/500)
            screen.blit(rotated, current_pos)
            pygame.display.update()
