#ifndef __PRIMITIVES__
#define __PRIMITIVES__

#include <limits>
#include <cmath>
#include <unordered_set>
#include <unordered_map>
#include <iomanip>
#include <sstream>
#include <inttypes.h>
#include <assert.h>

#include "Types.h"

#ifdef NOT_ANDROID
#include <opencv2/opencv.hpp>
#endif

const float g_PI  = 3.14159265358979;
constexpr int g_nNormalizedVectorLength = 1024;
const int g_nIntMax = 0x7FFFFFFF;
const int g_nIntMin = -0x80000000;

struct Vector2D {
	static const Vector2D ms_notValidVector;

	int x, y;

	Vector2D() : x(0), y(0) {}
	Vector2D(int _x, int _y) : x(_x), y(_y) {}

#ifdef NOT_ANDROID
	operator cv::Point() {
		return cv::Point(x, y);
	}

	cv::Point toCV() const {
		return cv::Point(x, y);
	}

#endif

	const Vector2D & operator *=(int _d) {
		assert(!isNotValid());
		x = x*_d;
		y = y*_d;
		return (*this);
	}

	const Vector2D & operator /=(int _d) {
		assert(!isNotValid());
		x = x/_d;
		y = y/_d;
		return (*this);
	}

	const Vector2D & operator *=(float _d) {
		assert(!isNotValid());
		x = fast_floor(x*_d + 0.5);
		y = fast_floor(y*_d + 0.5);
		return (*this);
	}

	const Vector2D & operator /=(float _d) {
		assert(!isNotValid());
		x = fast_floor(x/_d + 0.5);
		y = fast_floor(y/_d + 0.5);
		return (*this);
	}

	Vector2D operator *(int _d) const {
		assert(!isNotValid());
		return Vector2D(*this) *= _d;
	}

	Vector2D operator /(int _d) const {
		assert(!isNotValid());
		return Vector2D(*this) /= _d;
	}

	Vector2D operator *(float _d) const {
		assert(!isNotValid());
		return Vector2D(*this) *= _d;
	}

	Vector2D operator /(float _d) const {
		assert(!isNotValid());
		return Vector2D(*this) /= _d;
	}

	bool operator ==(const Vector2D &_other) const {
		return x == _other.x && y == _other.y;
	}

	bool operator !=(const Vector2D &_other) const {
		return !((*this) == _other);
	}

	bool operator <(const Vector2D &_other) const {
		assert(!isNotValid());
		if (x != _other.x)
			return x < _other.x;

		return y < _other.y;
	}

	Vector2D operator -() const {
		assert(!isNotValid());
		return Vector2D(-x, -y);
	}

	Vector2D operator -(const Vector2D &_other) const {
		assert(!isNotValid());
		return Vector2D(x - _other.x, y - _other.y);
	}

	Vector2D operator +(const Vector2D &_other) const {
		assert(!isNotValid());
		return Vector2D(x + _other.x, y + _other.y);
	}

	void operator -=(const Vector2D &_other) {
		assert(!isNotValid());
		x -= _other.x;
		y -= _other.y;
	}

	void operator +=(const Vector2D &_other) {
		assert(!isNotValid());
		x += _other.x;
		y += _other.y;
	}

	int64_t operator *(const Vector2D &_other) const {
		return dotProduct(_other);
	}

	int64_t dotProduct(const Vector2D &_other) const {
		assert(!isNotValid());
		return int64_t(x)*_other.x + int64_t(y)*_other.y;
	}

	int dotProduct32Bits(const Vector2D &_other) const {
		assert(std::abs(int64_t(x)*_other.x) < std::numeric_limits<int>::max() && std::abs(int64_t(y)*_other.y) < std::numeric_limits<int>::max());
		assert(!isNotValid());
		return x*_other.x + y*_other.y;
	}

	int64_t crossProduct(const Vector2D & _other) const {
		assert(!isNotValid());
		return int64_t(x)*_other.y - int64_t(y)*_other.x;
	}

	int64_t distance2(const Vector2D & _b) const {
		return (_b - (*this)).length2();
	}

	float distance(const Vector2D & _b) const {
		assert(!isNotValid());
		return (_b - (*this)).length();
	}

