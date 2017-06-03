
#include <iostream>
#include <cstdlib>
#include <ctime>
#include "mpi.h"

using namespace std;

int main(int argc, char **argv){

MPI::Init(argc, argv);


int rank=MPI::COMM_WORLD.Get_rank();
int size=MPI::COMM_WORLD.Get_size();
int n=0;
int sum;
 
srand(rank*2+1+rank);
  
  if(rank==0){
    MPI::Status status;
    for(int i=0;i<size-1;i++){
    MPI::COMM_WORLD.Recv(&n,1,MPI::INTEGER,MPI::ANY_SOURCE,0,status);
    sum+=n;
    printf("I received %d from processor %d\n",n,status.Get_source());
            }
    printf("MASTER:Sum of the numbers %d\n",sum);
    for (int j=1;j<size;j++){
    MPI::COMM_WORLD.Send(&sum,1,MPI::INTEGER,j,0);
            }
}

if(rank!=0){
  MPI::Status status;
    n=(rand()%size)+1;
    MPI::COMM_WORLD.Send(&n,1,MPI::INTEGER,0,0);
    printf("Sended from processor %d a number %d\n",rank,n);
    MPI::COMM_WORLD.Recv(&sum,1,MPI::INTEGER,0,0,status);
    printf("Sum=%d from a processor %d\n",sum,rank);
          }

MPI::Finalize();
return 0;
}