#ifndef __PRIMITIVES3D__
#define __PRIMITIVES3D__

#include <cmath>
#include <inttypes.h>
#include <assert.h>

#include "Primitives2D.h"

constexpr float g_fEpsilon = 1;
constexpr int g_nOneMeter = 10000;
constexpr int g_nOneMeter2 = g_nOneMeter*g_nOneMeter;

struct Vector3D {
	static const Vector3D ms_notValidVector;

	float x, y, z;

	Vector3D() : x(0), y(0), z(0) {}
	Vector3D(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
	explicit Vector3D(const Vector2D &_vec) : x(_vec.x), y(_vec.y), z(0) {}
	const Vector3D & operator *=(float _d) {
		assert(!isNotValid());
		x = x*_d;
		y = y*_d;
		z = z*_d;
		return (*this);
	}

	const Vector3D & operator /=(float _d) {
		assert(!isNotValid());
		x = x/_d;
		y = y/_d;
		z = z/_d;
		return (*this);
	}

	Vector3D operator *(float _d) const {
		assert(!isNotValid());
		return Vector3D(*this) *= _d;
	}

	Vector3D operator /(float _d) const {
		assert(!isNotValid());
		return Vector3D(*this) /= _d;
	}

	bool operator ==(const Vector3D &_other) const {
		return x == _other.x && y == _other.y && z == _other.z;
	}

	bool operator !=(const Vector3D &_other) const {
		return !((*this) == _other);
	}

	bool operator <(const Vector3D &_other) const {
		assert(!isNotValid());
		if (x != _other.x)
			return x < _other.x;

		if (y != _other.y)
			return y < _other.y;

		return z < _other.z;
	}

	Vector3D operator -(const Vector3D &_other) const {
		assert(!isNotValid());
		return Vector3D(x - _other.x, y - _other.y, z - _other.z);
	}

	Vector3D operator -() const {
		assert(!isNotValid());
		return Vector3D(-x, -y, -z);
	}

	Vector3D operator +(const Vector3D &_other) const {
		assert(!isNotValid());
		return Vector3D(x + _other.x, y + _other.y, z + _other.z);
	}

	void operator -=(const Vector3D &_other) {
		assert(!isNotValid());
		x -= _other.x;
		y -= _other.y;
		z -= _other.z;
	}

	void operator +=(const Vector3D &_other) {
		assert(!isNotValid());
		x += _other.x;
		y += _other.y;
		z += _other.z;
	}

	float operator *(const Vector3D &_other) const {
		return dotProduct(_other);
	}

	float dotProduct(const Vector3D &_other) const {
		assert(!isNotValid());
		return x*_other.x + y*_other.y + z*_other.z;
	}

	Vector3D crossProduct(const Vector3D &_other) const {
		assert(!isNotValid());
		return Vector3D(y*_other.z - z*_other.y, z*_other.x - x*_other.z, x*_other.y - y*_other.x);
	}

	float distance2(const Vector3D & _b) const {
		return (_b - (*this)).length2();
	}

	float distance(const Vector3D & _b) const {
		assert(!isNotValid());
		return (_b - (*this)).length();
	}

	float length2() const {
		assert(!isNotValid());
		return x*x + y*y + z*z;
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

		x = x/fLen;
		y = y/fLen;
		z = z/fLen;
	}

	Vector3D getNormalized() const {
		Vector3D newVec = *this;
		newVec.normalize();
		return newVec;
	}

	bool isEmpty() const {
		return x == 0 && y == 0 && z == 0;
	}

	bool isNotValid() const {
		return *this == ms_notValidVector;
	}

	Vector2D to2D() const {
		return Vector2D(fast_floor(x + 0.5), fast_floor(y + 0.5));
	}
};

struct Plane {
	static const int ms_nDefaultEarthHeight = g_nOneMeter;

	static
	Plane createEarthPlane(const Vector3D &_normal, float dHeight = ms_nDefaultEarthHeight) {
		return Plane(Vector3D() - _normal*dHeight, _normal);
	}

