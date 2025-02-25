#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <mpi.h>
#include <chrono>

using namespace std;

struct CSRMatrix {
    int rows, cols;
    vector<int> values;
    vector<int> col_indices;
    vector<int> row_pointers;
};

// Function to read a CSR matrix from a file
void readMatrixFromFile(const string &filename, CSRMatrix &matrix) {
    ifstream file(filename);
    if (!file) {
        cerr << "Error opening file: " << filename << endl;
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string key;
        iss >> key;

        if (key == "Values:") {
            int val;
            while (iss >> val) matrix.values.push_back(val);
        } else if (key == "Column_Indices:") {
            int col;
            while (iss >> col) matrix.col_indices.push_back(col);
        } else if (key == "Row_Pointers:") {
            int row;
            while (iss >> row) matrix.row_pointers.push_back(row);
        }
    }
    file.close();
    
    
}

void printCSRMatrix(const CSRMatrix &matrix) {
    cout << "\nValues: ";
    for (int v : matrix.values) cout << v << " ";
    cout << "\nColumn Indices: ";
    for (int c : matrix.col_indices) cout << c << " ";
    cout << "\nRow Pointers: ";
    for (int r : matrix.row_pointers) cout << r << " ";
    cout << endl;
}

// Function to multiply sparse matrices in CSR format
CSRMatrix multiplySparseMatrices(const CSRMatrix &A, const CSRMatrix &B) {
    if (A.cols != B.rows) {
        cerr << "Matrix dimensions do not match for multiplication" << endl;
        exit(EXIT_FAILURE);
    }
    
    int C_rows = A.rows, C_cols = B.cols;
    vector<int> C_values;
    vector<int> C_col_indices;
    vector<int> C_row_pointers(C_rows + 1, 0);
    
    vector<vector<int>> temp(C_rows, vector<int>(C_cols, 0));
    
    for (int i = 0; i < A.rows; i++) {
        if (i + 1 >= A.row_pointers.size()) {
            cerr << "Error: Row pointer index out of bounds at row " << i << endl;
            exit(EXIT_FAILURE);
        }
        
        for (int j = A.row_pointers[i]; j < A.row_pointers[i + 1]; j++) {
            if (j >= A.col_indices.size()) {
                cerr << "Error: Column index out of bounds at row " << i << endl;
                exit(EXIT_FAILURE);
            }
            int A_col = A.col_indices[j];
            int A_val = A.values[j];
            
            if (A_col >= B.row_pointers.size() - 1) {
                cerr << "Error: Accessing invalid row in B at index " << A_col << endl;
                exit(EXIT_FAILURE);
            }
            
            for (int k = B.row_pointers[A_col]; k < B.row_pointers[A_col + 1]; k++) {
                if (k >= B.col_indices.size()) {
                    cerr << "Error: Column index out of bounds in B at row " << A_col << endl;
                    exit(EXIT_FAILURE);
                }
                int B_col = B.col_indices[k];
                int B_val = B.values[k];
                temp[i][B_col] += A_val * B_val;
            }
        }
    }
    
    for (int i = 0; i < C_rows; i++) {
        C_row_pointers[i] = C_values.size();
        for (int j = 0; j < C_cols; j++) {
            if (temp[i][j] != 0) {
                C_values.push_back(temp[i][j]);
                C_col_indices.push_back(j);
            }
        }
    }
    C_row_pointers[C_rows] = C_values.size();
    
    return {C_rows, C_cols, C_values, C_col_indices, C_row_pointers};
}