	int64_t length2() const {
		assert(!isNotValid());
		return int64_t(x)*x + int64_t(y)*y;
	}

	float length() const {
		assert(!isNotValid());
		return ::sqrt((float)length2());
	}

	void normalize() {
		assert(!isNotValid());
		const float fLen = length();
		assert(fLen != 0.0);
		if (fLen == 0.0)
			return;

		x = float(x)*g_nNormalizedVectorLength/fLen;
		y = float(y)*g_nNormalizedVectorLength/fLen;
	}

	Vector2D getNormalized() const {
		Vector2D newVec = *this;
		newVec.normalize();
		return newVec;
	}

	Vector2D multNormalized(float _fMult) const {
		return Vector2D(x*_fMult/g_nNormalizedVectorLength, y*_fMult/g_nNormalizedVectorLength);
	}

	bool isNotValid() const {
		return *this == ms_notValidVector;
	}

	bool empty() const {
		return x == 0 && y == 0;
	}

	// Tells if vec c lies on the left side of directed edge a->b
	// 1 if left, -1 if right, 0 if colinear
	int isOnLeftSide(const Vector2D &_v) const {
		const int64_t x = crossProduct(_v);
		return x < 0 ? -1 : (x > 0 ? 1 : 0);
	}
};

struct Vector2DF {
	static const Vector2DF ms_notValidVector;

	float x, y;

	Vector2DF() : x(0), y(0) {}
	Vector2DF(const Vector2D &_vec) : x(_vec.x), y(_vec.y) { }
	Vector2DF(float _x, float _y) : x(_x), y(_y) {}

#ifdef NOT_ANDROID
	operator cv::Point() {
		return cv::Point(x, y);
	}

	cv::Point toCV() const {
		return cv::Point(x + 0.5, y + 0.5);
	}

#endif

	Vector2D toInt() const {
		return Vector2D(fast_floor(x + 0.5), fast_floor(y + 0.5));
	}

	const Vector2DF & operator *=(float _d) {
		x = x*_d;
		y = y*_d;
		return (*this);
	}

	const Vector2DF & operator /=(float _d) {
		x = x/_d;
		y = y/_d;
		return (*this);
	}

	Vector2DF operator *(float _d) const {
		return Vector2DF(*this) *= _d;
	}

	Vector2DF operator /(float _d) const {
		return Vector2DF(*this) /= _d;
	}

	bool operator ==(const Vector2DF &_other) const {
		return x == _other.x && y == _other.y;
	}

	bool operator !=(const Vector2DF &_other) const {
		return !((*this) == _other);
	}

	bool operator <(const Vector2DF &_other) const {
		if (x != _other.x)
			return x < _other.x;

		return y < _other.y;
	}

	Vector2DF operator -() const {
		return Vector2DF(-x, -y);
	}

	Vector2DF operator -(const Vector2DF &_other) const {
		return Vector2DF(x - _other.x, y - _other.y);
	}

	Vector2DF operator -(const Vector2D &_other) const {
		return Vector2DF(x - _other.x, y - _other.y);
	}

	Vector2DF operator +(const Vector2DF &_other) const {
		return Vector2DF(x + _other.x, y + _other.y);
	}

	Vector2DF operator +(const Vector2D &_other) const {
		return Vector2DF(x + _other.x, y + _other.y);
	}

	void operator -=(const Vector2DF &_other) {
		x -= _other.x;
		y -= _other.y;
	}

	void operator -=(const Vector2D &_other) {
		x -= _other.x;
		y -= _other.y;
	}

	void operator +=(const Vector2DF &_other) {
		x += _other.x;
		y += _other.y;
	}

	void operator +=(const Vector2D &_other) {
		x += _other.x;
		y += _other.y;
	}

	float operator *(const Vector2DF &_other) const {
		return dotProduct(_other);
	}

	float dotProduct(const Vector2DF &_other) const {
		return x*_other.x + y*_other.y;
	}

	float crossProduct(const Vector2DF & _other) const {
		return x*_other.y - y*_other.x;
	}

	int64_t distance2(const Vector2DF & _b) const {
		return (_b - (*this)).length2();
	}

	float distance(const Vector2DF & _b) const {
		return (_b - (*this)).length();
	}

