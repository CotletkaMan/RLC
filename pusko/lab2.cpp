#include "system/ModelingTime/UniformTime.cpp"
#include "system/Plate.cpp"
#include "system/Components/Resist.cpp"
#include <stdio.h>
#include "system/Components/Conduct.cpp"
#include "system/Components/Induct.cpp"
#include "system/Components/Voltage.cpp"
#include "system/Components/Currency.cpp"
#include "parallels/paralel.h"
#include <pthread.h>
#include <string.h>
#include <sys/time.h>


#define REENTRANT
#define R 1
#define L 1
#define C 0.1
#define dt 0.5
#define totalTime 10.
#define MaxToGnuPlot 10.

double* test = new double[100];

typedef struct{
	int id;
	Plate* localPlate;
	int width;
	double* localMatrix;
	double** cornerMatrix;

	void init(int p_num , Plate* p_localPlate , int p_width){
		id = p_num;
		localPlate = p_localPlate;
		width = p_width;
	}

} argThread;


Time* ModelingTime::timer;
Plate* plate;
double** voltage;
pthread_barrier_t init , firstStep , secondStep;
pthread_mutex_t mx;
argThread* args;
int countThreads;
int x;
int y;

double Ufunction(double time){
	return 5;
}

double Ifunction(double time){
	return -5;
}

void printMat(double* mat , int yi , int xi){
	for(int i = 0 ; i < yi ; i++){
		for(int j = 0 ; j < xi ; j++)
			fprintf(stderr, "%3.3f ", mat[i  * xi + j]);
		fprintf(stderr, "\n");
	}
}

Plate* createPlate(int x , int y , double** voltage){
	Plate* plate = new Plate(x * y + 1, voltage);
	for(int node = 1 ; node <= (x * y) ; node++){
		plate -> addComponent(node , 0 , new Conduct(C));
		if(node % x != 0)
			plate -> addComponent(node , node + 1 , new Resist(R));
		if((node - 1) / x < y - 1)
			plate -> addComponent(node , node + x , new Resist(R));
	}
	plate -> addComponent(0 , 1 , new Voltage(Ufunction));
	plate -> addComponent(0 , x * y , new Currency(Ufunction));
	return plate;
}

void* action(void* v_arg){
	argThread* arg = (argThread*)v_arg;
	int countNodes = arg -> localPlate -> getCountNodes() - 1;
	arg -> localMatrix = new double[countNodes * (countNodes + 1) + 1];
	arg -> cornerMatrix = new double*[countNodes - arg -> width];
	double** localVolt = &arg -> localPlate -> getVoltage()[1];
	FILE* script = openFile("gnu.sh");

	while(ModelingTime::getTime() < totalTime){
		pthread_barrier_wait(&firstStep);
		arg -> localPlate->createStiffMatrix(arg -> localMatrix);

		for(int i = arg -> width , j = 0 ; i < countNodes ; i++ , j++){
			memset(arg -> localMatrix + i * (countNodes + 1) + arg -> width, 0 , (countNodes + 1 - arg -> width) * sizeof(double));
			arg -> cornerMatrix[j] = arg -> localMatrix  + i * (countNodes + 1) + arg -> width;
		}

		for(int i = 0 ; i < arg -> width ; i++){
			for(int j = i + 1 ; j < countNodes ; j++){
				double key = arg -> localMatrix[j * (countNodes + 1) + i] / arg -> localMatrix[i * (countNodes + 1) + i];
				for(int k = i; k < countNodes + 1 ; k++)
					arg -> localMatrix[j * (countNodes + 1) + k] -= arg -> localMatrix[i * (countNodes + 1) + k] * key;
			}
		}

		pthread_barrier_wait(&firstStep);

		if(arg -> id == 0){
			int count = plate -> getCountNodes() - 1;
			double* matrix = new double[count * (count + 1)];
			int countCorner = count - arg -> width * countThreads;
			double** corner = new double*[countCorner];
			memset(matrix , 0 , count * (count + 1) * sizeof(double));
			plate -> createStiffMatrix(matrix);
			for(int i = 0 , it = arg -> width * countThreads ; i < countCorner ; i++ , it++)
				corner[i] = matrix + it * (count + 1) + arg -> width * countThreads;

			for(int i = 0 , delta = 0 ; i < countThreads ; i++){
				int widthCorner = args[i].localPlate -> getCountNodes() - 1 - args[i].width;
				for(int j = 0 ; j < widthCorner ; j++){
					for(int k = 0 ; k < widthCorner ; k++)
						corner[j+ delta][k + delta] += args[i].cornerMatrix[j][k];
					corner[j+ delta][countCorner] += args[i].cornerMatrix[j][widthCorner];
				}
				delta += widthCorner - (countNodes - arg -> width);
			}

			for(int i = 0 ; i < countCorner ; i++){
				for(int j = i + 1 ; j < countCorner ; j++){
					double key = corner[j][i] / corner[i][i];
					for(int k = i; k <= countCorner; k++)
						corner[j][k] -= corner[i][k] * key;
				}
			}

			for(int i = countCorner - 1 ,  h = plate -> getCountNodes() - 1 ; i >= 0 ; i-- , h--){
				for(int j = countCorner - 1 ,  row =  plate -> getCountNodes() - 1  ; j > i ; j-- , row --)
					corner[i][countCorner] -= corner[i][j] * (*voltage[row]);
				(*voltage[h]) = corner[i][countCorner] / corner[i][i];
			}

			delete corner;
			delete matrix;

		}

		pthread_barrier_wait(&secondStep);

		for(int i = arg -> width - 1 ; i >= 0 ; i--){
			for(int j = countNodes - 1 ; j > i; j--)
				arg->localMatrix[i * (countNodes + 1) + countNodes] -= arg->localMatrix[i * (countNodes  + 1) + j] * (*localVolt[j]);
			(*localVolt[i]) = arg->localMatrix[i * (countNodes + 1) + countNodes] / arg -> localMatrix[i * (countNodes + 1) + i];
		}

		pthread_barrier_wait(&init);

		if(arg -> id == 0){
			writeToFile(script , voltage , plate -> getOrder() , y , x);
			ModelingTime::nextStep();
			std :: cerr << ModelingTime :: getTime() << std::endl;
		}
		memset(arg -> localMatrix  , 0 , countNodes * (countNodes + 1) * sizeof(double));
	}
	closeFile(script);
	delete arg -> cornerMatrix;
	delete arg -> localMatrix;
}

