#ifndef _QUADRANGLE_ITERATOR_
#define _QUADRANGLE_ITERATOR_

#include <functional>

#include "BresenhamIterator.h"

class QuadrangleIterator {
	Vector2D m_points[4];
	BresenhamIterator m_itLeft, m_itRigth;
	int m_nLeftIndex, m_nRigthIndex;
	Vector2D m_lastPoint;

	int nIteratorsChanged;
	int nCurY;
	int nBegX;
	int nEndX;
	int nCurX;
	bool m_bLastLine;

	int _nextIndex(int index) { return (index + 1)%4;}
	int _prevIndex(int index) { return (index + 3)%4;}

	void _nextLine() {
		assert(!m_bLastLine);
		assert(m_itLeft.getY() == m_itRigth.getY());
		nEndX = nBegX = m_itLeft.getX();
		nCurY = m_itLeft.getY();

		while (m_itLeft.getY() == nCurY) {
			nBegX = std::min(nBegX, m_itLeft.getX());
			nEndX = std::max(nEndX, m_itLeft.getX());

			if (!m_itLeft.moveNext()) {
				if (nIteratorsChanged++ >= 2) {
					m_bLastLine = true;
					break;
				}

				if (m_itLeft.getTargetX() == m_lastPoint.x && m_itLeft.getTargetY() == m_lastPoint.y)
					m_itLeft = BresenhamIterator::create(m_lastPoint, Vector2D(m_itRigth.getTargetX(), m_itRigth.getTargetY()));
				else
					m_itLeft = BresenhamIterator::create(Vector2D(m_itLeft.getTargetX(), m_itLeft.getTargetY()), m_lastPoint);
			}
		}

		while (m_itRigth.getY() == nCurY) {
			nBegX = std::min(nBegX, m_itRigth.getX());
			nEndX = std::max(nEndX, m_itRigth.getX());

			if (!m_itRigth.moveNext()) {
				if (nIteratorsChanged++ >= 2) {
					m_bLastLine = true;
					break;
				}

				if (m_itRigth.getTargetX() == m_lastPoint.x && m_itRigth.getTargetY() == m_lastPoint.y)
					m_itRigth = BresenhamIterator::create(m_lastPoint, Vector2D(m_itLeft.getTargetX(), m_itLeft.getTargetY()));
				else
					m_itRigth = BresenhamIterator::create(Vector2D(m_itRigth.getTargetX(), m_itRigth.getTargetY()), m_lastPoint);
			}
		}

		nCurX = nBegX;
	}

public:
	QuadrangleIterator(const Vector2D &p1, const Vector2D &p2, const Vector2D &p3, const Vector2D &p4) : nIteratorsChanged(0), m_bLastLine(false) {
		m_points[0] = p1;
		m_points[1] = p2;
		m_points[2] = p3;
		m_points[3] = p4;

		int nIndex = -1;
		int nMinY = std::numeric_limits<int>::max();

		for (int i = 0; i < 4; ++i)
			if (m_points[i].y < nMinY) {
				nMinY = m_points[i].y;
				nIndex = i;
			}

		m_nLeftIndex = _prevIndex(nIndex);
		m_nRigthIndex = _nextIndex(nIndex);
		m_lastPoint = m_points[_nextIndex(m_nRigthIndex)];

		m_itLeft = BresenhamIterator::create(m_points[nIndex], m_points[m_nLeftIndex]);
		m_itRigth = BresenhamIterator::create(m_points[nIndex], m_points[m_nRigthIndex]);

		_nextLine();
	}

	bool moveNext() {
		if (m_bLastLine && nCurX == nEndX)
			return false;

		if (nCurX == nEndX)
			_nextLine();
		else
			nCurX++;

		return true;
	}

	int getX() const {
		return nCurX;
	}

	int getY() const {
		return nCurY;
	}

	Vector2D Point() const {
		return Vector2D(nCurX, nCurY);
	}
};

#endif //_QUADRANGLE_ITERATOR_