	float length2() const {
		return x*x + y*y;
	}

	float length() const {
		return ::sqrt(length2());
	}

	void normalize() {
		const float fLen = length();
		assert(fLen != 0.0);
		if (fLen == 0.0)
			return;

		x = x/fLen;
		y = y/fLen;
	}

	Vector2DF getNormalized() const {
		Vector2DF newVec = *this;
		newVec.normalize();
		return newVec;
	}

	// Tells if vec c lies on the left side of directed edge a->b
	// 1 if left, -1 if right, 0 if colinear
	int isOnLeftSide(const Vector2DF &_v) const {
		const float x = crossProduct(_v);
		return x < 0 ? -1 : (x > 0 ? 1 : 0);
	}

	bool empty() const {
		return x == 0 && y == 0;
	}

	bool isNotValid() const {
		return *this == ms_notValidVector;
	}
};

//Test the itersection of line (a) and segment (b). Does not detect collinear lines intersection.
//WARNING!!! This function does not use Epsilon with optimisation goals and the problems with float precision rounding are possible.
inline
bool isLineAndSegmentIntersected(const Vector2D &_a0, const Vector2D &_a1, const Vector2D &_b0, const Vector2D &_b1, Vector2D &_res) {
	const Vector2D dx = _a1 - _a0;
	const Vector2D dy = _b1 - _b0;
	const Vector2D d = _a0 - _b0;

	float dCross = float(dy.x)*dx.y - float(dy.y)*dx.x;
	if (dCross == 0)
		return false;

	dCross = (float(d.x)*dx.y - float(d.y)*dx.x)/dCross;

	if (dCross <= 0 || dCross >= 1)
		return false;

	_res.x = _b0.x + dCross*dy.x;
	_res.y = _b0.y + dCross*dy.y;
	return true;
}

inline
bool isLinesIntersected(const Vector2D &_a0, const Vector2D &_a1, const Vector2D &_b0, const Vector2D &_b1, Vector2D &_res) {
	const Vector2D dx = _a1 - _a0;
	const Vector2D dy = _b1 - _b0;
	const Vector2D d = _a0 - _b0;

	float dCross = float(dy.x)*dx.y - float(dy.y)*dx.x;
	if (dCross == 0)
		return false;

	dCross = (float(d.x)*dx.y - float(d.y)*dx.x)/dCross;

	_res.x = _b0.x + dCross*dy.x;
	_res.y = _b0.y + dCross*dy.y;
	return true;
}

//Test the itersection of line (a) and segment (b). Does not detect collinear lines intersection.
//WARNING!!! This function does not use Epsilon with optimisation goals and the problems with float precision rounding are possible.
inline
bool isLineAndSegmentIntersected(const Vector2D &_a0, const Vector2D &_a1, const Vector2D &_b0, const Vector2D &_b1) {
	const Vector2D dx = _a1 - _a0;
	const Vector2D dy = _b1 - _b0;
	const Vector2D d = _a0 - _b0;

	float dCross = float(dy.x)*dx.y - float(dy.y)*dx.x;
	if (dCross == 0)
		return false;

	dCross = (float(d.x)*dx.y - float(d.y)*dx.x)/dCross;

	if (dCross <= 0 || dCross >= 1)
		return false;

	return true;
}

inline
float _distance2PointToEdge(const Vector2D &_begin, const Vector2D &_end, const Vector2D &_point) {
	if (_begin == _end)
		return (_point - _begin).length2();

	const Vector2D edgeVector =_end - _begin;
	Vector2D newVector = _point - _begin;
	const int64_t llDot = edgeVector.dotProduct(newVector);

	if (llDot <= 0)
		return newVector.length2();

	const int64_t llEdgeLength2 = edgeVector.length2();
	if (llEdgeLength2 <= llDot)
		return (_point - _end).length2();

	float dVecToProjX = newVector.x - float(edgeVector.x)*llDot/llEdgeLength2;
	float dVecToProjY = newVector.y - float(edgeVector.y)*llDot/llEdgeLength2;

	return dVecToProjX*dVecToProjX + dVecToProjY*dVecToProjY;
}

