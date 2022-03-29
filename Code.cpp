#include <stdio.h>
#include <iostream>
#include <winsock2.h> // 윈속 헤더 포함 
#include <windows.h> 
#include <tchar.h>
#include <string>
#include <iomanip>
#include <cmath>
#include <vector>
#include <fstream>
#include "SerialClass.h"	// Library described above

#pragma comment (lib,"ws2_32.lib") // 윈속 라이브러리 링크
#define BUFFER_SIZE 64 // 버퍼 사이즈 default=1024

const double D = 2.834646; //inch
const int CPI = 1200;
const double PI = 3.14159265358979323846;
const double R = 180.0 / (PI * D * CPI);

double move[2][2]; // [ [xi, yi] , [xi+1, yi+1] ….   ]
double dmove[2][2]; // [ [dxi, dyi] , [dxi+1, dyi+1] ….   ]
double theta[2];

double degree_to_rad(double degree);
void theta_converter(int dx1, int dy1, int dx2, int dy2, int buttonState1, int buttonState2, int index);

// application reads from the specified serial port and reports the collected data
int main()
{
	printf("Welcome to the serial test app!\n\n");
	Serial* SP = new Serial("\\\\.\\COM5");    // adjust as needed

	if (SP->IsConnected())
		std::cout << "We're connected\n" << std::endl;

	char incomingData[2] = "";			// don't forget to pre-allocate memory
	int readResult = 0;

	//make socket
	WSADATA wsaData; // 윈속 데이터 구조체
	SOCKET ClientSocket; // 소켓 선언
	SOCKADDR_IN ToServer; // 서버로 보내는 주소정보 구조체

	int Send_Size;
	ULONG   ServerPort = 61557; // 서버 포트번호

	char Buffer[BUFFER_SIZE] = {};
	//sprintf_s(Buffer, "STOP,%.2lf\n", dd);
	if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR)
	{
		std::cout << "WinSock 초기화부분에서 문제 발생 " << std::endl;
		WSACleanup();
		exit(0);
	}

	memset(&ToServer, 0, sizeof(ToServer));

	ToServer.sin_family = AF_INET;
	ToServer.sin_addr.s_addr = inet_addr("127.0.0.1");
	ToServer.sin_port = htons(ServerPort); // 포트번호

	ClientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);//

	if (ClientSocket == INVALID_SOCKET)
	{
		std::cout << "소켓을 생성할수 없습니다." << std::endl;
		closesocket(ClientSocket);
		WSACleanup();
		exit(0);
	}

	int dx1(0), dy1(0), dx2(0), dy2(0);
	int button[2] = { 0, 0 };
	int cnt = 0;
	std::string inputState = "";
	std::string num = "0";

	while (SP->IsConnected())
	{
		readResult = SP->ReadData(incomingData, 1);

		if (readResult != 0)
		{
			char ch = incomingData[0];
			//std::cout << ch;
			
			switch (ch)
			{
			case 'x':
			case 'y':
			case 'c':
				inputState = ch;
				break;
			case 'a':
			case 'b':
				inputState += ch;
				break;
			default:
				if (inputState == "n") num += ch;
				else
				{
					if (inputState == "xa")
					{
						button[1] = stoi(num);
						cnt = (cnt + 1) % 2;
						theta_converter(dx1, dy1, -dx2, dy2, button[0], button[1], cnt);
						//std::cout << " " << dx1 << " " << dx2 << " " << dy1 << " " << dy2 << " " << button[0] << " " << button[1] << std::endl;

						
						std::cout << "x: " << std::setw(10) << move[cnt][0]
							<< ", y: " << std::setw(10) << move[cnt][1]
							<< ", theta: " << std::setw(10) << theta[cnt] << std::endl;
						
						//send packet
						sprintf_s(Buffer, "%lf %lf %lf \n", move[cnt][0], move[cnt][1], theta[cnt]);

						Send_Size = sendto(ClientSocket, Buffer, BUFFER_SIZE, 0,
							(struct sockaddr*)&ToServer, sizeof(ToServer));

						// 패킷송신시 에러처리
						if (Send_Size != BUFFER_SIZE)
						{
							std::cout << "sendto() error!" << std::endl;
							exit(0);
						}
												
					}
					else if (inputState == "xb") dx1 = stoi(num);
					else if (inputState == "ya") dx2 = stoi(num);
					else if (inputState == "yb") dy1 = stoi(num);
					else if (inputState == "ca") dy2 = stoi(num);
					else if (inputState == "cb") button[0] = stoi(num);

					inputState = "n";
					num = ch;
				}
			}
			
			//std::cout << inputState;

			//If right-clicked, break the loop
			if (button[1] == 1)
			{
				std::cout << "right clicked, break the loop" << std::endl;
				break;
			}
		}
	}

	//프로그램 종료 전 "END"가 담긴 패킷을 보냄
	sprintf_s(Buffer, "END ");

	Send_Size = sendto(ClientSocket, Buffer, BUFFER_SIZE, 0,
		(struct sockaddr*)&ToServer, sizeof(ToServer));

	closesocket(ClientSocket); //소켓 닫기
	WSACleanup();

	std::cout << "program is terminating" << std::endl;
	return 0;
}

double degree_to_rad(double degree)
{
	return degree * PI / 180;
}

void theta_converter(int dx1, int dy1, int dx2, int dy2, int buttonState1, int buttonState2, int index) {
	int previousIndex = (index + 1) % 2;

	if (dx1 + dx2 == 0) //no rotation
	{
		theta[index] = theta[previousIndex];
		dmove[index][0] = 0.0;
		dmove[index][1] = 0.0;
		move[index][0] = move[previousIndex][0];
		move[index][1] = move[previousIndex][1];

		return;
	}

	double delta_theta = static_cast<double>(dx1 + dx2) * R;
	theta[index] = theta[previousIndex] + (delta_theta / 2.0);

	dmove[index][0] = cos(degree_to_rad(theta[index])) * dx1 - sin(degree_to_rad(theta[index])) * dy1;
	dmove[index][1] = sin(degree_to_rad(theta[index])) * dx1 + cos(degree_to_rad(theta[index])) * dy1;
	move[index][0] = move[previousIndex][0] + (delta_theta / (2.0 * sin(degree_to_rad(delta_theta / 2.0)))) * dmove[index][0];
	move[index][1] = move[previousIndex][1] + (delta_theta / (2.0 * sin(degree_to_rad(delta_theta / 2.0)))) * dmove[index][1];

	theta[index] = theta[index] + delta_theta / 2.0;
}