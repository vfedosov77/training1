#ifndef _KEY_POINTS_GRID_
#define _KEY_POINTS_GRID_

#include "Gradient.h"

struct KeyPoint {
	KeyPoint() { }

	Vector2D pointInLevel;
	Vector2D point;
	int nValue;
	int nSize;
	int nLevelId;
	bool bHessianSign;//sign(Dxx + Dyy) - determines the lightness of the spot.

	int getHessianArrayLevelId() const {
		return nLevelId;// != 0 ? nLevelId : 1;
	}
};

struct KeyPointStatistic {
	KeyPointStatistic() : cSuccessDetections(0) { }

	size_t cSuccessDetections;
};

struct KeyPoint3D {
	KeyPoint3D() { }

	KeyPoint3D(const KeyPoint &_p, const Plane &_plane, const Frustum &_frustum, const class Picture *_pPicture) : nLastMatchLevelId(-1), pPicture(_pPicture) {
		point = _frustum.toPlane(_p.point, _plane);
		nValue = _p.nValue;
		nSize = _p.nSize;
		screenPoint = _p.point;
		bHessianSign = _p.bHessianSign;
		nLevelId = _p.nLevelId;

		static int nCurId = 0;
		nId = nCurId++;
	}

	int getGradVecSize() const {
		return nSize/2 + 4;
	}

	Vector2D getGradVec(Vector2D _grad) const {
		_grad.normalize();
		_grad *= getGradVecSize();
		_grad /= g_nNormalizedVectorLength;
		return _grad;
	}

	Vector2DF getGradVecF(Vector2D _grad) const {
		Vector2DF grad(_grad);
		grad.normalize();
		grad *= getGradVecSize();
		return grad;
	}

	Vector2DF getGradVecF(Vector2DF _grad) const {
		_grad.normalize();
		_grad *= getGradVecSize();
		return _grad;
	}

	Vector2D getGradEnd(Vector2D _grad) const {
		Vector2D grad = getGradVec(_grad);
		grad.x += screenPoint.x;
		grad.y += screenPoint.y;
		return grad;
	}

	int getHessianArrayLevelId() const {
		return nLevelId;// != 0 ? nLevelId : 1;
	}

	Vector2D screenPoint;
	Vector3D point;
	float fDistance;
	int nDistance2;
	int nValue;
	int nSize;
	int nLevelId;
	mutable int nLastMatchLevelId;
	bool bHessianSign;
	int nId;

	const class Picture *pPicture;

	std::list<Gradient> gradientDirection;

	mutable Vector2D tagPoint;
	mutable KeyPointStatistic statistic;
};

struct GridCell {
	GridCell() : nThreshold(g_nTrashold) { }

	std::list<KeyPoint> points;
	int nThreshold;
	Rect2D rect;

	KeyPoint &addKeyPoint(const Vector2D &_point, int _nValue) {
		assert(rect.contains(_point));
		points.push_back(KeyPoint());
		KeyPoint &kp = points.back();
		kp.point = _point;
		kp.nValue = _nValue;

		assert(_nValue > nThreshold);

		_nValue = _nValue>>2;
		if (_nValue > nThreshold)
			nThreshold = _nValue;

		return kp;
	}
};

class KeyPointsGrid {
	//32x32 pix in cell.
	static const int ms_nWidth = g_nWidth/32;
	static const int ms_nHeight = g_nHeight/32;
	static const int ms_nSize = ms_nWidth*ms_nHeight;

	GridCell m_cells[ms_nSize];

	static inline
	int _getIdByPoint(int _nX, int _nY) {
		_nX >>= 5;
		_nY >>= 5;
		assert(_nX >= 0 && _nX < ms_nWidth && _nY >= 0 && _nY < ms_nHeight);
		return _nX + _nY*ms_nWidth;
	}

public:
	KeyPointsGrid() {
		for (int i = 0; i < ms_nWidth; ++i)
			for (int j = 0; j < ms_nHeight; ++j)
				m_cells[i + j*ms_nWidth].rect = Rect2D(i*32, j*32, i*32 + 32, j*32 + 32);
	}

	void collectPoints(std::list<KeyPoint> &_list) {
		for (GridCell &cell : m_cells)
			_list.splice(_list.end(), cell.points);
	}

	GridCell &getCellByPoint(int _nX, int _nY) {
		return m_cells[_getIdByPoint(_nX, _nY)];
	}

	int getCellAndNeihbours(int _nCellX, int _nCellY, GridCell *(&_cells)[9]);
	void removeMinorPointsAndCollectPointsForLines(std::list<KeyPoint> &_pointsForLinesDetection);
	void getEdgePoints(const Edge &_edge, bool _bNearest, const Plane &_plane, std::list<KeyPoint3D> &_result, const Frustum &_frustum, const class Picture *_pPicture) const;
};

#endif//_KEY_POINTS_GRID_