inline
float _distance2PointToLine(const Vector2D &_begin, const Vector2D &_end, const Vector2D &_point) {
	assert(_begin != _end);
	const Vector2D edgeVector = _end - _begin;
	const Vector2D newVector = _point - _begin;
	const float fY = float(-newVector.x)*edgeVector.y + float(newVector.y)*edgeVector.x;
	return fY*fY/edgeVector.length2();
}

inline
int64_t getTriArea2(const Vector2D &_a, const Vector2D &_b, const Vector2D &_c) {
	return std::abs(_a.x*(_c.y - _b.y) + _b.x*(_a.y - _c.y) + _c.x*(_b.y - _a.y));
}

inline
bool isPointInsideTriangle(const Vector2D _polygon[4], size_t _c1, size_t _c2, size_t _c3, const Vector2D &_point) {
	const int64_t dTriArea = getTriArea2(_polygon[_c1], _polygon[_c2], _polygon[_c3]);
	const int64_t dSubArea1 = getTriArea2(_point, _polygon[_c2], _polygon[_c3]);
	const int64_t dSubArea2 = getTriArea2(_polygon[_c1], _point, _polygon[_c3]);
	const int64_t dSubArea3 = getTriArea2(_polygon[_c1], _polygon[_c2], _point);
	return dTriArea - (dSubArea1 + dSubArea2 + dSubArea3) == 0;
}

inline
bool isPointInside(const Vector2D _polygon[4], const Vector2D &_point, size_t _cSeparatePoint) {
	if (_cSeparatePoint == 2)
		return isPointInsideTriangle(_polygon, 0, 1, 2, _point) || isPointInsideTriangle(_polygon, 0, 2, 3, _point);
	else {
		assert(_cSeparatePoint == 3);
		return isPointInsideTriangle(_polygon, 0, 3, 1, _point) || isPointInsideTriangle(_polygon, 3, 2, 1, _point);
	}
}

struct Edge {
	static const Edge ms_emptyEdge;

	Vector2D v1, v2;

	Edge() { }
	Edge(const Vector2D &_v1, const Vector2D &_v2) : v1(_v1), v2(_v2) {}

	bool operator ==(const Edge &_other) const {
		return (v1 == _other.v1 && v2 == _other.v2) || (v2 == _other.v1 && v1 == _other.v2);
	}

	bool operator !=(const Edge &_other) const {
		return !((*this) == _other);
	}

	bool operator <(const Edge &_other) const {
		if (v1 < v2) {
			if (_other.v1 < _other.v2) {
				if (v1 != _other.v1)
					return v1 < _other.v1;

				return v2 < _other.v2;
			} else {
				if (v1 != _other.v2)
					return v1 < _other.v2;

				return v2 < _other.v1;
			}
		} else {
			if (_other.v1 < _other.v2) {
				if (v2 != _other.v1)
					return v2 < _other.v1;

				return v1 < _other.v2;
			} else {
				if (v2 != _other.v2)
					return v2 < _other.v2;

				return v1 < _other.v1;
			}
		}
	}

	inline
	Vector2D getVector() const {
		return v2 - v1;
	}

	inline
	Vector2D getVectorNorm() const {
		assert(!isEmpty());
		return (v2 - v1).getNormalized();
	}

	float length() const {
		return getVector().length();
	}

	int64_t length2() const {
		return getVector().length2();
	}

	bool isEmpty() const {
		return v1 == v2;
	}

	Vector2D &getPointRef(const Vector2D &_p) {
		assert(_p == v1 || _p == v2);
		return _p == v1 ? v1 : v2;
	}

	const Vector2D &getOpposite(const Vector2D &_p) const {
		assert(_p == v1 || _p == v2);
		return _p == v1 ? v2 : v1;
	}

	inline
	bool contains(const Vector2D &_v) const {
		return v1 == _v || v2 == _v;
	}

	inline
	bool hasCommonVertex(const Edge &_other) const {
		return contains(_other.v1) || contains(_other.v2);
	}

	inline
	const Vector2D &getCommonVertex(const Edge &_other) const {
		assert(hasCommonVertex(_other));
		return contains(_other.v1) ? _other.v1 : (contains(_other.v2) ? _other.v2 : Vector2D::ms_notValidVector);
	}

