#ifndef _PICTURE_H__
#define _PICTURE_H__

#include <fstream>
#include <iostream>
#include <stdexcept>

#include "BaseElements/Frustum.h"
#include "IntegralTransform.h"
#include "KeyPoint.h"
#include "LevelData.h"

struct PicturePosition {
	static PicturePosition defaultPosition;

	Vector3D position;
	Vector3D direction;
	Vector3D topDirection;
};

class Picture : public Counted {
	static size_t ms_cId;

	byte *m_pBuf;
	bool m_bDeleteBuf;
	IntegralTransform m_integral;
	Vector3D m_verticalDirection;
	Plane m_earthPlane;
	Frustum m_frustum;
	OrientedFrustum m_orientedFrustum;
	size_t m_cId;

	Picture(const Picture &/*_other*/);
public:

	Picture(const std::string &_strFileName) : m_bDeleteBuf(true) {
		m_cId = ms_cId++;

		std::ifstream stream(_strFileName.data());
		stream.seekg(0, std::ios_base::end);
		const size_t cSize = stream.tellg();
		stream.seekg(0, std::ios_base::beg);
		m_pBuf = new byte[cSize];
		stream.read((char *)m_pBuf, cSize);
		stream.close();
		Accelerometer accelerometer(m_pBuf);
		m_verticalDirection = accelerometer.verticalDirection();
		m_earthPlane = accelerometer.getEarth();
		IntegralTransform transform(m_pBuf);
		m_integral.swap(transform);
		m_frustum = Frustum(Application::getFocalLength(m_pBuf), g_nWidth, g_nHeight);
	}

	Picture() : m_pBuf(nullptr), m_bDeleteBuf(false), m_cId(size_t(-1)) { }

	Picture(byte *_pBuf) :
		m_pBuf(_pBuf),
		m_bDeleteBuf(false),
		m_integral(_pBuf),
		m_frustum(Application::getFocalLength(_pBuf), g_nWidth, g_nHeight)
	{
		m_cId = ms_cId++;

		Accelerometer accelerometer(_pBuf);
		m_verticalDirection = accelerometer.verticalDirection();
		m_earthPlane = accelerometer.getEarth();
	}

	~Picture() {
		if (m_bDeleteBuf)
			delete[] m_pBuf;
	}

	void swap(Picture &_other) {
		m_integral.swap(_other.m_integral);
		std::swap(m_pBuf, _other.m_pBuf);
		std::swap(m_bDeleteBuf, _other.m_bDeleteBuf);
		std::swap(m_verticalDirection, _other.m_verticalDirection);
		std::swap(m_earthPlane, _other.m_earthPlane);
		std::swap(m_frustum, _other.m_frustum);
		std::swap(m_orientedFrustum, _other.m_orientedFrustum);
		std::swap(m_cId, _other.m_cId);
	}

	void setOrientedFrustum(const OrientedFrustum &_frustum) {
		m_orientedFrustum = _frustum;
	}

	const OrientedFrustum &getOrientedFrustum() const {
		assert(isValid());
		assert(!m_orientedFrustum.getPosition().isNotValid());
		return m_orientedFrustum;
	}

	const byte *getBuf() const {
		assert(isValid());
		return m_pBuf;
	}

	const Vector3D &getVerticalDirection() const {
		assert(isValid());
		return m_verticalDirection;
	}

	const Plane &getEarthPlane() const {
		assert(isValid());
		return m_earthPlane;
	}

	const Frustum &getFrustum() const {
		return m_frustum;
	}

	const IntegralTransform &getIntegral() const {
		assert(isValid());
		return m_integral;
	}

	size_t getId() const {
		return m_cId;
	}

	void fillLineGradients(const Edge &_line, float *pArray, size_t _cSize) const;
	bool fillGradient(const KeyPoint3D &_kp, Gradient &_grad, const Vector2D &_direction, const LevelData &_data) const;
	void calcDescriptor(KeyPoint3D &_kp, const LevelData &_data, const Vector2DF &_prevDir = Vector2DF::ms_notValidVector) const;

	bool isValid() const {
		return m_cId != size_t(-1);
	}
};

Vector2D calcGradient(const Vector2D &_point, int _nLevelId, const IntegralTransform &_integral, int _nWidth, int _nHeigth,
	const Vector2DF &_prevDirection
);
void calcGradients(const Vector2D &_point, int _nLevelId, const IntegralTransform &_integral, int _nWidth, int _nHeigth, std::vector<Vector2D> &_bestSumms);
void calculateArrayAlongGrad(int _nKeyPointSize, const Vector2D &_point, GradientArrayMinMax &_gradient, const IntegralTransform &_integral);
size_t fillLineGradientsByBresenham(const Edge &_line, int *_pArray, size_t _cHaarSize, const IntegralTransform &_integral);

#endif //_PICTURE_H__
