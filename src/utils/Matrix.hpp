#ifndef MATRIX_HPP
#define MATRIX_HPP

#include "utils.hpp"
#include "SparseMatrix.hpp"
#include <vector>

using namespace std;

#ifdef WEIGHTED
    #define VALUE_UNIT ushort
#else
    #define VALUE_UNIT bool
#endif
    
#ifdef SPARSE
    #define MATRIX_DATA_STRUCTURE SparseMatrix<VALUE_UNIT>
#else
    #define MATRIX_DATA_STRUCTURE vector<vector<VALUE_UNIT> >
#endif


class Matrix {
public:

    Matrix();
    Matrix(const Matrix & matrix);
    Matrix(uint numberOfNodes);

    Matrix & operator = (const Matrix & matrix);


    VALUE_UNIT get(uint node1, uint node2) const;
    void set(VALUE_UNIT value, uint node1, uint node2);

    uint size() const;

    void connect(VALUE_UNIT value, uint node1, uint node2);
    bool isConnected(uint node1, uint node2) const;

    template <class Archive>
    void serialize(Archive & archive) {
        archive(CEREAL_NVP(data));
    }

private:
   MATRIX_DATA_STRUCTURE data;
};

/*
 * The inline prefix is a must (mostly for get), otherwise it will 
 * fail the speed test. And these inline functions must be defined in 
 * the header files.
 */

inline VALUE_UNIT Matrix::get(uint node1, uint node2) const {
#ifdef SPARSE
    return data.get(node1, node2);
#else
    return data[node1][node2];
#endif
}

inline void Matrix::set(VALUE_UNIT value, uint node1, uint node2) {
#ifdef SPARSE
    if (data.get(node1, node2) || data.get(node2, node1)) {
        return ;
    }
    data.set(value, node1, node2);
#else
    if (data[node1][node2] || data[node2][node1]) {
        return ;
    }
    data[node1][node2] = value;
#endif
}

inline void Matrix::connect(VALUE_UNIT value, uint node1, uint node2) {
#ifdef SPARSE
    data.set(value, node1, node2);
    data.set(value, node2, node1);
#else
    data[node1][node2] = value;
    data[node2][node1] = value;
#endif       
}

inline bool Matrix::isConnected(uint node1, uint node2) const {
#ifdef SPARSE
    return data.get(node1, node2) && data.get(node2, node1);
#else
    return data[node1][node2] && data[node2][node1];
#endif
}
#endif
