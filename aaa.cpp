#include <iostream>
#include <iomanip>
#include <cmath>

const int D = 10;
const int CPI = 200;

float R = 180 / (M_PI* D * CPI);

float theta[20000];
// [ [dxi, dyi] , [dxi+1, dyi+1] ….   ]
float dmove[20000][2];
// [ [xi, yi] , [xi+1, yi+1] ….   ]
float move[20000][2];

void theta_converter( int dx1, int dy1, int dx2, int dy2, int buttonState, int index){
    float delta_theta = (dx1 + dx2) * R;
    theta[index] = theta[index-1] + (delta_theta /2);

    dmove[index][0] = cos(theta[index]) * dx1 - sin(theta[index]) + dy1;
    dmove[index][1] = sin(theta[index]) * dx1 + cos(theta[index]) * dy1;
    move[index][0] = move[index-1][0] +  (delta_theta / (2 * sin(delta_theta / 2) )) * dmove[index][0];
    move[index][1] = move[index-1][1] +  (delta_theta / (2 * sin(delta_theta / 2) )) * dmove[index][1];

    theta[index] = theta[index] + delta_theta/2;
}

int main(void) {
    for (int row =0 ; row <20000; row++) {
        if (row !=0 ) {
            theta_converter(rand() % 10 +1,rand() % 10 +1,rand() % 10 +1,rand() % 10 +1,0, row);
        }

    }
    return 0;
}