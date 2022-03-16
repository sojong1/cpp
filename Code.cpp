#include <stdio.h>
#include <tchar.h>
#include "SerialClass.h"	// Library described above
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <sstream>
#include <vector>

const int D = 10;
const int CPI = 200; //mm

float R = 180 / (3.14159265358979323846 * D * CPI);

float theta[20000];
// [ [dxi, dyi] , [dxi+1, dyi+1] ….   ]
float dmove[20000][2];
// [ [xi, yi] , [xi+1, yi+1] ….   ]
float move[20000][2];

void theta_converter(int dx1, int dy1, int dx2, int dy2, int buttonState, int index) {
	float delta_theta = (dx1 + dx2) * R;
	theta[index] = theta[index - 1] + (delta_theta / 2);

	dmove[index][0] = cos(theta[index]) * dx1 - sin(theta[index]) + dy1;
	dmove[index][1] = sin(theta[index]) * dx1 + cos(theta[index]) * dy1;
	move[index][0] = move[index - 1][0] + (delta_theta / (2 * sin(delta_theta / 2))) * dmove[index][0];
	move[index][1] = move[index - 1][1] + (delta_theta / (2 * sin(delta_theta / 2))) * dmove[index][1];

	theta[index] = theta[index] + delta_theta / 2;
}

/*
for (int row = 1; row < 20000; row++) {
		theta_converter(rand() % 10 + 1, rand() % 10 + 1, rand() % 10 + 1, rand() % 10 + 1, 0, row);

}
*/


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
	int dx1, dy1, dx2, dy2, dx, dy, button(0);
	int cnt = 0;

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
				while (micro < 10000000 && ss.tellg() != -1) //micro의 범위를 알아내서 구체화할 필요 있음
					ss >> micro;

				ss >> dx1 >> dy1 >> dx2 >> dy2 >> dx >> dy >> button;
				std::cout << micro << " " << dx1 << " " << dy1 << " " << dx2 << " " << dy2 << " " << dx << " " << dy << " " << button << std::endl;
				theta_converter(dx1, dy1, dx2, dy2, button, ++cnt);
			}

			int curPos = ss.tellg();
			if (curPos == -1)
			{
				std::cout << "ERROR: reaching the end of buffer" << std::endl;
				ss.clear();
				ss.str("");
			}
			else
				ss.str(ss.str().substr(ss.tellg()));

			if (button == 2) break;
		}
	}

	for (int i = 0; i < 1000; i++)
		std::cout << "x: " << move[i][0] << ", y: " << move[i][1] << ", theta: " << theta[i] << std::endl;

	return 0;
}
