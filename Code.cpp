#include <stdio.h>
#include <iostream>
#include <winsock2.h> // ���� ��� ���� 
#include <windows.h> 
#include <tchar.h>
#include <string>
#include <iomanip>
#include <cmath>
#include <vector>
#include <fstream>
#include "SerialClass.h"
#include  <signal.h>

#include "myologger.h"
#include "motion_capture.h"
#include <thread>
#pragma comment (lib,"ws2_32.lib")

//bool PRINT_LOG = true;
//bool RECORDING = true;
//std::mutex myo_mutex;

using std::fstream;
using std::to_string;

#define BUFFER_SIZE 128 //���� ���� �ÿ��� ��� default=1024
const double D = 2.834646; //inch
const double CPI = 1200.0;
const double PI = 3.14159265358979323846;
const double R = 180.0 / (PI * D * CPI);
const double l1 = 5; //���� �̸� �����ؾ���
const double l2 = 8;
const double l3 = 3;
const int TERM = 2;

double move[TERM][2]; // [ [xi, yi] , [xi+1, yi+1] ��.   ]
double dmove[TERM][2]; // [ [dxi, dyi] , [dxi+1, dyi+1] ��.   ]
double theta[TERM];
double degree1, degree2, degree3;

//extern std::chrono::time_point<clock_> begin_time;

int c = 0;

//���� ��ȯ
double degree_to_rad(double degree);
double rad_to_degree(double rad);

//mouse���� ���� input�� ����ؼ� move, dmove, theta�� ���
void theta_converter(int dx1, int dy1, int dx2, int dy2, int buttonState1, int buttonState2, int index);

//inverse kinematics
void _2dof_inversekinematics(double x, double y);
void _3dof_inversekinematics(double x, double y, double degree);//���ⱸ���� Ǫ�� ����

//����� �е� �߾ӿ� ������ ����
void diffMean(double centerX, double centerY, double centerTheta);
//x,y,theta ���� �ʱ�ȭ
void reset(int cnt);

void file_out(std::ofstream& file, int x, int y, int x1, int y1, int x2, int y2, double theta, double degree1, double degree2, double degree3);

// cntr c handler

void     INThandler(int);

