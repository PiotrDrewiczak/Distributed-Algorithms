#include <iostream>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include <string>
#include <math.h>
#include <sstream>
#include "mpi.h"
using namespace std;


int fileSize(const char *add){
    ifstream mySource;
    mySource.open(add, ios_base::binary);
    mySource.seekg(0,ios_base::end);
    int size = mySource.tellg();
    mySource.close();
    return size=size/40;
}

void readNumbers(const char *add,double **array,int counter){
	ifstream file(add);
	for(int i=0;i<counter;i++)
	{
		for(int j=0;j<3;j++){
			file >> array[i][j];
		}
	}
}

void readFile(const char *add,double **array,int counter,int rank,int size){
	MPI::File file;
	MPI::Status status;
	const int oneLine=40;
	int p=counter/size;  
	int	r=counter%size; 	
	
	char *array2=new char[counter*oneLine];
	char *array3=new char[counter*oneLine];
	int pom;
	file = MPI::File::Open(MPI::COMM_WORLD,add,MPI_MODE_RDONLY,MPI::INFO_NULL);
	if(rank<r)
	{
		file.Seek(rank*(p+1)*oneLine,MPI_SEEK_SET);	
		file.Read(array2,(p+1)*oneLine,MPI::CHAR);
	}
	else{
		file.Seek((r*(p+1)+(rank-r)*p)*oneLine,MPI_SEEK_SET);	
		file.Read(array2,p*oneLine,MPI::CHAR);
	}
	
	file.Close();
	
	if(rank<r)
	{
		pom=rank*(p+1);
		for(int i=0;	i<p+1	;i++)
			{
			for (int j = 0; j < 3; j++)
				{
				copy(array2 + (i * 40) + (j * 13), array2 + (i * 40) + (j * 13) + 10, array3);
				array[pom][j] = atof(array3);
				}	
				pom++;
			}
	}
	else
	{
		pom=r*(p+1)+(rank-r)*p;
		for(int i=0;	i<p	;i++)
			{
			for (int j = 0; j < 3; j++)
				{
				copy(array2 + (i * 40) + (j * 13), array2 + (i * 40) + (j * 13) + 10, array3);
				array[pom][j] = atof(array3);	
				}
				pom++;
			}
					
	}							
			
}

double* Q(double **array,int counter,int size,int rank){
	static double xyzVectors[3];
	int p=counter/size;
	int r=counter%size;
	
	if(rank<r){
	for(int i=rank*(p+1);	i<(rank+1)*(p+1)	;i++){
		xyzVectors[0]+=array[i][0];
		xyzVectors[1]+=array[i][1];
		xyzVectors[2]+=array[i][2];
		}	
	}	
	else{
	for(int i=r*(p+1)+(rank-r)*p;	i<r*(p+1)+(rank-r)*p+p	;i++)
		{
		xyzVectors[0]+=array[i][0];
		xyzVectors[1]+=array[i][1];
		xyzVectors[2]+=array[i][2];
		}
	}
	
		return xyzVectors;
}

double L(double **array,int counter,int size,int rank){
	double sum;
	double sumOfPow;
	int p=counter/size;   
	int r=counter%size; 
	
	if(rank < r){
		for(int i=rank*(p+1);	i<(rank+1)*(p+1)	;i++)
		{
			for(int j=0;j<3;j++)
				{
			sumOfPow+=pow(array[i][j],2);	
				}
			sum+=sqrt(sumOfPow);
			sumOfPow=0;
		}
	}
	else{
		for(int i=r*(p+1)+(rank-r)*p;	i<r*(p+1)+(rank-r)*p+p	;i++)
		{
			for(int j=0;j<3;j++)
				{
			sumOfPow+=pow(array[i][j],2);	
				}
			sum+=sqrt(sumOfPow);
			sumOfPow=0;
		}
	}
		return sum;
}

double* printMethod(double startTime,double readData,double processData,double reduceps){
	static double array[3];
	array[0]=readData-startTime;
	array[1]=processData-readData;
	array[2]=reduceps-startTime;

	return array;
}

void writeTimesIntoFile(int size,double *timeOfVectors){
	ofstream file;
	double totalRead;
	double totalProcess;
	double totalReduce;
	double total;
	file.open("pTimes.txt");	
	for(int i=0;i<size;i++)
	{
		file<<"Proc ("<<i<<")"<<endl;
		for(int j=i*3;j<3+(i*3);j=3+(i*3))
			{
		file<<"readData:	"<<timeOfVectors[j]<<endl;
		totalRead+=timeOfVectors[j];
		file<<"processData:	"<<timeOfVectors[j+1]<<endl;
		totalProcess+=timeOfVectors[j+1];
		file<<"reduceps:	"<<timeOfVectors[j+2]<<endl;
		totalReduce+=timeOfVectors[j+2];
		file<<"total:	"<<timeOfVectors[j]+timeOfVectors[j+1]+timeOfVectors[j+2]<<endl;
		total+=timeOfVectors[j]+timeOfVectors[j+1]+timeOfVectors[j+2];
		file<<endl;
			}		
	}
	file<<"Total timings:	"<<endl;
	file<<"readData:	"<<totalRead<<endl;
	file<<"processData:	"<<totalProcess<<endl;
	file<<"reduceps:	"<<totalReduce<<endl;
	file<<"total:	"<<total<<endl;	
	file.close();	
}




int main(int argc,char**argv){
	
	MPI::Init(argc,argv);
	int size=MPI::COMM_WORLD.Get_size();
	int rank=MPI::COMM_WORLD.Get_rank();
	
	double startTime,readData,processData,reduceps;	
	double *wsk; // pointer to return Q
	double sumOfVectors[3];	// ARRAY FOR RECIVE VALUES FROM REDUCE METHOD (Q).
	
	
	double medumLargeSum;		// variable for L value
	double recvMediumLargeSum;	// variable for recived L value for MASTER
	
	
	double *wsk2; // pointer to return Times;
	startTime= MPI::Wtime();
	int linesCounter=fileSize("v01.dat");
	
	double **bufor= new double*[linesCounter];
	for(int i=0;i<linesCounter;i++){
			bufor[i]=new double[3];
		}
		
		if(argc==1 && (strcmp("rownolegle",argv[2])==0)){
			readFile("v01.dat",bufor,linesCounter,rank,size);
		}
		else
			readNumbers("v01.dat",bufor,linesCounter);
		
	
	readData= MPI::Wtime();
	
	wsk=Q(bufor,linesCounter,size,rank);
	
	medumLargeSum=L(bufor,linesCounter,size,rank);
	
	processData=MPI::Wtime();
	
	MPI::COMM_WORLD.Reduce(&medumLargeSum,&recvMediumLargeSum,1,MPI::DOUBLE,MPI::SUM,0);	
	MPI::COMM_WORLD.Reduce(wsk,sumOfVectors,3,MPI::DOUBLE,MPI::SUM,0);
	
	reduceps=MPI::Wtime();
	wsk2=printMethod(startTime,readData,processData,reduceps);
	
	
	double *timeOfVectors = new double[size*3]; // ARRAY FOR TIMES		
	MPI::COMM_WORLD.Gather(wsk2,3,MPI::DOUBLE,timeOfVectors,3,MPI::DOUBLE,0);
	
	if(rank==0)
	{
		printf("L=%f\n",recvMediumLargeSum/linesCounter);
		for(int i=0;i<3;i++){
		printf("%0.8f\n",sumOfVectors[i]/linesCounter); // PRINT Q
		}
		writeTimesIntoFile(size,timeOfVectors);
	}
	
	MPI::Finalize();
	return 0;
}