#include <iostream>
#include "mpi.h"

using namespace std;

int main(int argc,char *argv[]){
MPI_Init(&argc,&argv);

int size = MPI::COMM_WORLD.Get_size();


int rank = MPI::COMM_WORLD.Get_rank();
printf("Hello World! I'm process %d and there are %d processes in total!\n",rank,size);
MPI_Finalize();
return 0;
}
