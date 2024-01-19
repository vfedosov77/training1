#ifndef _MATRIX_H_
#define _MATRIX_H_

#include "Primitives3D.h"

typedef float Matrix3D[9];

inline
Vector3D mulMatrix(const Vector3D &_vec, const Matrix3D _matrix) {
	const Vector3D result(_vec.x*_matrix[0] + _vec.y*_matrix[3] + _vec.z*_matrix[6],
		_vec.x*_matrix[1] + _vec.y*_matrix[4] + _vec.z*_matrix[7],
		_vec.x*_matrix[2] + _vec.y*_matrix[5] + _vec.z*_matrix[8]
	);
	return result;
}

inline
Vector3D mulMatrixT(const Vector3D &_vec, const Matrix3D _matrix) {
	const Vector3D result(_vec.x*_matrix[0] + _vec.y*_matrix[1] + _vec.z*_matrix[2],
		_vec.x*_matrix[3] + _vec.y*_matrix[4] + _vec.z*_matrix[5],
		_vec.x*_matrix[6] + _vec.y*_matrix[7] + _vec.z*_matrix[8]
	);
	return result;
}

//Result will translate by mulMatrix from new system to old. By mulMatrixT this matrix will translate from old system to new.
inline
void fillMatrixByBasis(const Vector3D &_x, const Vector3D &_y, const Vector3D &_z, Matrix3D _matrix) {
	_matrix[0] = _x.x;
	_matrix[1] = _x.y;
	_matrix[2] = _x.z;
	_matrix[3] = _y.x;
	_matrix[4] = _y.y;
	_matrix[5] = _y.z;
	_matrix[6] = _z.x;
	_matrix[7] = _z.y;
	_matrix[8] = _z.z;
}

#endif //_MATRIX_H_