	Plane() { }

	Plane(const Vector3D &_point, const Vector3D &_normal) {
		normal = _normal;
		point = _point;
	}

	float distance(const Vector3D &_v) const {
		return float((_v - point)*normal);
	}

	Vector3D getProjection(const Vector3D &_v) const {
		return _v - normal*distance(_v);
	}

	Vector3D getIntersection(const Vector3D &_lineP1, const Vector3D &_lineP2) const {
		assert((_lineP2 - _lineP1).crossProduct(normal).length2() > 0);

		float dist1 = distance(_lineP1);
		float dist2 = distance(_lineP2);
		Vector3D proj1 = _lineP1 - normal*dist1;
		Vector3D proj2 = _lineP2 - normal*dist2;
		Vector3D intersection = proj1 + (proj2 - proj1)*(dist1/(dist1 - dist2));
		return intersection;
	}

	Vector3D normal;
	Vector3D point;
};

struct PlaneWith2DCoordinates {
	Plane m_plane;
	Vector3D m_planeDir;
	Vector3D m_planeOrt;

	PlaneWith2DCoordinates() { }

	PlaneWith2DCoordinates(const Plane &_plane, const Vector3D &_planeDirNorm) : m_plane(_plane), m_planeDir(_planeDirNorm),
		m_planeOrt(_plane.normal.crossProduct(_planeDirNorm))
	{
		assert(std::abs(m_plane.distance(m_plane.point + m_planeDir)) < g_fEpsilon);
	}

	Vector2DF get2DCoord(const Vector3D &_point) const {
		assert(std::abs(m_plane.distance(_point)) < g_fEpsilon);
		const Vector3D delta = _point - m_plane.point;
		const Vector2DF result(delta*m_planeDir, delta*m_planeOrt);
		return result;
	}

	Vector2DF get2DProjection(const Vector3D &_point) const {
		return get2DCoord(m_plane.getProjection(_point));
	}

	Vector3D get3DCoord(const Vector2DF &_point) const {
		return m_plane.point + m_planeDir*_point.x + m_planeOrt*_point.y;
	}

	Vector3D get3DDirection(const Vector2DF &_direction) const {
		return m_planeDir*_direction.x + m_planeOrt*_direction.y;
	}

	static
	PlaneWith2DCoordinates createDefault(const Plane &_plane) {
		Vector3D dir;
		if (_plane.normal.y > _plane.normal.z)
			dir = Vector3D(0, 0, 1);
		else
			dir = Vector3D(0, -1, 0);

		const Vector3D dirOnPlane = (_plane.getProjection(_plane.point + dir) - _plane.point).getNormalized();
		return PlaneWith2DCoordinates(_plane, dirOnPlane);
	}
};

struct Edge3D {
	Vector3D v1, v2;

	Edge3D() { }
	Edge3D(const Vector3D &_v1, const Vector3D &_v2) : v1(_v1), v2(_v2) {}

	bool operator ==(const Edge3D &_other) const {
		return (v1 == _other.v1 && v2 == _other.v2) || (v2 == _other.v1 && v1 == _other.v2);
	}

	bool operator !=(const Edge3D &_other) const {
		return !((*this) == _other);
	}

