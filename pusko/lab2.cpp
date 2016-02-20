#include "system/ModelingTime/UniformTime.cpp"
#include "system/Plate.cpp"
#include "system/Components/Resist.cpp"
#include <stdio.h>
#include "system/Components/Conduct.cpp"
#include "system/Components/Induct.cpp"
#include "system/Components/Voltage.cpp"
#include "system/Components/Currency.cpp"

#define R 1
#define L 1
#define C 1
#define dt 1
#define totalTime 2

Time* ModelingTime::timer;

Plate* createPlate(int x , int y , double* voltage){
	Plate* plate = new Plate(x , y , voltage);
	for(int node = 1 ; node <= (x * y) ; node++){
		plate -> addComponent(node , 0 , new Conduct(C));
		if(node % x != 0)
			plate -> addComponent(node , node + 1 , new Resist(R));
		if((node - 1) / x < y - 1)
			plate -> addComponent(node , node + x , new Induct(L));
	}
	return plate;
}


int main(){
	int x = 3;
	int y = 3;
	int count = x * y + 1;
	ModelingTime::start(new UniformTime(dt));
	double* voltage = new double[x * y];
	Plate* plate = createPlate(x , y , voltage);
	double** matrix = new double*[count];
	for(int i = 0 ; i < count ; i++)
		matrix[i] = new double[count + 1];
	plate -> createStiffMatrix(matrix , count + 1, count);

	for(int i = 0 ; i < count ; i++){
		for(int j = 0 ; j < count + 1 ; j++){
			printf("%2.f ", matrix[i][j]);
		}
		printf("\n");
	}


	delete plate;
	delete voltage;
    return 0;
}
