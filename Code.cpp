#include <stdio.h>
#include <tchar.h>
#include "SerialClass.h"	// Library described above
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <sstream>
#include <vector>

const float D = 2.834646; //inch
const int CPI = 1200;

const float R = 180.0 / (3.14159265358979323846 * D * CPI);

float theta[40000];
// [ [dxi, dyi] , [dxi+1, dyi+1] ….   ]
float dmove[40000][2];
// [ [xi, yi] , [xi+1, yi+1] ….   ]
float move[40000][2];

void theta_converter(int dx1, int dy1, int dx2, int dy2, int buttonState1, int buttonState2, int index) {
	if (dx1 + dx2 == 0)
	{
		theta[index] = theta[index - 1];
		dmove[index][0] = 0.0;
		dmove[index][1] = 0.0;
		move[index][0] = move[index - 1][0];
		move[index][1] = move[index - 1][1];

		return;
	}

	float delta_theta = (dx1 + dx2) * R;
	theta[index] = theta[index - 1] + (delta_theta / 2.0);

	dmove[index][0] = cos(theta[index]) * dx1 - sin(theta[index]) * dy1;
	dmove[index][1] = sin(theta[index]) * dx1 + cos(theta[index]) * dy1;
	move[index][0] = move[index - 1][0] + (delta_theta / (2.0 * sin(delta_theta / 2.0))) * dmove[index][0];
	move[index][1] = move[index - 1][1] + (delta_theta / (2.0 * sin(delta_theta / 2.0))) * dmove[index][1];

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
			
			while (ss.tellg() != -1 && ss.str().size() - ss.tellg() >= 70)
			{
				ss >> micro;
				while (micro < 100000 && ss.tellg() != -1) //micro의 범위를 알아내서 구체화할 필요 있음
					ss >> micro;

				ss >> dx1 >> dy1 >> dx2 >> dy2 >> button[0] >> button[1];

				//printing incoming values
				//std::cout << micro << " " << dx1 << " " << dy1 << " " << dx2 << " " << dy2 << " " << button[0] << " " << button[1] << std::endl;
				
				if (cnt<40000)
				{
					theta_converter(dx1, dy1, -dx2, dy2, button[0], button[1], ++cnt);
					std::cout << std::setw(5) << cnt << ") " << "x: " << std::setw(20) << move[cnt][0]
						<< ", y: " << std::setw(20) << move[cnt][1]
						<< ", theta: " << std::setw(20) << theta[cnt] << std::endl;
				}
			}

			int curPos = ss.tellg();
			if (curPos == -1)
			{
				std::cout << "ERROR: reaching the end of buffer" << std::endl;
				ss.str("");
				ss.clear();
			}
			else
				ss.str(ss.str().substr(ss.tellg()));

			//If right-clicked, break the loop
			if (button[1] == 1) break;
		}
	}

	return 0;
}
