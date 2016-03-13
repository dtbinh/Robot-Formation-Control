#include <math.h>
#include <cstdlib>
#include <iostream>
#include <time.h>
//This cc is for the use of calculating out the positions for the robots in random
using namespace std;

double drand(double min, double max)
{
    double d = (double)rand() / RAND_MAX;
    return min + d * (max - min);
}

int main(int argc, char **argv){
	srand(time(NULL));
	double radius = atof(argv[1]);
	radius *= 0.0254;
	radius *= radius;
	for(int i = 0; i < 6; i++){
		int dir = rand() % 360;
		double x = drand(0, radius);
		double y = drand(0, radius - x);
		double neg_x = drand(-1, 1);
		double neg_y = drand(-2, 2);
		
		x = sqrt(x);
		if(neg_x >= 0) x *= -1;		
		y = sqrt(y);
		if(neg_y >= 0) y *= -1;
		cout << "  pose [" <<  x << " " << y << " " << 0 << " " << dir << "]" << endl;
	}
}
