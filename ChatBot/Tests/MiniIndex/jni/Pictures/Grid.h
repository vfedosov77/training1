#ifndef __GRID_KP_
#define __GRID_KP_

#include "BaseElements/Primitives2D.h"
#include "KeyPoint.h"

template<size_t _cStepShift>
class NearestPointsIterator {
protected:
	enum Move {
		mTopLeftToRigth = 0,
		mTopRigthToBottom = 1,
		mBottomRigthToLeft = 2,
		mBottomLeftToTop = 3
	};

	int m_nStep;
	int m_nMask;
	int m_nDistance;
	int m_nMaxDistance;
	Vector2D m_centre;
	Vector2D m_current;
	Vector2D m_nextCorner;

	bool m_bHorizontal;
	Move m_cCurMove;
	Rect2D m_rect;
	bool m_bStateChanged;
	bool m_bGap;
	bool m_bHasPoints;

	bool _nextCorner();
	bool _findNextInGrid();

	bool _beginDistance() {
		assert(m_nDistance%m_nStep == 0);
		if (m_nDistance > m_nMaxDistance)
			return false;

		m_bHorizontal  = true;
		m_current = m_centre + Vector2D(-m_nDistance, -m_nDistance);
		m_current.x += m_nStep;
		m_nextCorner = m_centre + Vector2D(m_nDistance, -m_nDistance);
		m_cCurMove = mTopLeftToRigth;
		m_bStateChanged = true;
		return _findNextInGrid();
	}

public:
	NearestPointsIterator() { }

	NearestPointsIterator(const Vector2D &_centre, int _nMaxDistance, const Rect2D &_rect) : m_nStep(1<<_cStepShift), m_nMask(m_nStep - 1), m_nDistance(0), m_nMaxDistance(_nMaxDistance),
		m_centre(_centre), m_current(Vector2D::ms_notValidVector), m_rect(_rect), m_bStateChanged(false), m_bHasPoints(true)
	{
		m_rect.nX0 += m_nStep - 1;
		m_rect.nX0 &= ~m_nMask;
		m_rect.nX1 += m_nStep - 1;
		m_rect.nX1 &= ~m_nMask;
		m_rect.nY0 += m_nStep - 1;
		m_rect.nY0 &= ~m_nMask;
		m_rect.nY1 += m_nStep - 1;
		m_rect.nY1 &= ~m_nMask;

		m_centre.x &= ~m_nMask;
		m_centre.y &= ~m_nMask;

		moveNext();
	}

	bool moveNext();

	const Vector2D &currentPoint() const {
		return m_current;

	}

	int getDistance() const {
		return m_nDistance;
	}

	bool eof() const {
		return !m_bHasPoints;
	}

	void resetStateChanged() {
		m_bStateChanged = false;
		m_bGap = false;
	}
};

template<size_t _cStepShift>
bool NearestPointsIterator<_cStepShift>::_nextCorner() {
	m_current = m_nextCorner;

	switch (m_cCurMove) {
	case mTopLeftToRigth:
		assert(m_nextCorner.y < m_centre.y);
		assert(m_bHorizontal && m_nextCorner == m_centre + Vector2D(m_nDistance, -m_nDistance));
		m_nextCorner = m_centre +  Vector2D(m_nDistance, m_nDistance);
		m_cCurMove = mTopRigthToBottom;
		m_bStateChanged = true;
		break;
	case mTopRigthToBottom:
		assert(!m_bHorizontal && m_nextCorner == m_centre +  Vector2D(+m_nDistance, m_nDistance));
		m_nextCorner = m_centre +  Vector2D(-m_nDistance, m_nDistance);
		m_cCurMove = mBottomRigthToLeft;
		m_bStateChanged = true;
		break;
	case mBottomRigthToLeft:
		assert(m_bHorizontal);
		assert(m_nextCorner == m_centre +  Vector2D(-m_nDistance, m_nDistance));
		m_nextCorner = m_centre +  Vector2D(-m_nDistance, -m_nDistance);
		m_cCurMove = mBottomLeftToTop;
		m_bStateChanged = true;
		break;
	case mBottomLeftToTop:
		assert(!m_bHorizontal);
		return false;
	}

	m_bHorizontal = !m_bHorizontal;
	return true;
}

template<size_t _cStepShift>
bool NearestPointsIterator<_cStepShift>::moveNext() {
	if (!m_bHasPoints)
		return false;

	if (m_nDistance == 0) {
		m_current = m_centre;
		m_nDistance += m_nStep;
		m_nextCorner = Vector2D::ms_notValidVector;
		return m_bHasPoints = m_rect.contains(m_current);
	}

	if (m_nDistance > m_nMaxDistance)
		return m_bHasPoints = false;

	if (m_nextCorner.isNotValid()) {
		return m_bHasPoints = _beginDistance();
	}

	if (m_nextCorner == m_current) {
		if (!_nextCorner()) {
			m_nDistance += m_nStep;
			return m_bHasPoints = _beginDistance();
		}
	}

	assert (m_nextCorner != m_current);
	if (m_bHorizontal) {
		if (m_current.x > m_nextCorner.x)
			m_current.x -= m_nStep;
		else
			m_current.x += m_nStep;
	} else {
		if (m_current.y > m_nextCorner.y)
			m_current.y -= m_nStep;
		else
			m_current.y += m_nStep;
	}

	if (!_findNextInGrid()) {
		m_nDistance += m_nStep;
		return m_bHasPoints = _beginDistance();
	}

	return true;
}

