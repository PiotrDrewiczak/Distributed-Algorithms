#include <iostream>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include "mpi.h"


using namespace std;

int main(int argc, char **argv){
	
	MPI::Init(argc,argv);				 
		
		int rank=MPI::COMM_WORLD.Get_rank();
		int size=MPI::COMM_WORLD.Get_size();
		double* arr2=new double[size*size];
		double *arr3=new double[size*size];	
		int counter=0;
		
		ofstream file;
		ostringstream oss;
		oss<<"log.proc_"<<rank;
		
		srand(rank*2+1*rank+33-rank*7);
	
			double** arr=new double*[size];
				for (int i=0;i<size;i++)
							{
						arr[i]=new double[size];
							}
		
		file.open((oss.str()).c_str());
		
		file <<"Wygenerowalem :"<<endl;
			for(int i=0;i<size;i++)
			{
				for(int j=0;j<size;j++)
					{
					arr[i][j]=(double)rand() / RAND_MAX;;
					file<<arr[i][j];
					}
					file<<endl;
			}
	

			for (int i = 0;i<size;i++)
				{
					for (int j= 0;j<size;j++)
					{
					counter=i*size+j;
					arr2[counter] = arr[i][j];
					}
				}			
			
			if(rank%2==0){
				MPI::COMM_WORLD.Send(arr2,size*size,MPI::DOUBLE,rank+1,0);
				MPI::COMM_WORLD.Recv(arr3,size*size,MPI::DOUBLE,MPI::ANY_SOURCE,0);
				file<<"Odebrałem macierz od "<<rank+1<<endl;
				 }
			
			else{
				MPI::COMM_WORLD.Recv(arr3,size*size,MPI::DOUBLE,MPI::ANY_SOURCE,0);
				MPI::COMM_WORLD.Send(arr2,size*size,MPI::DOUBLE,rank-1,0);
				file<<"Odebrałem macierz od "<<rank-1<<endl;
			}
	
			for(int i=0;i<size;i++){
					for(int j=0;j<size;j++){
						counter=i*size+j;
						file<<arr3[counter];
				}
				file <<endl;
			}
										
			file.close();				 
	MPI::Finalize();
	return 0;
}