	inline
	const size_t getCommonVertexIndex(const Edge &_other) const {
		assert(hasCommonVertex(_other));
		return contains(_other.v1) ? 0 : 1;
	}

	inline
	void reorderVertices() {
		std::swap(v1, v2);
	}

	// Tells if vec c lies on the left side of directed edge a->b
	// 1 if left, -1 if right, 0 if colinear
	inline
	int isPointOnLeftLineSide(const Vector2D &_v) const {
		const int64_t x = getVector().crossProduct(_v - v1);
		return x < 0 ? -1 : (x > 0 ? 1 : 0);
	}

	inline
	Vector2D projection(const Vector2D &_v, bool &_bInside) const {
		assert(!isEmpty());

		if (v1 == v2) {
			_bInside = false;
			return Vector2D();
		}

		const Vector2D edgeVec = getVector();
		const Vector2D vec2 = _v - v1;
		const int64_t llProduct = vec2.dotProduct(edgeVec);
		const int64_t llEdgeVecLen2 = edgeVec.length2();
		const Vector2D toProj(edgeVec.x*llProduct/llEdgeVecLen2, edgeVec.y*llProduct/llEdgeVecLen2);

		if (toProj.x == 0 && toProj.y == 0) {
			_bInside = true;
			return v1;
		}

		const int64_t llLen2 = toProj.length2();
		_bInside = llLen2 <= llEdgeVecLen2 && (llProduct >= 0);
		return v1 + toProj;
	}

	inline
	Vector2D projectionInside(const Vector2D &_v) const {
		bool bInside;
		const Vector2D p = projection(_v, bInside);

		if (bInside)
			return p;

		return (_v - v1).length2() <= (_v - v2).length2() ? v1 : v2;
	}

//	inline
//	bool isIntersectNotInCommonVertex(const Edge &_other) const {
//		if (hasCommonVertex(_other)) {
//			return false;
//		}

//		return _isLineIntersectedSegment(v1, v2, _other.v1, _other.v2) && _isLineIntersectedSegment(_other.v1, _other.v2, v1, v2);
//	}

//	inline
//	bool isIntersect(const Edge &_other) const {
//		if (hasCommonVertex(_other)) {
//			return true;
//		}

//		return _isLineIntersectedSegment(v1, v2, _other.v1, _other.v2) && _isLineIntersectedSegment(_other.v1, _other.v2, v1, v2);
//	}

	inline
	bool isIntersect(const Edge &_other, Vector2D &_point) const {
		if (hasCommonVertex(_other)) {
			_point = getCommonVertex(_other);
			return true;
		}

		return isLineAndSegmentIntersected(v1, v2, _other.v1, _other.v2, _point) && isLineAndSegmentIntersected(_other.v1, _other.v2, v1, v2, _point);
	}

	inline
	bool isIntersectLines(const Edge &_other, Vector2D &_point) const {
		return isLinesIntersected(v1, v2, _other.v1, _other.v2, _point);
	}

	inline
	bool isLineIntersectsSegment(const Edge &_other, Vector2D &_point) const {
		if (hasCommonVertex(_other)) {
			_point = getCommonVertex(_other);
			return true;
		}

		return isLineAndSegmentIntersected(v1, v2, _other.v1, _other.v2, _point);
	}

	inline
	float distance2(const Vector2D &_v) const {
		assert(!isEmpty());
		return _distance2PointToEdge(v1, v2, _v);
	}

	float distance2FromLine(const Vector2D &_v) const {
		return _distance2PointToLine(v1, v2, _v);
	}

	float distance2(const Edge &_other) const;

	float dotProduct(const Edge &_other) const {
		return getVector().dotProduct(_other.getVector());
	}

	float dotProductNormalized(const Edge &_other) const {
		return getVector().getNormalized().dotProduct(_other.getVector().getNormalized());
	}
};

struct Rect2D {
	int nX0, nY0, nX1, nY1;

	Rect2D() : nX0(g_nIntMax), nY0(g_nIntMax), nX1(g_nIntMin),
		nY1(g_nIntMin)
	{

	}

