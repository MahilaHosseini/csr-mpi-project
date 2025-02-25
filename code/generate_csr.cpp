#include <iostream>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <algorithm>

using namespace std;

struct CSRMatrix {
    int rows, cols;
    vector<int> values;
    vector<int> col_indices;
    vector<int> row_pointers;
};

void generateCSRMatrix(int rows, int cols, double sparsity, CSRMatrix &matrix) {
    srand(time(nullptr));
    matrix.rows = rows;
    matrix.cols = cols;
    matrix.row_pointers.push_back(0);
    
    for (int i = 0; i < rows; ++i) {
        int non_zero_count = (cols * sparsity) + 0.5; // Approximate non-zero elements per row
        vector<int> used_cols;
        
        for (int j = 0; j < non_zero_count; ++j) {
            int col;
            do {
                col = rand() % cols;
            } while (std::find(used_cols.begin(), used_cols.end(), col) != used_cols.end());
            used_cols.push_back(col);
            
            matrix.values.push_back(rand() % 20 + 1); // Random natural number 1-20
            matrix.col_indices.push_back(col);
        }
        std::sort(used_cols.begin(), used_cols.end());
        matrix.row_pointers.push_back(matrix.values.size());
    }
}

void saveCSRToFile(const string &filename, const CSRMatrix &matrix) {
    ofstream file(filename);
    if (!file) {
        cerr << "Error opening file: " << filename << endl;
        exit(EXIT_FAILURE);
    }
    
    file << "Values: ";
    for (int v : matrix.values) file << v << " ";
    file << "\nColumn_Indices: ";
    for (int c : matrix.col_indices) file << c << " ";
    file << "\nRow_Pointers: ";
    for (int r : matrix.row_pointers) file << r << " ";
    file << endl;
    
    file.close();
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " rows cols" << endl;
        return EXIT_FAILURE;
    }
    
    int rows = stoi(argv[1]);
    int cols = stoi(argv[2]);
    double sparsity = 0.2; // 20% sparsity
    
    CSRMatrix matrix;
    generateCSRMatrix(rows, cols, sparsity, matrix);
    
    string filename = to_string(rows) + "_" + to_string(cols) + "_csr.txt";
    saveCSRToFile(filename, matrix);
    
    cout << "CSR matrix saved to " << filename << endl;
    
    return 0;
}

