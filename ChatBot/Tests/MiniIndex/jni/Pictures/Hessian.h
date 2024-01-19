#ifndef _HESSIAN_
#define _HESSIAN_

#include <vector>
#include <list>

#include "BaseElements/Colors.h"
#include "Application.h"
#include "KeyPoint.h"
#include "LevelData.h"
#include "Picture.h"
#include "Dynamic/DynamicGrid.h"

typedef DynamicGrid<g_nTrashold/2> DynamicGridWithThreshold;

class HessianTransform {
	const IntegralTransform &m_integral;

	std::vector<LevelData> m_levels;
	std::list<KeyPoint> m_pointsForLinesDetection;
	std::list<KeyPoint> m_keypoints;
	KeyPointsGrid m_grid;
	Rect2D m_rect;
	bool m_bUseDynamicOnly;

	bool _compareWithLevel(const LevelData &_level, Vector2D _point, int _nCurVal, int _nShift, int (&_n9matrix)[9]);
	Vector2D _calcGradInPoint(const Vector2D &_point, const Vector2D &_centrePoint, int _nLevelId, const Plane &_plane, const Frustum &_frustum);
	size_t _calculateArrayAlongOrt(const KeyPoint3D &_point, const Vector2D &_ort, int *pArrayX) const;

	static
	int _interpolateKeypoint(int (&N9)[3][9], int _nDx, int _nDy, int _nDs, KeyPoint& _kpt);

	mutable DynamicGridWithThreshold m_dynamicGrid;

	template<size_t cRowSize>
	struct ThreeRowsData;

public:
	HessianTransform(const Picture &_picture, Rect2D _rect = Application::getScreenRect());

	void findKeyPoints();

	const KeyPointsGrid &getGrid() const {
		return m_grid;
	}

	DynamicGridWithThreshold &getDynamicGrid() {
		if (!m_dynamicGrid.isInitialized())
			m_dynamicGrid.initialize();
		return m_dynamicGrid;
	}

	const std::list<KeyPoint> &getKeypoints() {
		assert(!m_bUseDynamicOnly);
		m_grid.collectPoints(m_keypoints);
		return m_keypoints;
	}

	inline
	void calculateKeypoint3D(KeyPoint &_keypoint, const Picture &_picture, KeyPoint3D &_result, const Vector2DF &_prevDir = Vector2DF::ms_notValidVector) {
		_result = KeyPoint3D(_keypoint, _picture.getEarthPlane(), _picture.getFrustum(), &_picture);
		_picture.calcDescriptor(_result, m_levels[_keypoint.getHessianArrayLevelId()], _prevDir);
	}

	void calculateKeypoints3D(std::list<KeyPoint> &_keypoints, std::list<KeyPoint3D> &_result, const Picture &_picture) {
		for (auto iPoint = _keypoints.begin(); iPoint != _keypoints.end(); ++iPoint) {
			_result.push_back(KeyPoint3D(*iPoint, _picture.getEarthPlane(), _picture.getFrustum(), &_picture));
			_picture.calcDescriptor(_result.back(), m_levels[iPoint->getHessianArrayLevelId()]);
		}
	}

	void calculateKeypoints3D(std::list<KeyPoint3D> &_points, const Picture &_picture) {
		getKeypoints();
		calculateKeypoints3D(m_keypoints, _points, _picture);
	}

	const std::list<KeyPoint> &getKeypointsForLineDetection() {
		return m_pointsForLinesDetection;
	}

	const LevelData &getLevelData(size_t _cLevel) const {
		return m_levels[_cLevel];
	}

	static void test();

	const IntegralTransform &getIntegral() const {
		return m_integral;
	}
};

template<size_t cRowSize>
struct HessianTransform::ThreeRowsData {
	int m_nWidth, m_nHeight;
	int m_nCurRowIdx;
	int rows[3*cRowSize];
	int *m_pEnd;
	int *m_pRow1, *m_pRow2, *m_pRow3;
	int *m_pCurrent, *m_pRowBegin, *m_pRowEnd;
	bool m_bReadyToCalculate;
	int m_nDeltaToMiddle;
	int m_nDeltaToPrev;

	ThreeRowsData(Rect2D &_searchRect) : m_nWidth(_searchRect.width()), m_nHeight(_searchRect.height()), m_nCurRowIdx(0), m_pEnd(rows + 3*cRowSize),
		m_pRow1(rows), m_pRow2(rows + cRowSize), m_pRow3(rows + cRowSize*2), m_pCurrent(m_pRow1), m_pRowBegin(m_pRow1), m_pRowEnd(m_pRow1 + m_nWidth),
		m_bReadyToCalculate(false), m_nDeltaToMiddle(cRowSize + 1), m_nDeltaToPrev(2*m_nDeltaToMiddle)
	{

	}

	void nextRow() {
		if (m_nCurRowIdx > 1) {
			int *pTemp = m_pRow1;
			m_pRow1 = m_pRow2;
			m_pRow2 = m_pRow3;
			m_pCurrent = m_pRowBegin = m_pRow3 = pTemp;
			m_nDeltaToMiddle = m_pCurrent - m_pRow2 + 1;
			m_nDeltaToPrev = m_pCurrent - m_pRow1 + 2;
		} else if (m_nCurRowIdx == 0)
			m_pCurrent = m_pRowBegin = m_pRow2;
		else {
			m_pCurrent = m_pRowBegin = m_pRow3;
			m_bReadyToCalculate	 = true;
		}

		m_pRowEnd = m_pRowBegin + m_nWidth;
		++m_nCurRowIdx;
		assert(m_nCurRowIdx < m_nHeight);
	}

	void pushValue(int _nValue) {
		assert(m_pCurrent < m_pRowBegin + m_nWidth);
		*m_pCurrent = _nValue;
		++m_pCurrent;
	}

	void skip(size_t _cCount) {
		m_pCurrent += _cCount;
		assert(m_pCurrent < m_pRowBegin + m_nWidth);
	}

	bool needValue() const {
		return m_pCurrent < m_pRowEnd;
	}

	bool needRow() const {
		return m_nCurRowIdx < m_nHeight - 1;
	}

	bool readyToCalculate() const {
		return m_bReadyToCalculate && m_pCurrent > m_pRowBegin + 2;
	}

	int *current() {
		int *pRes =  m_pCurrent - 1 - m_nDeltaToMiddle;
		assert(pRes >= rows && pRes < m_pEnd);
		return pRes;
	}

	int *nextRowBegin() {
		int *pRes =  m_pCurrent - 3;
		assert(pRes >= rows && pRes < m_pEnd);
		return pRes;
	}

	int *prevRowBegin() {
		int *pRes =  m_pCurrent - 1 - m_nDeltaToPrev;
		assert(pRes >= rows && pRes < m_pEnd);
		return pRes;
	}
};


int getLevelIdBySize(int _nSize);

#endif//_HESSIAN_
