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
stick = pygame.image.load('C:/Users/KimDongWoo/Downloads/cpp-master/stick.png')
img = pygame.transform.scale(img, (555/6, 976/6))
stick1 = pygame.transform.scale(stick, (512/13 * 5, 189/4))
stick2 = pygame.transform.scale(stick, (512/13 * 8, 189/4))
stick3 = pygame.transform.scale(stick, (512/13 * 3, 189/4))


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
        
        if(temp[0] == 'END'):
            print("Program is terminating")
            quit()
            
        if(msg) :
            pos_x1, pos_y1, pos_x2, pos_y2, pos_x3, pos_y3, degree1, degree2, degree3, rotate = float(temp[0]), float(temp[1]), float(temp[2]), float(temp[3]), float(temp[4]), float(temp[5]), float(temp[6]), float(temp[7]), float(temp[8]), float(temp[9])
            rotated = pygame.transform.rotate(img, -rotate)
            rotated1 = pygame.transform.rotate(stick1, degree1) 
            rotated2 = pygame.transform.rotate(stick2, degree1 + degree2)
            rotated3 = pygame.transform.rotate(stick3, degree1 + degree2 + degree3)
            #clock.tick(1000)
            #player_rect =img.get_rect()
            #current_pos= (SCREEN_WIDTH/2 + 40*(pos_x3 - 3), SCREEN_HEIGHT/2 - 40*(pos_y3 - 5))
            #SCREEN_HEIGHT/2 - 40*((pos_y2 + pos_y3)/2 - 4) + (stick3.get_height() - rotated3.get_height())/2
            #current_pos = (SCREEN_WIDTH/2 + 40*((pos_x2 + pos_x3)/2 - 2.5) + (stick3.get_width() - rotated3.get_width())/2, SCREEN_HEIGHT/2 - 40*((pos_y2 + pos_y3)/2 - 4) + (stick3.get_height() - rotated3.get_height())/2)

            current_pos = (SCREEN_WIDTH/2 + 40*(pos_x3 - 2.5) + (stick3.get_width() - rotated3.get_width())/2, SCREEN_HEIGHT/2 - 40*(pos_y3 - 4) + (stick3.get_height() - rotated3.get_height())/2)
            screen.blit(rotated, current_pos)
            screen.blit(rotated1, (SCREEN_WIDTH/2 + 40*(pos_x1/2 - 3) + (stick1.get_width() - rotated1.get_width())/2, SCREEN_HEIGHT/2 - 40*(pos_y1/2 - 5) + (stick1.get_height() - rotated1.get_height())/2))
            screen.blit(rotated2, (SCREEN_WIDTH/2 + 40*((pos_x1 + pos_x2)/2 - 4.5) + (stick2.get_width() - rotated2.get_width())/2, SCREEN_HEIGHT/2 - 40*((pos_y1 + pos_y2)/2 - 5) + (stick2.get_height() - rotated2.get_height())/2))
            screen.blit(rotated3, (SCREEN_WIDTH/2 + 40*((pos_x2 + pos_x3)/2 - 2) + (stick3.get_width() - rotated3.get_width())/2, SCREEN_HEIGHT/2 - 40*((pos_y2 + pos_y3)/2 - 5) + (stick3.get_height() - rotated3.get_height())/2))
            pygame.display.update()