int main(int argc , char** argv){
	x = atoi(argv[1]);
	y = atoi(argv[2]);
	countThreads = atoi(argv[3]);
	int countNodes = x * y;

	ModelingTime::start(new UniformTime(dt));

	voltage = new double*[countNodes + 1];
	for(int i = 0 ; i < countNodes + 1 ; i++)
		voltage[i] = new double[1];
	plate = createPlate(x , y , voltage);
	args = new argThread[countThreads];

	struct timeval begin;
	struct timeval end;
	struct timezone zone;

	gettimeofday(&begin, &zone);

	pthread_attr_t p_attr;
	pthread_t* ids = new pthread_t[countThreads - 1];
	pthread_attr_init(&p_attr);
	pthread_attr_setscope(&p_attr , PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&p_attr , PTHREAD_CREATE_JOINABLE);
	pthread_mutex_init(&mx , NULL);
	pthread_barrier_init(&firstStep , NULL , countThreads);
	pthread_barrier_init(&secondStep , NULL , countThreads);
	pthread_barrier_init(&init , NULL , countThreads);

	int *order = plate -> getOrder();

	int** intervals = createBlockOrder(&order[1] , y ,  x , countThreads);

	if(countThreads > 1){
		Plate** plates = new Plate*[countThreads];
		for(int i = 1 ; i < countThreads - 1; i++){
			plates[i] = new Plate(&plate -> getNodes()[intervals[2 * i - 1][0]] , intervals[2 * i + 1][1] - intervals[2 * i - 1][0]);
		}
		plates[0] = new Plate(&plate -> getNodes()[intervals[0][0]] ,  intervals[1][1] - intervals[0][0]);
		plates[countThreads - 1] = new Plate(&plate -> getNodes()[intervals[2 * countThreads - 3][0]] ,  intervals[2 * countThreads - 2][1] - intervals[2 * countThreads - 3][0]);

		for(int i = 1 ; i < countThreads ; i++){
			args[i].init(i , plates[i] , intervals[i * 2][1] - intervals[i * 2][0]);
			pthread_create(&ids[i] , &p_attr , action , (void*)(&args[i]));
		}
		args[0].init(0 , plates[0] , intervals[0][1] - intervals[0][0]);
	}
	else
		args[0].init(0 , plate , intervals[0][1] - intervals[0][0]);



	action((void*)(&args[0]));

	gettimeofday(&end, &zone);
	fprintf(stderr , "Time executing :: %lu on number of process %d\n" , end.tv_sec * 1000000 + end.tv_usec - begin.tv_usec - begin.tv_sec * 1000000 , countThreads);
		

	createScript(x , y , totalTime / dt + 1 , MaxToGnuPlot);

	pthread_attr_destroy(&p_attr);
	pthread_barrier_destroy(&firstStep);
	pthread_barrier_destroy(&init);
	pthread_barrier_destroy(&secondStep);
	pthread_mutex_destroy(&mx);
	delete ids;
	delete plate;
	delete voltage;
    return 0;
}