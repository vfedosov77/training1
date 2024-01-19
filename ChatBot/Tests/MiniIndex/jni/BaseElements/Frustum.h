#ifndef __FRUSTUM__
#define __FRUSTUM__

#include "Primitives2D.h"
#include "Primitives3D.h"
#include "Matrix.h"

class Frustum {
	int m_nFocalLength;
	int m_nWidth;
	int m_nHeight;
	int m_nHalfWidth;
	int m_nHalfHeight;

	Vector3D m_centre;

	Vector3D m_defaultNormal;

public:
	Frustum() : m_nFocalLength(0) { }

	Frustum(int _nFocalLength, int _dWidth, int _dHeight) : m_nFocalLength(_nFocalLength), m_nWidth(_dWidth), m_nHeight(_dHeight),
		m_nHalfWidth(m_nWidth/2), m_nHalfHeight(m_nHeight/2), m_centre(0, 0, 0), m_defaultNormal(0, -1, 0)
	{

	}

	int getFocalLength() const {
		return m_nFocalLength;
	}

	int getWidth() const {
		return m_nWidth;
	}

	int getHeight() const {
		return m_nHeight;
	}

	bool canBeOnPlane(const Vector2D &_point, const Plane &_earth) const {
		return _earth.getIntersection(m_centre, fromScreenPoint(_point)).z > 0;
	}

	Vector2D toScreen(const Vector3D &_v) const {
		assert(m_nFocalLength != 0);
		const float d = float(m_nFocalLength)/float(_v.z);
		return Vector2D(fast_floor(_v.x*d + m_nHalfWidth), fast_floor(_v.y*d + m_nHalfHeight));
	}

	Vector2DF toScreenF(const Vector3D &_v) const {
		assert(m_nFocalLength != 0);
		const float d = float(m_nFocalLength)/float(_v.z);
		return Vector2DF(_v.x*d + m_nHalfWidth, _v.y*d + m_nHalfHeight);
	}

	Vector2D toScreen(const Vector3D &_v, const Vector3D &_earthNormal) const {
		assert(m_nFocalLength != 0);
		Vector3D v = rotate(_earthNormal, _v);
		const float d = float(m_nFocalLength)/float(_v.z);
		return Vector2D(fast_floor(v.x*d + m_nHalfWidth), fast_floor(v.y*d + m_nHalfHeight));
	}

	Vector2DF toScreenF(const Vector2DF &_v, const PlaneWith2DCoordinates &_plane) const {
		Vector3D v = _plane.get3DCoord(_v);
		assert(m_nFocalLength != 0);
		const float d = float(m_nFocalLength)/float(v.z);
		return Vector2DF(v.x*d + m_nHalfWidth, v.y*d + m_nHalfHeight);
	}

	inline
	Vector3D fromScreenPoint(const Vector2D &_v) const {
		assert(m_nFocalLength != 0);
		return Vector3D(_v.x - m_nHalfWidth, _v.y - m_nHalfHeight, m_nFocalLength);
	}

	inline
	Vector3D fromScreenPoint(const Vector2DF &_v) const {
		assert(m_nFocalLength != 0);
		return Vector3D(_v.x - m_nHalfWidth, _v.y - m_nHalfHeight, m_nFocalLength);
	}

	inline
	Vector3D toPlane(const Vector2D &_screenPoint, const Plane &_plane) const {
		assert(m_nFocalLength != 0);
		Vector3D intersection = _plane.getIntersection(m_centre, fromScreenPoint(_screenPoint));
		return intersection;
	}

	inline
	Vector3D toPlane(const Vector2DF &_screenPoint, const Plane &_plane) const {
		assert(m_nFocalLength != 0);
		Vector3D intersection = _plane.getIntersection(m_centre, fromScreenPoint(_screenPoint));
		return intersection;
	}

	inline
	Vector2DF toPlane(const Vector2D &_screenPoint, const PlaneWith2DCoordinates &_plane) const {
		assert(m_nFocalLength != 0);
		Vector3D intersection = _plane.m_plane.getIntersection(m_centre, fromScreenPoint(_screenPoint));
		return _plane.get2DCoord(intersection);
	}

	inline
	Edge3D toPlane(const Edge &_edge, const Plane &_plane) const {
		assert(m_nFocalLength != 0);
		return Edge3D(toPlane(_edge.v1, _plane), toPlane(_edge.v2, _plane));
	}

	Edge getHorizont(const Vector3D &_earthNormal) const {
		const float fDivider = _earthNormal.y;
		const float y1 = (-(float(_earthNormal.x)*- m_nHalfWidth) - (float(_earthNormal.z)*m_nFocalLength))/fDivider;
		const float y2 = (-(float(_earthNormal.x)*m_nHalfWidth) - (float(_earthNormal.z)*m_nFocalLength))/fDivider;
		return Edge(Vector2D(0, int(y1 + m_nHalfHeight)), Vector2D(m_nWidth, int(y2 + m_nHalfHeight)));
	}

	Vector3D rotate(const Vector3D &_earthNorm, const Vector3D &_toRotate) const {
		//TODO: Optimize this.
		float alpha = ::acos(_earthNorm*m_defaultNormal);
		Quaternion q(m_defaultNormal.crossProduct(_earthNorm), alpha);
		return q*_toRotate;
	}

	int getScreenZ() const {
		return m_nFocalLength;
	}

