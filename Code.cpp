#include <stdio.h>
#include <tchar.h>
#include "SerialClass.h"	// Library described above
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <sstream>
#include <vector>
#include <fstream>

const float D = 2.834646; //inch
const int CPI = 1200;

const float R = 180.0 / (3.14159265358979323846 * D * CPI);

float theta[2];
// [ [dxi, dyi] , [dxi+1, dyi+1] ….   ]
float dmove[2][2];
// [ [xi, yi] , [xi+1, yi+1] ….   ]
float move[2][2];

void theta_converter(int dx1, int dy1, int dx2, int dy2, int buttonState1, int buttonState2, int index) {
	int previousIndex = (index + 1) % 2;

	if (dx1 + dx2 == 0)
	{
		theta[index] = theta[previousIndex];
		dmove[index][0] = 0.0;
		dmove[index][1] = 0.0;
		move[index][0] = move[previousIndex][0];
		move[index][1] = move[previousIndex][1];

		return;
	}

	float delta_theta = (dx1 + dx2) * R;
	theta[index] = theta[previousIndex] + (delta_theta / 2.0);

	dmove[index][0] = cos(theta[index]) * dx1 - sin(theta[index]) * dy1;
	dmove[index][1] = sin(theta[index]) * dx1 + cos(theta[index]) * dy1;
	move[index][0] = move[previousIndex][0] + (delta_theta / (2.0 * sin(delta_theta / 2.0))) * dmove[index][0];
	move[index][1] = move[previousIndex][1] + (delta_theta / (2.0 * sin(delta_theta / 2.0))) * dmove[index][1];

	theta[index] = theta[index] + delta_theta / 2.0;
}

// application reads from the specified serial port and reports the collected data
int main()
{
	printf("Welcome to the serial test app!\n\n");
	Serial* SP = new Serial("\\\\.\\COM5");    // adjust as needed

	if (SP->IsConnected())
		std::cout << "We're connected\n" << std::endl;

	char incomingData[256] = "";			// don't forget to pre-allocate memory
	//printf("%s\n",incomingData);
	int dataLength = 255;
	int readResult = 0;
	
	std::ofstream fout("/Users/KimDongWoo/Documents/output.txt");
	if (!fout.is_open())
	{
		std::cout << "output file not opened" << std::endl;
		return 1;
	}
	
	

	std::stringstream ss;
	ss.clear();
	ss.str("");
	long micro;
	int dx1, dy1, dx2, dy2;
	int button[2] = { 0, 0 };
	int cnt = 0;

	if (SP->IsConnected()) //to ignore first line of data which is a bug time to time
		while(readResult != 0)
			readResult = SP->ReadData(incomingData, 100);

	while (SP->IsConnected())
	{
		readResult = SP->ReadData(incomingData, sizeof(incomingData)-1);
		incomingData[readResult] = '\0';

		if (readResult != 0)
		{
			ss << incomingData;

			//printing incoming data
			//std::cout << incomingData << std::endl;
			
			while (ss.tellg() != -1 && ss.str().size() - ss.tellg() >= 70)
			{
				ss >> micro;
				while (micro < 1000000 && ss.tellg() != -1) //micro의 범위를 알아내서 구체화할 필요 있음
					ss >> micro;

				ss >> dx1 >> dy1 >> dx2 >> dy2 >> button[0] >> button[1];
				cnt = (cnt + 1) % 2;

				//printing received values
				//std::cout << micro << " " << ss.tellg() << " " << dx1 << " " << dy1 << " " << dx2 << " " << dy2 << " " << button[0] << " " << button[1] << std::endl;

				if (ss.tellg() == -1)
					theta_converter(0, 0, 0, 0, 0, 0, cnt);
				else
					theta_converter(dx1, dy1, -dx2, dy2, button[0], button[1], cnt);

				
				std::cout << std::setw(10) << micro << ") " << "x: " << std::setw(20) << move[cnt][0]
					<< ", y: " << std::setw(20) << move[cnt][1]
					<< ", theta: " << std::setw(20) << theta[cnt] << std::endl;
				
				
				fout << micro << " " << move[cnt][0] << " " << move[cnt][1] << " " << theta[cnt] << std::endl;
			}

			int curPos = ss.tellg();
			if (curPos == -1)
			{
				std::cout << "Reaching the end of buffer, please wait a minute" << std::endl;
				ss.str("");
				ss.clear();
			}
			else
			{
				ss.str(ss.str().substr(ss.tellg()));
				ss.seekp(0, std::ios::end);
			}
				

			//If right-clicked, break the loop
			if (button[1] == 1) break;
		}
	}

	if (fout) fout.close();

	return 0;
}