	Rect2D(int _nX0, int _nY0, int _nX1, int _nY1) : nX0(_nX0), nY0(_nY0), nX1(_nX1), nY1(_nY1) { }

	Rect2D(const Vector2D &_centre, int _nDistX, int _nDistY) : nX0(_centre.x - _nDistX), nY0(_centre.y - _nDistY),
		nX1(_centre.x + _nDistX), nY1(_centre.y + _nDistY)
	{ }

	bool empty() const {
		return nX0 >= nX1 && nY0 >= nY1;
	}

	void clipBy(const Rect2D &_other) {
		nX0 = std::max(_other.nX0, nX0);
		nX1 = std::min(_other.nX1, nX1);
		nY0 = std::max(_other.nY0, nY0);
		nY1 = std::min(_other.nY1, nY1);
	}

	int width() const {
		if (nX0 > nX1)
			return 0;

		return nX1 - nX0;
	}

	int height() const {
		if (nY0 > nY1)
			return 0;

		return nY1 - nY0;
	}

	Vector2D topLeft() const {
		return Vector2D(nX0, nY0);
	}

	Vector2D bottomRigth() const {
		return Vector2D(nX1, nY1);
	}

	Vector2D topRigth() const {
		return Vector2D(nX1, nY0);
	}

	Vector2D bottomLeft() const {
		return Vector2D(nX0, nY1);
	}

	void extend(const Vector2D &_point) {
		if (_point.x < nX0)
			nX0 = _point.x;
		if (_point.x > nX1)
			nX1 = _point.x;
		if (_point.y < nY0)
			nY0 = _point.y;
		if (_point.y > nY1)
			nY1 = _point.y;
	}

	void extend(const Rect2D &_rect) {
		if (_rect.nX0 < nX0)
			nX0 = _rect.nX0;
		if (_rect.nX1 > nX1)
			nX1 = _rect.nX1;
		if (_rect.nY0 < nY0)
			nY0 = _rect.nY0;
		if (_rect.nY1 > nY1)
			nY1 = _rect.nY1;
	}

	void extend(float _dX, float _dY) {
		nX0 -= _dX;
		nX1 += _dX;
		nY0 -= _dY;
		nY1 += _dY;
	}

	bool contains(const Vector2D &_point) const {
		return _point.x >= nX0 && _point.x < nX1 && _point.y >= nY0 && _point.y < nY1;
	}

	bool containsInside(const Vector2D &_point) const {
		return _point.x > nX0 && _point.x < nX1 - 1 && _point.y > nY0 && _point.y < nY1 - 1;
	}

	bool contains(const Rect2D &_rect) const {
		return nX0 <= _rect.nX0 && nX1 >= _rect.nX1 && nY0 <= _rect.nY0 && nY1 >= _rect.nY1;
	}

	bool intersects(const Rect2D &_rect) const {
		return nX0 <= _rect.nX1 && nX1 >= _rect.nX0 && nY0 <= _rect.nY1 && nY1 >= _rect.nY0;
	}

	bool intersects(const Edge &_e) const {
		if (contains(_e.v1) || contains(_e.v2))
			return true;

		if (_e.v1.x < nX0 && _e.v2.x < nX0)
			return false;

		if (_e.v1.x > nX1 && _e.v2.x > nX1)
			return false;

		if (_e.v1.y < nY0 && _e.v2.y < nY0)
			return false;

		if (!_e.v1.y > nY1 && !_e.v2.y > nY1)
			return false;

		Vector2D temp;
		if (!_isIntersectVertEdge(_e, nX0, nY0, nY1, temp) && !_isIntersectVertEdge(_e, nX1, nY0, nY1, temp) && !_isIntersectHorEdge(_e, nY0, nX0, nX1, temp))
			return false;

		return true;
	}

	void inflate(int _nDelta) {
		nX0 += _nDelta;
		nY0 += _nDelta;
		nX1 -= _nDelta;
		nY1 -= _nDelta;
	}

