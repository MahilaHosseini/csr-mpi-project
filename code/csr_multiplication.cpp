#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>

using namespace std;

struct CSRMatrix {
    int rows, cols;
    vector<int> values;
    vector<int> col_indices;
    vector<int> row_pointers;
};

void readCSRMatrix(const string &filename, CSRMatrix &matrix) {
    ifstream file(filename);
    if (!file) {
        cerr << "Error opening file: " << filename << endl;
        exit(EXIT_FAILURE);
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
    
    if (matrix.row_pointers.empty()) {
        cerr << "Error: Row pointers missing in " << filename << endl;
        exit(EXIT_FAILURE);
    }
    
    if (matrix.row_pointers.size() != (matrix.rows + 1)) {
        cerr << "Warning: Row pointer size mismatch in " << filename << ". Expected " << (matrix.rows + 1) << " but got " << matrix.row_pointers.size() << endl;
    }
}

CSRMatrix multiplyCSR(const CSRMatrix &A, const CSRMatrix &B) {
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

void printCSRMatrix(const CSRMatrix &matrix) {
    cout << "Values: ";
    for (int v : matrix.values) cout << v << " ";
    cout << "\nColumn Indices: ";
    for (int c : matrix.col_indices) cout << c << " ";
    cout << "\nRow Pointers: ";
    for (int r : matrix.row_pointers) cout << r << " ";
    cout << endl;
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        cerr << "Usage: " << argv[0] << " A_rows A_cols B_cols sparse_matrix_A.txt sparse_matrix_B.txt" << endl;
        return EXIT_FAILURE;
    }
    
    int A_rows = stoi(argv[1]);
    int A_cols = stoi(argv[2]);
    int B_cols = stoi(argv[3]);
    string file_A = argv[4];
    string file_B = argv[5];
    
    CSRMatrix A{A_rows, A_cols}, B{A_cols, B_cols};
    readCSRMatrix(file_A, A);
    readCSRMatrix(file_B, B);
    
    auto start = chrono::high_resolution_clock::now();
    CSRMatrix C = multiplyCSR(A, B);
    auto end = chrono::high_resolution_clock::now();
    
    chrono::duration<double> elapsed = end - start;
    cout << "Multiplication Time: " << elapsed.count() << " seconds\n";
    printCSRMatrix(A);
    printCSRMatrix(B);    
    printCSRMatrix(C);
    
    return 0;
}