int main()
{
	//connect serial port
	printf("Welcome to the serial test app!\n\n");
	Serial* SP = new Serial("\\\\.\\COM5");    // ����� pc�� ���缭 �����ؾ���

	if (SP->IsConnected())
		std::cout << "We're connected\n" << std::endl;

	//make socket
	WSADATA wsaData;
	SOCKET ClientSocket;
	SOCKADDR_IN ToServer;
	int Send_Size;
	ULONG   ServerPort = 61557; // ���� ��Ʈ��ȣ


	char Buffer[BUFFER_SIZE] = {};
	if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR) //winSock �ʱ�ȭ ���� ��, ���α׷� ����
	{
		std::cout << "WinSock �ʱ�ȭ�κп��� ���� �߻� " << std::endl;
		WSACleanup();
		exit(0);
	}

	memset(&ToServer, 0, sizeof(ToServer));
	ToServer.sin_family = AF_INET;
	ToServer.sin_addr.s_addr = inet_addr("127.0.0.1");
	ToServer.sin_port = htons(ServerPort);
	ClientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);//

	if (ClientSocket == INVALID_SOCKET) //���� ���� ���� ��, ���α׷� ����
	{
		std::cout << "������ �����Ҽ� �����ϴ�." << std::endl;
		closesocket(ClientSocket);
		WSACleanup();
		exit(0);
	}

	std::cout << std::fixed;
	std::cout.precision(2);

	//ctrl c handler
	signal(SIGINT, INThandler);


	//receive data
	char incomingData[2] = "";
	int readResult = 0;
	int dx1(0), dy1(0), dx2(0), dy2(0);
	double x1, y1, x2, y2;
	int button[2] = { 0, 0 };
	int cnt = 0;
	std::string inputState = "";
	std::string num = "0";
	move[0][0] = l1;  //�ʱ� x��
	move[0][1] = l2 + l3; //�ʱ� y��

	std::ofstream mouseOutFile;
	mouseOutFile.open("rawdata/mouse.csv");
	if (!mouseOutFile.is_open())
	{
		std::cout << "mouse not opened" << std::endl;
		return 1;
	}

	//myo
	std::thread* mt = new std::thread(LogMyoArmband, "myoarmband");
	if (mt) mt->detach();
	else std::printf("Failed to start Myo Armband Thread\n");

	//motion capture
	std::thread* motive = new std::thread(logMotive);
	if (motive) motive->detach();
	else std::printf("Failed to start Motive Thraed\n");


	while (SP->IsConnected() && c != 3)
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

						//TERM ����, x, y, theta�� ����� l1, l2 + l3, 0�� �ǵ��� ����
						//if (cnt == TERM - 1) diffMean(l1, l2 + l3, 0);

						cnt = (cnt + 1) % TERM;
						theta_converter(dx1, dy1, dx2, dy2, button[0], button[1], cnt);
						//std::cout << " " << dx1 << " " << dx2 << " " << dy1 << " " << dy2 << " " << button[0] << " " << button[1] << std::endl;

						x1 = l1 * cos(degree_to_rad(degree1));
						y1 = l1 * sin(degree_to_rad(degree1));
						x2 = x1 + l2 * cos(degree_to_rad(degree1 + degree2));
						y2 = y1 + l2 * sin(degree_to_rad(degree1 + degree2));

						_3dof_inversekinematics(move[cnt][0], move[cnt][1], -theta[cnt] + 90);
						file_out(mouseOutFile, move[cnt][0], move[cnt][1], x1, y1, x2, y2, theta[cnt], degree1, degree2, degree3);
						/*std::cout << "x1: " << std::setw(5) << x1
							<< ", y1: " << std::setw(5) << y1
							<< ", x2: " << std::setw(5) << x2
							<< ", y2: " << std::setw(5) << y2
							<< ", x: " << std::setw(5) << move[cnt][0]
							<< ", y: " << std::setw(5) << move[cnt][1]
							<< ", th: " << std::setw(5) << theta[cnt]
							<< ", th1: " << std::setw(5) << degree1
							<< ",  th2: " << std::setw(5) << degree2
							<< ", th3: " << std::setw(5) << degree3 << std::endl;*/

							/*		std::cout << "dx1: " << std::setw(3) << dx1
										<< ", dx2: " << std::setw(3) << dx2
										<< ", dy1: " << std::setw(3) << dy1
										<< ", dy3: " << std::setw(3) << dy2
										<< ", th: " << std::setw(5) << theta[cnt] << std::endl;*/



										//send packet
						sprintf_s(Buffer, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf \n", x1, y1, x2, y2, move[cnt][0], move[cnt][1], degree1, degree2, degree3, theta[cnt]);
						Send_Size = sendto(ClientSocket, Buffer, BUFFER_SIZE, 0,
							(struct sockaddr*)&ToServer, sizeof(ToServer));

						// ��Ŷ�۽Ž� ����ó��
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

			/*std::cout << inputState;
			char temp;
			std::cin.get(temp);
			std::cout << temp << std::endl;*/

			//If right-clicked, break the loop
			if (button[1] == 1)
			{
				std::cout << "right clicked, break the loop" << std::endl;
				break;
			}
		}
	}

	//���α׷� ���� �� "END"�� ��� ��Ŷ�� ����
	sprintf_s(Buffer, "END ");
	Send_Size = sendto(ClientSocket, Buffer, BUFFER_SIZE, 0,
		(struct sockaddr*)&ToServer, sizeof(ToServer));

	closesocket(ClientSocket); //���� �ݱ�
	WSACleanup();

	
	if (mouseOutFile) mouseOutFile.close();

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

void file_out(std::ofstream& file, int x, int y, int x1, int y1, int x2, int y2, double theta, double degree1, double degree2, double degree3) {
	//time_t timer;
	//struct tm* t;
	//timer = time(NULL); // 1970�� 1�� 1�� 0�� 0�� 0�ʺ��� �����Ͽ� ��������� ��
	//t = localtime(&timer); // �������� ���� ����ü�� �ֱ�


	//std::string time_str(to_string(t->tm_hour) + ":" + to_string(t->tm_min) + ":" + to_string(t->tm_sec));
	std::string time_str = to_string(elapsed());

	if (!file.is_open()) {
		std::cout << "failed to open " << '\n';
	}
	else {
		file << time_str << "," << to_string(x) << "," << to_string(y) << "," << to_string(x1) << "," << to_string(y1) << "," << to_string(x2) << "," << to_string(y2) << "," << to_string(theta) << "," << to_string(degree1) << "," << to_string(degree2) << "," << to_string(degree3) << std::endl;
	}
	//std::cout << "end!" << std::endl;
}

void  INThandler(int sig)
{
	char  j;

	signal(sig, SIG_IGN);
	printf("OUCH, did you hit Ctrl-C?\n"
		"Do you really want to quit? [y/n] ");
	j = getchar();
	if (j == 'y' || j == 'Y') {
		c = 3;
		signal(SIGINT, INThandler);
	}
	else
		signal(SIGINT, INThandler);
	getchar(); // Get new line character
}