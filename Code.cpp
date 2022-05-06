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
#include "SerialClass.h"	
#pragma comment (lib,"ws2_32.lib")


#define BUFFER_SIZE 128 //버퍼 생성 시에만 사용 default=1024
const double D = 2.834646; //inch
const double CPI = 1200.0;
const double PI = 3.14159265358979323846;
const double R = 180.0 / (PI * D * CPI);
const double l1 = 5; //길이 미리 지정해야함
const double l2 = 8;
const double l3 = 3;
const int TERM = 2;

double move[TERM][2]; // [ [xi, yi] , [xi+1, yi+1] ….   ]
double dmove[TERM][2]; // [ [dxi, dyi] , [dxi+1, dyi+1] ….   ]
double theta[TERM];
double degree1, degree2, degree3;


//단위 변환
double degree_to_rad(double degree);
double rad_to_degree(double rad);

//mouse에서 받은 input을 사용해서 move, dmove, theta를 계산
void theta_converter(int dx1, int dy1, int dx2, int dy2, int buttonState1, int buttonState2, int index);

//inverse kinematics
void _2dof_inversekinematics(double x, double y);
void _3dof_inversekinematics(double x, double y, double degree);//역기구학을 푸는 힘수

//평균이 패드 중앙에 오도록 조정
void diffMean(double centerX, double centerY, double centerTheta);
//x,y,theta 값을 초기화
void reset(int cnt);

int main()
{
	//connect serial port
	printf("Welcome to the serial test app!\n\n");
	Serial* SP = new Serial("\\\\.\\COM5");    // 사용자 pc에 맞춰서 변경해야함

	if (SP->IsConnected())
		std::cout << "We're connected\n" << std::endl;

	//make socket
	WSADATA wsaData;
	SOCKET ClientSocket;
	SOCKADDR_IN ToServer;
	int Send_Size;
	ULONG   ServerPort = 61557; // 서버 포트번호

	char Buffer[BUFFER_SIZE] = {};
	if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR) //winSock 초기화 실패 시, 프로그램 종료
	{
		std::cout << "WinSock 초기화부분에서 문제 발생 " << std::endl;
		WSACleanup();
		exit(0);
	}

	memset(&ToServer, 0, sizeof(ToServer));
	ToServer.sin_family = AF_INET;
	ToServer.sin_addr.s_addr = inet_addr("127.0.0.1");
	ToServer.sin_port = htons(ServerPort);
	ClientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);//

	if (ClientSocket == INVALID_SOCKET) //소켓 생성 실패 시, 프로그램 종료
	{
		std::cout << "소켓을 생성할수 없습니다." << std::endl;
		closesocket(ClientSocket);
		WSACleanup();
		exit(0);
	}

	std::cout << std::fixed;
	std::cout.precision(2);

	//receive data
	char incomingData[2] = "";
	int readResult = 0;
	int dx1(0), dy1(0), dx2(0), dy2(0);
	double x1, y1, x2, y2;
	int button[2] = { 0, 0 };
	int cnt = 0;
	std::string inputState = "";
	std::string num = "0";
	move[0][0] = l1;  //초기 x값
	move[0][1] = l2 + l3; //초기 y값

	while (SP->IsConnected())
	{
		readResult = SP->ReadData(incomingData, 1);
		if (readResult != 0)
		{
			char ch = incomingData[0];
			//std::cout << ch;
			
			switch (ch)
			{
			case 'f':
				std::cout << "end of clutching" << std::endl;
				reset(cnt);
				break;
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

						//TERM 마다, x, y, theta의 평균이 l1, l2 + l3, 0이 되도록 조정
						//if (cnt == TERM - 1) diffMean(l1, l2 + l3, 0);

						cnt = (cnt + 1) % TERM;
						theta_converter(dx1, dy1, dx2, dy2, button[0], button[1], cnt);
						//std::cout << " " << dx1 << " " << dx2 << " " << dy1 << " " << dy2 << " " << button[0] << " " << button[1] << std::endl;

						x1 = l1 * cos(degree_to_rad(degree1));
						y1 = l1 * sin(degree_to_rad(degree1));
						x2 = x1 + l2 * cos(degree_to_rad(degree1 + degree2));
						y2 = y1 + l2 * sin(degree_to_rad(degree1 + degree2));

						_3dof_inversekinematics(move[cnt][0], move[cnt][1], -theta[cnt] + 90);
						std::cout << "x1: " << std::setw(5) << x1
							<< ", y1: " << std::setw(5) << y1
							<< ", x2: " << std::setw(5) << x2
							<< ", y2: " << std::setw(5) << y2
							<< ", x: " << std::setw(5) << move[cnt][0]
							<< ", y: " << std::setw(5) << move[cnt][1]
							<< ", th: " << std::setw(5) << theta[cnt]
							<< ", th1: " << std::setw(5) << degree1
							<< ",  th2: " << std::setw(5) << degree2
							<< ", th3: " << std::setw(5) << degree3 << std::endl;

				/*		std::cout << "dx1: " << std::setw(3) << dx1
							<< ", dx2: " << std::setw(3) << dx2
							<< ", dy1: " << std::setw(3) << dy1
							<< ", dy3: " << std::setw(3) << dy2
							<< ", th: " << std::setw(5) << theta[cnt] << std::endl;*/
						
						

						//send packet
						sprintf_s(Buffer, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf \n", x1, y1, x2, y2, move[cnt][0], move[cnt][1], degree1, degree2, degree3, theta[cnt]);
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
	int previousIndex = index == 0 ? TERM - 1 : index - 1;
	double delta_theta = (static_cast<double>(dx1) + static_cast<double>(dx2)) * R;
	theta[index] = theta[previousIndex] + (delta_theta / 2.0);

	//std::cout << degree_to_rad(delta_theta) / (2.0 * sin(degree_to_rad(delta_theta / 2.0))) << std::endl;
	double th = degree_to_rad(theta[index]);
	double d_th = degree_to_rad(delta_theta);

	dmove[index][0] = (cos(th) * dx1 + sin(th) * dy1) / CPI;
	dmove[index][1] = (-1 * sin(th) * dx1 + cos(th) * dy1) / CPI;
	for (int i = 0; i < 2; i++)
		move[index][i] = move[previousIndex][i] + (d_th == 0 ? 1 : (d_th / (2.0 * sin(d_th / 2.0)))) * dmove[index][i];

	theta[index] = theta[index] + delta_theta / 2.0;
}

void _2dof_inversekinematics(double x, double y)
{
	double k = ((pow(x, 2) + pow(y, 2) - pow(l1, 2) - pow(l2, 2)) / (2 * l1 * l2));
	double a2 = (atan2(sqrt(1 - pow(k, 2)), k));
	double a1 = atan2(y, x) - atan2(l2 * sin(a2), l1 + l2 * cos(a2));

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

void diffMean(double centerX, double centerY, double centerTheta)
{
	int half = TERM / 2;

	double sumX = 0, sumY = 0, sumTh = 0;
	for (int i = half; i < TERM; i++)
	{
		sumX += move[i][0];
		sumY += move[i][1];
		sumTh += theta[i];
	}

	move[TERM - 1][0] -= sumX / half - centerX;
	move[TERM - 1][1] -= sumY / half - centerY;
	theta[TERM - 1] -= sumTh / half - centerTheta;
}

void reset(int cnt)
{
	move[cnt][0] = l1;
	move[cnt][1] = l2 + l3;
	theta[cnt] = 0;
}