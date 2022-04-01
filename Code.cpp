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

double l1 = 5, l2 = 8, l3 = 3; //링크의 길이이므로 무조건 길이를 설정해야 한다.
double x, y;
double a1, a2;
double degree1, degree2, degree3; //각도 1,2,3

double move[2][2]; // [ [xi, yi] , [xi+1, yi+1] ….   ]
double dmove[2][2]; // [ [dxi, dyi] , [dxi+1, dyi+1] ….   ]
double theta[2];

double degree_to_rad(double degree);
double rad_to_degree(double rad);
void theta_converter(int dx1, int dy1, int dx2, int dy2, int buttonState1, int buttonState2, int index);

void _2dof_inversekinematics(double x, double y);
void _3dof_inversekinematics(double x, double y, double degree);//역기구학을 푸는 힘수

// application reads from the specified serial port and reports the collected data
int main()
{
	//connect serial port
	printf("Welcome to the serial test app!\n\n");
	Serial* SP = new Serial("\\\\.\\COM5");    // adjust as needed

	if (SP->IsConnected())
		std::cout << "We're connected\n" << std::endl;


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


	//receive data
	char incomingData[2] = "";			// don't forget to pre-allocate memory
	int readResult = 0;
	int dx1(0), dy1(0), dx2(0), dy2(0);
	int button[2] = { 0, 0 };
	int cnt = 0;
	int sumdy = 0;
	std::string inputState = "";
	std::string num = "0";
	move[0][0] = 5;
	move[0][1] = 11;

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
						theta_converter(dx1, dy1, dx2, dy2, button[0], button[1], cnt);
						//std::cout << " " << dx1 << " " << dx2 << " " << dy1 << " " << dy2 << " " << button[0] << " " << button[1] << std::endl;

						_3dof_inversekinematics(move[cnt][0], move[cnt][1], -theta[cnt] + 90);
						std::cout << "x: " << std::setw(10) << move[cnt][0]
							<< ", y: " << std::setw(10) << move[cnt][1]
							<< ", theta1: " << std::setw(10) << degree1
							<< ",  theta2: " << std::setw(10) << degree2
							<< ", theta3: " << std::setw(10) << degree3 << std::endl;
						
						sumdy += dy1;
						

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
					else if (inputState == "ya") dx2 = -stoi(num);
					else if (inputState == "yb") dy1 = -stoi(num);
					else if (inputState == "ca") dy2 = -stoi(num);
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

double rad_to_degree(double rad)
{
	return (rad * 180 / PI);
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

	double delta_theta = (static_cast<double>(dx1) + static_cast<double>(dx2)) * R;
	theta[index] = theta[previousIndex] + (delta_theta / 2.0);

	//std::cout << degree_to_rad(delta_theta) / (2.0 * sin(degree_to_rad(delta_theta / 2.0))) << std::endl;

	dmove[index][0] = (cos(degree_to_rad(theta[index])) * dx1 - sin(degree_to_rad(theta[index])) * dy1) / 1200.0;
	dmove[index][1] = (sin(degree_to_rad(theta[index])) * dx1 + cos(degree_to_rad(theta[index])) * dy1) / 1200.0;
	move[index][0] = move[previousIndex][0] + (degree_to_rad(delta_theta) / (2.0 * sin(degree_to_rad(delta_theta / 2.0)))) * dmove[index][0];
	move[index][1] = move[previousIndex][1] + (degree_to_rad(delta_theta) / (2.0 * sin(degree_to_rad(delta_theta / 2.0)))) * dmove[index][1];

	theta[index] = theta[index] + delta_theta / 2.0;
}

void _2dof_inversekinematics(double x, double y)//기구학을 이용한 좌표계산
{
	double k = ((pow(x, 2) + pow(y, 2) - pow(l1, 2) - pow(l2, 2)) / (2 * l1 * l2));
	a2 = (atan2(sqrt(1 - pow(k, 2)), k));
	a1 = atan2(y, x) - atan2(l2 * sin(a2), l1 + l2 * cos(a2));

	a1 *= 180 / PI;//radian to degree
	a2 *= 180 / PI;//radian to degree
	degree1 = a1;
	degree2 = a2;
}

void _3dof_inversekinematics(double x, double y, double degree)
{
	_2dof_inversekinematics(x - l3 * cos(degree_to_rad(degree)), y - l3 * sin(degree_to_rad(degree)));
	//std::cout << x - l3 * cos(degree_to_rad(degree)) << " " << y - l3 * sin(degree_to_rad(degree)) << std::endl;
	degree3 = degree - (degree1 + degree2);
}