	Edge clipEdge(const Edge &_e) const {
		const bool bContainFirst = containsInside(_e.v1);
		const bool bContainSecond = containsInside(_e.v2);

		if (bContainFirst && bContainSecond)
			return _e;

		if (_e.v1.x < nX0 && _e.v2.x < nX0)
			return Edge::ms_emptyEdge;

		if (_e.v1.x > nX1 && _e.v2.x > nX1)
			return Edge::ms_emptyEdge;

		if (_e.v1.y < nY0 && _e.v2.y < nY0)
			return Edge::ms_emptyEdge;

		if (_e.v1.y > nY1 && _e.v2.y > nY1)
			return Edge::ms_emptyEdge;

		Vector2D points[2];
		size_t cIndex = 0;

		if (bContainFirst) {
			points[0] = _e.v1;
			++cIndex;
		} else if (bContainSecond) {
			points[0] = _e.v2;
			++cIndex;
		}

		if (_isIntersectVertEdge(_e, nX0, nY0, nY1, points[cIndex])) {
			if (cIndex == 1)
				return Edge(points[0], points[1]);
			++cIndex;
		}

		if (_isIntersectVertEdge(_e, nX1, nY0, nY1, points[cIndex])) {
			if (cIndex == 1)
				return Edge(points[0], points[1]);
			++cIndex;
		}

		if (_isIntersectHorEdge(_e, nY0, nX0, nX1, points[cIndex])) {
			if (cIndex == 1)
				return Edge(points[0], points[1]);
			++cIndex;
		}

		if (_isIntersectHorEdge(_e, nY1, nX0, nX1, points[cIndex])) {
			assert(cIndex == 1);
		}

		return Edge(points[0], points[1]);
	}

	Edge clipLine(const Edge &_e) const {
		int nCurPoint = 0;
		Edge result;
		const Vector2D &lineP1 = _e.v1;
		const Vector2D &lineP2 = _e.v2;

		if (_isIntersectHorLine(lineP1, lineP2, nY0, nX0, nX1, result.v1))
			nCurPoint++;

		if (_isIntersectVertLine(lineP1, lineP2, nX1, nY0, nY1, nCurPoint == 0 ? result.v1 : result.v2) &&
			(nCurPoint == 0 || result.v2 != result.v1)
		)
			nCurPoint++;

		if (nCurPoint < 2 && _isIntersectHorLine(lineP1, lineP2, nY1, nX0, nX1, nCurPoint == 0 ? result.v1 : result.v2) &&
			(nCurPoint == 0 || result.v2 != result.v1)
		)
			nCurPoint++;

		if (nCurPoint < 2 && _isIntersectVertLine(lineP1, lineP2, nX0, nY0, nY1, nCurPoint == 0 ? result.v1 : result.v2) &&
			(nCurPoint == 0 || result.v2 != result.v1)
		)
			nCurPoint++;

		assert(nCurPoint == 2);

		return result;
	}

private:
	static inline
	bool _isIntersectVertLine(const Vector2D &_lineP1, const Vector2D & _lineP2, int _nX, int _nYBegin, int _nYEnd, Vector2D &_point) {
		assert(_nYBegin < _nYEnd);

		if (_lineP1.x == _lineP2.x) {
			if (_nX == _lineP1.x) {
				_point.x = _nX;
				_point.y = _nYBegin;
				return true;
			}

			return false;
		}

		const int nY = _lineP1.y + (_lineP2.y - _lineP1.y)*(_nX - _lineP1.x)/(_lineP2.x - _lineP1.x);
		_point.x = _nX;
		_point.y = nY;
		return nY >= _nYBegin && nY <= _nYEnd;
	}

	static inline
	bool _isIntersectHorLine(const Vector2D &_lineP1, const Vector2D &_lineP2, double _nY, double _nXBegin, double _nXEnd, Vector2D &_point) {
		assert(_nXBegin < _nXEnd);

		if (_lineP1.y == _lineP2.y) {
			if (_nY == _lineP1.y) {
				_point.x = _nXBegin;
				_point.y = _nY;
				return true;
			}

			return false;
		}

		const int nX = _lineP1.x + (_lineP2.x - _lineP1.x)*(_nY - _lineP1.y)/(_lineP2.y - _lineP1.y);
		_point.x = nX;
		_point.y = _nY;
		return nX >= _nXBegin && nX <= _nXEnd;
	}