template<size_t _cStepShift>
bool NearestPointsIterator<_cStepShift>::_findNextInGrid() {
	while (true) {
		if (m_rect.contains(m_current))
			return true;

		m_bGap = true;
		if (m_bHorizontal) {
			if (m_current.y < m_rect.nY0 || m_current.y >= m_rect.nY1) {
				if (!_nextCorner())
					return false;
			} else {
				if (m_current.x < m_rect.nX0 && m_nextCorner.x >= m_rect.nX0)
					m_current.x = m_rect.nX0;
				else if (m_current.x >= m_rect.nX1 && m_nextCorner.x < m_rect.nX1)
					m_current.x = m_rect.nX1 - m_nStep;
				else  if (!_nextCorner())
					return false;
			}
		} else {
			if (m_current.x < m_rect.nX0 || m_current.x >= m_rect.nX1) {
				if (!_nextCorner())
					return false;
			} else {
				if (m_current.y < m_rect.nY0 && m_nextCorner.y >= m_rect.nY0)
					m_current.y = m_rect.nY0;
				else if (m_current.y >= m_rect.nY1 && m_nextCorner.y < m_rect.nY1)
					m_current.y = m_rect.nY1 - m_nStep;
				else  if (!_nextCorner())
					return false;
			}
		}
	}

	return false;
}

class Grid {
public:
	static constexpr int ms_nWidthOfCell = 64;
	static constexpr int ms_nHeightOfCell = 64;
	static const int ms_nWidth = (g_nWidth - 1)/ms_nWidthOfCell + 1;
	static const int ms_nHeight = (g_nHeight - 1)/ms_nHeightOfCell + 1;
	static const int ms_nSize = ms_nWidth*ms_nHeight;
	static const Rect2D gridRect;

	struct GridCell {
		std::vector<const KeyPoint3D *> points;
		Rect2D rect;
	};

	class RectCellsIterator {
		const Grid &m_grid;
		const Rect2D m_rect;
		const GridCell *m_pRowBegin;
		const GridCell *m_pCurrent;

	public:
		RectCellsIterator(const Grid &_grid, const Rect2D &_rect);

		const GridCell *current() const {
			return m_pCurrent;
		}

		bool moveNext();
	};

	class NearestCellsIterator : public NearestPointsIterator<0> {
		const Grid *m_pGrid;
	public:
		NearestCellsIterator() : m_pGrid(nullptr) { }

		NearestCellsIterator(const Grid &_grid, const Vector2D &_centre, int _nMaxCellDistance) : NearestPointsIterator(_centre, _nMaxCellDistance, gridRect), m_pGrid(&_grid) {

		}

		const GridCell &current() const {
			assert(gridRect.contains(m_current));
			return m_pGrid->m_cells[m_current.x + m_current.y*ms_nWidth];
		}
	};

	Grid() : m_pEnd(m_cells + ms_nSize) {
		for (int i = 0; i < ms_nWidth; ++i)
			for (int j = 0; j < ms_nHeight; ++j)
				m_cells[i + j*ms_nWidth].rect = Rect2D(i*ms_nWidthOfCell, j*ms_nHeightOfCell, i*ms_nWidthOfCell + ms_nWidthOfCell, j*ms_nHeightOfCell + ms_nHeightOfCell);
	}

	void add(const KeyPoint3D *_pPoint) {
		const Vector2D &point = _pPoint->tagPoint;
		if (!Application::getScreenRect().contains(point))
			return;

		const int nCol = point.x/ms_nWidthOfCell;
		const int nRow = point.y/ms_nHeightOfCell;

		GridCell &cell = getCell(nCol, nRow);
		cell.points.push_back(_pPoint);
	}

	inline
	RectCellsIterator getRectCells(const Rect2D &_rect) const {
		return RectCellsIterator(*this, _rect);
	}

	inline
	NearestCellsIterator getNearestCells(const Vector2D &_point, int _nMaxCellDistance) const {
		return NearestCellsIterator(*this, Vector2D(_point.x/ms_nWidthOfCell, _point.y/ms_nHeightOfCell), _nMaxCellDistance);
	}

	GridCell &getCell(int _nCol, int _nRow) {
		assert(_nCol >= 0 && _nCol < ms_nWidth && _nRow >= 0 && _nRow < ms_nHeight);
		return m_cells[_nCol + _nRow*ms_nWidth];
	}

	void sortCellsPoints() {
		for (GridCell *pCell = m_cells, *pEnd = m_cells + ms_nSize; pCell != pEnd; ++pCell) {
			std::sort(pCell->points.begin(), pCell->points.end(), [](const KeyPoint3D *_p1, const KeyPoint3D *_p2) {
				const size_t c1 = _p1->statistic.cSuccessDetections;
				const size_t c2 = _p2->statistic.cSuccessDetections;
				return  /*c1 == c2 ? _p1->nValue > _p2->nValue : */c1 > c2;
			});
		}
	}

	void clear() {
		for (GridCell *pCell = m_cells, *pEnd = m_cells + ms_nSize; pCell != pEnd; ++pCell)
			pCell->points.clear();
	}

private:
	Grid(const Grid &);

	GridCell m_cells[ms_nSize];
	const GridCell *m_pEnd;
};

#endif //__GRID_KP_
