/*  YOUR_FIRST_NAME: Jayant
 *  YOUR_LAST_NAME:	Solanki
 *  YOUR_UBIT_NAME: jayantso
 */
#ifndef A1_HPP
#define A1_HPP

#include <vector>
#include <mpi.h>
#include <unistd.h>
#include <algorithm>


int connected_components(std::vector<signed char>& A, int n, int q, const char* out, MPI_Comm comm) {
    // ..code goes downward
    int rank;
    int size;
    MPI_Comm col_comm;//communicator for columnwise
    MPI_Comm row_comm;//communicator for row-wise
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    int col = rank % q;
    int row = rank / q;
    int col_rank, col_size;
    std::vector<signed char> Ptemp; //declaring temporary parent sub-matrix
    std::vector<signed char> M; //declaring temp sub-matrix, helper matrix
    std::vector<signed char> Mtemp; //declaring temp sub-matrix
    std::vector<signed char> Q; //declaring temp sub-matrix
    std::vector<signed char> P; //declaring parent sub-matrix
    std::vector<signed char> PPrime; //declaring parent sub-matrix
    std::vector<signed char> Dummy; //declaring parent sub-matrix
    int b = n / q;
    int max = 0;
    int iteration_limit = 300;
    int tree_hanging = 1;//flag for implementing tree hanging part, set to 0 for otherwise
    int count =0;
    int success_checker = 0;//check if the block level P is same or not then only break the loop, it should be equal to processor count
    // printf("size is %d \n",size);
    // printf("q is %d \n",q);
    Ptemp.resize(b * b, 0);
    P.resize(b * b, 0);
    Q.resize(b * b, 0);
    M.resize(b * b, 0);
    Mtemp.resize(b * b, 0);
    PPrime.resize(b * b, 0);
    Dummy.resize(b * b, 0);
    // sequential processing here for each processor
    //initial P
    for (int i = 0; i<b; i++)
    {
    	for (int j = 0; j<b; j++)
    	{
    		if (A[i * b + j] == 1){
    			Ptemp[i * b + j] = row*b+i;//assigning the highest row-wise vertex
    			if(Ptemp[0 + j]<=Ptemp[i * b + j]){//storing the local columnwise vertex at the start of the column
    				Ptemp[0 + j] = Ptemp[i * b + j];
    			}
     		}
    			
    		else
    			Ptemp[i * b + j] = 0;
    		// P[i * b + j] = 0;//piggybacking, initialising other matrices with zero values
    		// M[i * b + j] = 0;//piggybacking
    	}
    }
    // for (int i = 0; i<b; i++)//replicating locally found max vertex in each process
    // {
    // 	for (int j = 0; j<b; j++)
    // 	{
    // 		Ptemp[i * b + j] = Ptemp[0 + j];
    // 	}
    // }
    //creating communicators for the performing ALLREDUCE columnwise
    // int color = rank%q;
    MPI_Comm_split(comm, (rank%q), rank, &col_comm);
    // MPI_Comm_rank(col_comm, &col_rank);
    // MPI_Comm_size(col_comm, &col_size);
    // printf("WORLD RANK/SIZE: %d/%d \t ROW RANK/SIZE: %d/%d\n",rank, size, col_rank, col_size);
    for (int j = 0; j<b; j++)
    	{	
    		max = Ptemp[j];
    		// printf("max %d", max);
    		MPI_Allreduce(&max, &P[j], 1, MPI_INT, MPI_MAX, col_comm);
    	}
    MPI_Comm_free(&col_comm);
    for (int i = 0; i<b; i++)//replicating globally found max vertex in each process, along column
    {
    	for (int j = 0; j<b; j++)
    	{
    		P[i * b + j] = P[0 + j];
    	}
    }
///////////////////////////initilisation of Parent P matrix is done///////////////////////////////////////////////
    while(1)//repeat until converge or die after 300 iteration
    {
	    for (int i = 0; i<b; i++)//creating the helper matrix M
	    {
	    	for (int j = 0; j<b; j++)
	    	{
	    		if (A[i * b + j] == 1){
	    			M[i * b + j] = P[i * b + j];//assigning the respective max vertex
	     		}
	    	}
	    }
	    //now applying global allreduce row-wise on M
	    //first finding the max vertex at block level and storing into Mtemp
	    Mtemp=M;
	    for (int i = 0; i<b; i++)
	    {
	    	for (int j = 0; j<b; j++)
	    	{
				if(Mtemp[i * b]<=Mtemp[i * b + j]){//storing the block level rowwise max vertex at the start of the row
					Mtemp[i * b] = Mtemp[i * b + j];
				}
	    	}
	    }
	  
	    //creating communicators for the performing ALLREDUCE row-wise
	    // int color = rank%q;
	    MPI_Comm_split(comm, (rank/q), rank, &row_comm);
	    int row_rank, row_size;
	    // MPI_Comm_rank(row_comm, &row_rank);
	    // MPI_Comm_size(row_comm, &row_size);
	    // printf("WORLD RANK/SIZE: %d/%d \t ROW RANK/SIZE: %d/%d\n",rank, size, row_rank, row_size);
	    for (int j = 0; j<b; j++)
		{	
			max = Mtemp[j*b];
			// printf("max %d", max);
			MPI_Allreduce(&max, &Q[j*b], 1, MPI_INT, MPI_MAX, row_comm);//storing resultant value in matrix Q
		}
		for (int i = 0; i<b; i++)//replicating Q along row-wise
	    {
	    	for (int j = 0; j<b; j++)
	    	{
	    		Q[i * b + j] = Q[i*b];
	    	}
	    }
	    MPI_Comm_free(&row_comm);
	    //finding Q
	    for (int i = 0; i<b; i++)
	    {
	    	for (int j = 0; j<b; j++)
	    	{
	    		if (Q[i * b + j] == j+col*b){//adding col*b, for adjusting the actual column number
	    			M[i * b + j] = P[i * b + j];//assigning the respective max vertex
	     		}
	    	}
	    }
	    //now applying global allreduce row-wise again on M
	    //first finding the max vertex at block level
	    Mtemp=M;
	    for (int i = 0; i<b; i++)
	    {
	    	for (int j = 0; j<b; j++)
	    	{
	    		if(Mtemp[i * b]<=Mtemp[i * b + j]){//storing the block level rowwise max vertex at the start of the row
					Mtemp[i * b] = Mtemp[i * b + j];
				}
	    	}
	    }
	     //creating communicators for the performing ALLREDUCE row-wise on helper Matrix Mtemp
	    // int color = rank%q;
	    MPI_Comm_split(comm, (rank/q), rank, &row_comm);
	    // MPI_Comm_rank(row_comm, &row_rank);
	    // MPI_Comm_size(row_comm, &row_size);
	    // printf("WORLD RANK/SIZE: %d/%d \t ROW RANK/SIZE: %d/%d\n",rank, size, row_rank, row_size);
	    for (int j = 0; j<b; j++)
		{	
			max = Mtemp[j*b];
			// printf("max %d", max);
			MPI_Allreduce(&max, &PPrime[j*b], 1, MPI_INT, MPI_MAX, row_comm);//storing resultant value in matrix Q
		}
		for (int i = 0; i<b; i++)//replicating PPrime along row-wise
	    {
	    	for (int j = 0; j<b; j++)
	    	{
	    		PPrime[i * b + j] = PPrime[i*b];
	    	}
	    }
	    if(tree_hanging==1){

		    MPI_Comm_free(&row_comm);
		    //////////////////////////////////////////////////Now the tree hanging part///////////////////////////////////////////
			M = Dummy;//reinitialising helper matrix to zeros
			for (int i = 0; i<b; i++)
		    {
		    	for (int j = 0; j<b; j++)
		    	{
		    		if (P[i * b + j] == i+row*b){//adding row*b, for adjusting the actual row number
		    			M[i * b + j] = PPrime[i * b + j];//assigning the respective max vertex
		     		}
		    	}
		    }
		    Mtemp = M;//Setting Mtemp as M for finding the local All reduce along row wise
		    //now applying global allreduce row-wise on M
		    //first finding the max vertex at block level
		    Mtemp=M;
		    for (int i = 0; i<b; i++)
		    {
		    	for (int j = 0; j<b; j++)
		    	{
		    		if(Mtemp[i * b]<=Mtemp[i * b + j]){//storing the block level rowwise max vertex at the start of the row
						Mtemp[i * b] = Mtemp[i * b + j];
					}
		    	}
		    }
		    MPI_Comm_split(comm, (rank/q), rank, &row_comm);
		    // MPI_Comm_rank(row_comm, &row_rank);
		    // MPI_Comm_size(row_comm, &row_size);
		    // printf("WORLD RANK/SIZE: %d/%d \t ROW RANK/SIZE: %d/%d\n",rank, size, row_rank, row_size);
		    Q = Dummy;//resetting Matrix Q for storing the ALLreduce from M
		    for (int j = 0; j<b; j++)
			{	
				max = Mtemp[j*b];
				// printf("max %d", max);
				MPI_Allreduce(&max, &Q[j*b], 1, MPI_INT, MPI_MAX, row_comm);//storing resultant value in matrix Q
			}
			for (int i = 0; i<b; i++)//replicating Q along row-wise
		    {
		    	for (int j = 0; j<b; j++)
		    	{
		    		Q[i * b + j] = Q[i*b];
		    	}
		    }
		    MPI_Comm_free(&row_comm);
		    //Final Step
		    //Giving P some hope in attaining convergence
		    Ptemp = P; //storing past value of P
		    for (int i = 0; i<b; i++)//
		    {
		    	for (int j = 0; j<b; j++)
		    	{
		    		if(Q[i * b + j] >= PPrime[i * b + j])//taking max value
		    			P[i * b + j] = Q[i*b];
		    		else
		    			P[i * b + j] = PPrime[i*b];
		    	}
		    }
		    // checking for any change in matrix	    
		    if (P == Ptemp){
		  		// printf("Success");
		  		success_checker++;
		  		if(success_checker==size){
		  			// printf("Success");
		  			break;
		  		}
		    }
		  	// else
		  		// printf("Repeat again");
		  	count++;//update loop count
		  	if(count == 300)//the loopbreaker
		  		break;
		  }//tree hanging , see tree_hanging==0 to not implement tree hanging
		  else 
		  	break;
	}//end of while loop, 
		sleep(rank);
	    // printf("Col %d",col*b);
	    // Printing the Parent Matrix
	    std::cout << "Parent Matrix P (" << row << "," << col << ")" << std::endl;
	    for (int i = 0; i < b; ++i) {
	        for (int j = 0; j < b; ++j) std::cout << static_cast<int>(P[i * b + j]) << " ";
	        std::cout << std::endl;
	    }
    return -1;
} // connected_components

#endif // A1_HPP