	Vector2D getScreenCentre() const {
		return Vector2D(m_nHalfWidth, m_nHalfHeight);
	}

	Vector2DF getOrtDirrection(const Vector2D &_centre, const Vector2DF &_dir, const Plane &_plane) const {
		if (!canBeOnPlane(_centre, _plane))
			return Vector2DF();

		const Vector3D planeCentre = toPlane(_centre, _plane);
		const Vector3D planeGradEnd = toPlane(_dir + _centre, _plane);
		const Vector3D gradDir = planeGradEnd - planeCentre;
		Vector3D ortDir = gradDir.crossProduct(_plane.normal);
		ortDir.normalize();
		ortDir *= gradDir.length();
		const Vector2DF ortEnd = toScreenF(planeCentre + ortDir);
		return ortEnd - Vector2DF(_centre);
	}
};

enum FrustumPlanes {
	fpTop = 0,
	fpRigth = 1,
	fpBottom = 2,
	fpLeft = 3,
	fpCount
};

class OrientedFrustum {
	int m_nFocalLength;
	int m_nWidth;
	int m_nHeight;
	int m_nHalfWidth;
	int m_nHalfHeight;

	Vector3D m_position;
	Vector3D m_direction;
	Vector3D m_topDir;
	Vector3D m_rigthDir;

	Matrix3D m_matrix;

	Plane m_planes[fpCount];
public:
	OrientedFrustum() : m_position(Vector3D::ms_notValidVector) { }

	OrientedFrustum(int _nFocalLength, int _nWidth, int _nHeight, const Vector3D &_position, const Vector3D &_direction, const Vector3D &_topDir) :
		m_nFocalLength(_nFocalLength), m_nWidth(_nWidth), m_nHeight(_nHeight), m_nHalfWidth(m_nWidth/2), m_nHalfHeight(m_nHeight/2),
		m_position(_position), m_direction(_direction), m_topDir(_topDir)
	{
		if (m_direction.length2() != 1.0f)
			m_direction.normalize();
		if (m_topDir.length2() != 1.0f)
			m_topDir.normalize();

		m_rigthDir = m_direction.crossProduct(m_topDir);

		assert(std::abs(m_direction.dotProduct(m_topDir)) < 0.01);

		const Vector3D screenCentrePoint = m_direction*_nFocalLength;

		const Vector3D topCentrePoint = screenCentrePoint + m_topDir*(float(_nHeight)/2);
		m_planes[fpTop] = Plane(m_position, m_rigthDir.crossProduct(topCentrePoint.getNormalized()));

		const Vector3D bottomCentrePoint = screenCentrePoint - m_topDir*(float(_nHeight)/2);
		m_planes[fpBottom] = Plane(m_position, bottomCentrePoint.getNormalized().crossProduct(m_rigthDir));

		const Vector3D rigthCentrePoint = screenCentrePoint + m_rigthDir*(float(_nWidth)/2);
		m_planes[fpRigth] = Plane(m_position, rigthCentrePoint.getNormalized().crossProduct(m_topDir));

		const Vector3D leftCentrePoint = screenCentrePoint - m_rigthDir*(float(_nWidth)/2);
		m_planes[fpLeft] = Plane(m_position, m_topDir.crossProduct(leftCentrePoint.getNormalized()));

		fillMatrixByBasis(m_rigthDir, -m_topDir, m_direction, m_matrix);
	}

	const Vector3D &getPosition() const {
		return m_position;
	}

	Vector3D toLocalCoordinates(const Vector3D &_vec) const {
		return mulMatrixT(_vec - m_position, m_matrix);
	}

	Vector3D fromLocalCoordinates(const Vector3D &_vec) const {
		return mulMatrix(_vec, m_matrix) + m_position;
	}

	bool isOnScreen(const Vector3D &_v) const {
		const Vector3D point = _v - m_position;
		for (size_t c = 0; c < fpCount; ++c)
			if (m_planes[c].normal.dotProduct(point) > 0)
				return false;

		return true;
	}

	Vector2D toScreen(const Vector3D &_v) const {
		const Vector3D localPoint = mulMatrixT(_v - m_position, m_matrix);
		if (localPoint.z <= 0)
			return Vector2D::ms_notValidVector;

		const float d = float(m_nFocalLength)/float(localPoint.z);
		return Vector2D(fast_floor(localPoint.x*d + m_nHalfWidth), fast_floor(localPoint.y*d + m_nHalfHeight));
	}

	Vector2DF toScreenF(const Vector3D &_v) const {
		const Vector3D localPoint = mulMatrixT(_v - m_position, m_matrix);
		if (localPoint.z <= 0)
			return Vector2DF::ms_notValidVector;

		const float d = float(m_nFocalLength)/float(localPoint.z);
		return Vector2DF(localPoint.x*d + m_nHalfWidth, localPoint.y*d + m_nHalfHeight);
	}

	inline
	Vector3D fromScreenPoint(const Vector2D &_v) const {
		const Vector3D local(_v.x - m_nHalfWidth, _v.y - m_nHalfHeight, m_nFocalLength);
		return mulMatrix(local, m_matrix);
	}

	inline
	Vector3D fromScreenPoint(const Vector2DF &_v) const {
		const Vector3D local(_v.x - m_nHalfWidth, _v.y - m_nHalfHeight, m_nFocalLength);
		return mulMatrix(local, m_matrix);
	}

};

#endif //__FRUSTUM__