int main(int argc, char *argv[]) {

    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (argc != 6) {
        if (rank == 0) {
            cerr << "Usage: " << argv[0] << " A_rows A_cols B_cols sparse_matrix_A.txt sparse_matrix_B.txt" << endl;
        }
        MPI_Finalize();
        return 1;
    }

    string file_A = argv[4];
    string file_B = argv[5];
    
    // Matrix A and B
    CSRMatrix A, B;
    A.rows = atoi(argv[1]);
    A.cols = atoi(argv[2]);
    B.rows = atoi(argv[2]);
    B.cols = atoi(argv[3]);

    // Read matrices from files (process 0 only)
    if (rank == 0) {
        readMatrixFromFile(file_A, A);  // Matrix A
        readMatrixFromFile(file_B, B);  // Matrix B
        //cout << "inpput matrices";
        //printCSRMatrix(A);
        //printCSRMatrix(B);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Broadcast necessary data to all processes
    // Broadcast matrix A and B sizes first
    MPI_Bcast(&B.rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&B.cols, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&A.rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&A.cols, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    int B_values_size, B_col_indices_size, B_row_pointers_size;
    int A_values_size, A_col_indices_size, A_row_pointers_size;
    
    if (rank == 0) {
        B_values_size = B.values.size();
        B_col_indices_size = B.col_indices.size();
        B_row_pointers_size = B.row_pointers.size();
             
        A_values_size = A.values.size();
        A_col_indices_size = A.col_indices.size();
        A_row_pointers_size = A.row_pointers.size();
    }

    MPI_Bcast(&B_values_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&B_col_indices_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&B_row_pointers_size, 1, MPI_INT, 0, MPI_COMM_WORLD);


    MPI_Bcast(&A_values_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&A_col_indices_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&A_row_pointers_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if (rank != 0) {
        B.values.resize(B_values_size);
        B.col_indices.resize(B_col_indices_size);
        B.row_pointers.resize(B_row_pointers_size);
        
        
        A.values.resize(A_values_size);
        A.col_indices.resize(A_col_indices_size);
        A.row_pointers.resize(A_row_pointers_size);
    }

    MPI_Bcast(B.values.data(), B_values_size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(B.col_indices.data(), B_col_indices_size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(B.row_pointers.data(), B_row_pointers_size, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Bcast(A.values.data(), A_values_size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(A.col_indices.data(), A_col_indices_size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(A.row_pointers.data(), A_row_pointers_size, MPI_INT, 0, MPI_COMM_WORLD);    

    MPI_Barrier(MPI_COMM_WORLD); 
    

    // Start timer for multiplication part
    auto start_time = chrono::high_resolution_clock::now();
    
    // Perform local multiplication (each process works on part of the result)
    int rows_per_process = A.rows / size;
    int remainder = A.rows % size;
    int start_row = rank * rows_per_process + min(rank, remainder);
    int end_row = start_row + rows_per_process + (rank < remainder);
    
    CSRMatrix localA;
    CSRMatrix localC;
    localA.rows = end_row - start_row;
    localA.cols = A.cols;
    localA.row_pointers.push_back(0);
    
    if(localA.rows !=0){
	    for (int i = start_row; i < end_row; ++i) {
		int start = A.row_pointers[i];
		int end = A.row_pointers[i + 1];
		for (int j = start; j < end; ++j) {
		    localA.values.push_back(A.values[j]);
		    localA.col_indices.push_back(A.col_indices[j]);
		}
		localA.row_pointers.push_back(localA.values.size());
	    }
    
	    //cout << "\nrank " << rank <<  " start_row " << start_row;
	    //cout << "\nrank " << rank <<  " end_row " << end_row;
	    //cout << "\nrank " << rank <<  " Values: ";
	    //for (int v : localA.values) cout << v << " ";
	    //cout << "\nrank " << rank << " Column Indices: ";
	    //for (int c : localA.col_indices) cout << c << " ";
	    //cout << "\nrank " << rank << " Row Pointers: ";
	    //for (int r : localA.row_pointers) cout << r << " ";
	    
	    localC = multiplySparseMatrices(localA, B);
	    //printCSRMatrix(localC);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    // End timer for multiplication part
    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> diff = end_time - start_time;   
    

    if (rank != 0) {
        
	// Send the size of each vector (row_pointers, colIndices, values)
	int row_pointers_size = localC.row_pointers.size();
	int col_indices_size = localC.col_indices.size();
	int values_size = localC.values.size();

	// First, send the sizes to the root (rank 0)
	MPI_Send(&row_pointers_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
	MPI_Send(&col_indices_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
	MPI_Send(&values_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
	
	MPI_Send(&localC.row_pointers[0], localC.row_pointers.size(), MPI_INT, 0, 0, MPI_COMM_WORLD);
	MPI_Send(&localC.col_indices[0], localC.col_indices.size(), MPI_INT, 0, 0, MPI_COMM_WORLD);
	MPI_Send(&localC.values[0], localC.values.size(), MPI_INT, 0, 0, MPI_COMM_WORLD);

    }

    MPI_Barrier(MPI_COMM_WORLD);
    
    
    // Gather results on process 0
    if (rank == 0) {
    CSRMatrix finalC;
    // First, copy the result computed by the root process
    finalC = localC;
    finalC.rows = atoi(argv[1]);
    finalC.cols = atoi(argv[3]);
    
    // Now receive the results from the other processes
    for (int i = 1; i < size; i++) {
        int start_row = i * rows_per_process + min(i, remainder);
        int end_row = start_row + rows_per_process + (i < remainder);

        // Receive the sizes of the vectors
	int row_pointers_size, col_indices_size, values_size;
	MPI_Recv(&row_pointers_size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&col_indices_size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(&values_size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	// Allocate buffers based on the received sizes
	vector<int> row_pointers(row_pointers_size);
	vector<int> col_indices(col_indices_size);
	vector<int> values(values_size);

	MPI_Recv(row_pointers.data(), row_pointers_size, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(col_indices.data(), col_indices_size, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(values.data(), values_size, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        finalC.values.insert(finalC.values.end(), values.begin(), values.end()); // Append values
        finalC.col_indices.insert(finalC.col_indices.end(), col_indices.begin(), col_indices.end()); // Append column indices
       
        // Update rowPtrs for the final matrix (finalC)
        int offset = finalC.row_pointers.back();
        for (int j = 1; j < row_pointers.size(); j++) {
            finalC.row_pointers.push_back( row_pointers[j] + offset);
        }

    }
    //printCSRMatrix(finalC);

    }

    if (rank == 0) {
        cout << "\nMatrix multiplication took: " << diff.count() << " seconds" << endl;
    }

    MPI_Finalize();
    return 0;
}