	bool _isIntersectVertEdge(const Edge &_e, int _dX, int _dYBegin, int _dYEnd, Vector2D &_point) const {
		assert(_dYBegin < _dYEnd);

		if (std::min(_e.v1.x, _e.v2.x) > _dX || std::max(_e.v1.x, _e.v2.x) < _dX)
			return false;

		if (_e.v1.x == _e.v2.x)
			return !(std::max(_e.v1.y, _e.v2.y) < _dYBegin || std::min(_e.v1.y, _e.v2.y) > _dYEnd);

		const float dY = _e.v1.y + (_e.v2.y - _e.v1.y)*(_dX - _e.v1.x)/(_e.v2.x - _e.v1.x);
		_point.x = _dX;
		_point.y = dY;
		return dY >= _dYBegin && dY <= _dYEnd;
	}

	bool _isIntersectHorEdge(const Edge &_e, int _dY, int _dXBegin, int _dXEnd, Vector2D &_point) const {
		assert(_dXBegin < _dXEnd);
		if (std::min(_e.v1.y, _e.v2.y) > _dY || std::max(_e.v1.y, _e.v2.y) < _dY)
			return false;

		if (_e.v1.y == _e.v2.y)
			return !(std::max(_e.v1.x, _e.v2.x) < _dXBegin || std::min(_e.v1.x, _e.v2.x) > _dXEnd);

		const float dX = _e.v1.x + (_e.v2.x - _e.v1.x)*(_dY - _e.v1.y)/(_e.v2.y - _e.v1.y);
		_point.x = dX;
		_point.y = _dY;
		return dX >= _dXBegin && dX <= _dXEnd;
	}
};

template <class Vector>
struct AngleComparableBase {
	float fTanF;
	bool bXLessThanZero;

	AngleComparableBase() {

	}

	AngleComparableBase(const Vector &_vec) {
		setDirection(_vec);
	}

	void setDirection(const Vector &_vec) {
		fTanF = _vec.x == 0 ? std::numeric_limits<float>::max()*(_vec.y >= 0 ? 1 : -1) : float(_vec.y)/float(_vec.x);
		bXLessThanZero = _vec.x < 0;
	}

	bool operator<(const AngleComparableBase &_other) const {
		if (bXLessThanZero != _other.bXLessThanZero)
			return !bXLessThanZero;

		return fTanF < _other.fTanF;
	}
};

//Function rotates the point around the coordinates center on a corner that is the corner between the vector _by and the X axe.
inline
Vector2D rotateByNormalizedVector(Vector2D _toRotate, Vector2D _byNormilized) {
	assert(std::abs(int64_t(_byNormilized.x)*_toRotate.x) < std::numeric_limits<int>::max() && std::abs(int64_t(_byNormilized.y)*_toRotate.y) < std::numeric_limits<int>::max());
	assert(std::abs(int64_t(_byNormilized.x)*_toRotate.y) < std::numeric_limits<int>::max() && std::abs(int64_t(_byNormilized.y)*_toRotate.x) < std::numeric_limits<int>::max());
	Vector2D res(_toRotate.x*_byNormilized.x + _toRotate.y*_byNormilized.y, -_toRotate.x*_byNormilized.y + _toRotate.y*_byNormilized.x);
	res /= g_nNormalizedVectorLength;
	return res;
}

inline
Vector2D rotateByVector(Vector2D _toRotate, Vector2D _by) {
	assert(std::abs(int64_t(_toRotate.x)*_by.x) < std::numeric_limits<int>::max() && std::abs(int64_t(_toRotate.y)*_by.y) < std::numeric_limits<int>::max());
	assert(std::abs(int64_t(_toRotate.x)*_by.y) < std::numeric_limits<int>::max() && std::abs(int64_t(_toRotate.y)*_by.x) < std::numeric_limits<int>::max());
	Vector2D res(_toRotate.x*_by.x + _toRotate.y*_by.y, -_toRotate.x*_by.y + _toRotate.y*_by.x);
	return res;
}

inline
Vector2DF rotateByVector(Vector2DF _toRotate, Vector2DF _by) {
	Vector2DF res(_toRotate.x*_by.x + _toRotate.y*_by.y, -_toRotate.x*_by.y + _toRotate.y*_by.x);
	return res;
}

#endif //__PRIMITIVES__