	bool operator <(const Edge3D &_other) const {
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
	Vector3D getVector() const {
		return v2 - v1;
	}

	inline
	Vector3D getVectorNorm() const {
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

	inline
	void reorderVertices() {
		std::swap(v1, v2);
	}

	inline
	Vector3D projection(const Vector3D &_v, bool &_bInside) const {
		if (v1 == v2) {
			_bInside = false;
			return Vector3D();
		}

		Vector3D edgeVec = getVector();
		Vector3D vec2 = _v - v1;
		const float fProduct = vec2.dotProduct(edgeVec);
		const float fEdgeVecLen2 = edgeVec.length2();
		const Vector3D toProj(edgeVec.x*fProduct/fEdgeVecLen2, edgeVec.y*fProduct/fEdgeVecLen2, edgeVec.z*fProduct/fEdgeVecLen2);
		float fLen2 = toProj.length2();
		_bInside = fLen2 >= 0 && fLen2 <= edgeVec.length2();
		return v1 + toProj;
	}

	inline
	Vector3D projectionInside(const Vector3D &_v) const {
		bool bInside;
		const Vector3D p = projection(_v, bInside);

		if (bInside)
			return p;

		return (_v - v1).length2() <= (_v - v2).length2() ? v1 : v2;
	}

	inline
	float distance2(const Vector3D &_v) const {
		const Vector3D proj = projectionInside(_v);
		return proj.distance2(_v);
	}

	inline
	float distance2ToLine(const Vector3D &_v) const {
		bool bInside;
		const Vector3D proj = projection(_v, bInside);
		return proj.distance2(_v);
	}

	float dotProduct(const Edge3D &_other) const {
		return getVector().dotProduct(_other.getVector());
	}
};

struct Quaternion {
	float x, y, z;
	float w;

	Quaternion() { }

	Quaternion(float latitude, float longitude, float angle) {
		float sin_a = ::sin(angle / 2);
		float cos_a = ::cos(angle / 2);

		float sin_lat = ::sin(latitude);
		float cos_lat = ::cos(latitude);

		float sin_long = ::sin(longitude);
		float cos_long = ::cos(longitude);

		x = sin_a * cos_lat * sin_long;
		y = sin_a * sin_lat;
		z = sin_a * sin_lat * cos_long;
		w = cos_a;
	}

	Quaternion(Vector3D v, float alpha) {
		v.normalize();
		v = v*float(::sin(alpha/2));
		x = v.x;
		y = v.y;
		z = v.z;
		w = ::cos(alpha / 2);
	}

	float lenght() const {
		return ::sqrt((float)(x*x + y*y + z*z + w*w));
	}

	void normalize() {
		float l = lenght();
		x /= l;
		y /= l;
		z /= l;
		w /= l;
	}

	Quaternion invert() const {
		Quaternion quat(*this);
		float length = 1.0f/lenght();
		quat.x *= -length;
		quat.y *= -length;
		quat.z *= -length;
		quat.w *= length;
		return quat;
	}

	static Quaternion QuaternionMultiply(Quaternion quat1, Quaternion quat2) {
		Quaternion cross;
		cross.x = quat1.y*quat2.z - quat1.z*quat2.y;
		cross.y = quat1.z*quat2.x - quat1.x*quat2.z;
		cross.z = quat1.x*quat2.y - quat1.y*quat2.x;

		Quaternion result;
		float angle = ((quat1.w * quat2.w) - (quat1.x*quat2.x + quat1.y*quat2.y + quat1.z*quat2.z));
		quat1.x *= quat2.w;
		quat1.y *= quat2.w;
		quat1.z *= quat2.w;
		quat2.x *= quat1.w;
		quat2.y *= quat1.w;
		quat2.z *= quat1.w;

		result.x = (quat1.x + quat2.x + cross.x);
		result.y = (quat1.y + quat2.y + cross.y);
		result.z = (quat1.z + quat2.z + cross.z);
		result.w = angle;

		return result;
	}

	Vector3D operator *(Vector3D vector) const {
		Quaternion vectorQuat;

		vectorQuat.x = vector.x;
		vectorQuat.y = vector.y;
		vectorQuat.z = vector.z;
		vectorQuat.w = 0.0;

		Quaternion inverseQuat = invert();
		Quaternion resultQuat = QuaternionMultiply(vectorQuat, inverseQuat);
		resultQuat = QuaternionMultiply(*this, resultQuat);

		Vector3D resultVector;
		resultVector.x = resultQuat.x;
		resultVector.y = resultQuat.y;
		resultVector.z = resultQuat.z;

		return resultVector;
	}
};

#endif //__PRIMITIVES3D__
