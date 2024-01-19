#ifndef __BRESENHAM__
#define __BRESENHAM__

#include <limits>
#include <cmath>
#include <functional>

#include "Primitives2D.h"

class BresenhamIterator
{
private:
	int x, y;
	int x1, y1;
	bool replaced;
	int error;
	int halfCellError;
	int cellError;
	int stepError;
	int dx;
	int dy;
	bool bEof;

public:
	int getStepsCount() {
		return std::abs(x1 - x);
	}

	int getX() const {
		return replaced ? y : x;
	}

	int getY() const {
		return replaced ? x : y;
	}

	int getTargetX() const {
		return replaced ? y1 : x1;
	}

	int getTargetY() const {
		return replaced ? x1 : y1;
	}

	Vector2D point() const {
		return replaced ? Vector2D(y, x) : Vector2D(x, y);
	}

	Vector2D near1() const {
		Vector2D p = point();

		if (!replaced)
			--p.y;
		else
			--p.x;

		return p;
	}

	Vector2D near2() {
		Vector2D p = point();

		if (!replaced)
			++p.y;
		else
			++p.x;

		return p;
	}

	bool eof() const {
		return bEof;
	}

	BresenhamIterator() { }

	BresenhamIterator(int _x, int _y, int _x1, int _y1) : x(_x), y(_y), x1(_x1), y1(_y1), replaced(false), bEof(false) {
		dx = x1 - x;
		dy = y1 - y;

		if (dx == 0 && dy == 0)
			return;

		if (dy*dy > dx*dx)
		{
			replaced = true;
			std::swap(dx, dy);
			std::swap(x, y);
			std::swap(x1, y1);
		}

		halfCellError = std::abs(dx);
		cellError = halfCellError * 2;

		stepError = std::abs(dy) * cellError/std::abs(dx);
		error = 0;

		dx = dx > 0 ? 1 : -1;
		dy = dy > 0 ? 1 : -1;
	}

	bool moveNext() {
		if (x == x1)
		{
			bEof = true;
			return false;
		}

		x += dx;
		error += stepError;

		if (error >= halfCellError) {
			y += dy;
			error -= cellError;
		}

		return true;
	}

	bool movePrev() {
		x -= dx;
		error -= stepError;

		if (error < -halfCellError) {
			y -= dy;
			error += cellError;
		}

		return true;
	}

	bool passSteps(int _nStepsCount) {
		if (_nStepsCount >= 0) {
			bool bRes = true;
			while (_nStepsCount > 0 && (bRes = moveNext()))
				--_nStepsCount;

			return bRes;
		} else {
			while (_nStepsCount++ < 0)
				movePrev();

			return true;
		}
	}

	void setUnlimited() {
		x1 = y1 = std::numeric_limits<int>::max();
	}

	static inline
	BresenhamIterator create(const Vector2D &_p1, const Vector2D &_p2) {
		return BresenhamIterator(_p1.x, _p1.y, _p2.x, _p2.y);
	}
};

typedef int FunctionInt2D(int, int);

class FloatStepIterator {
	static const int ms_nPartsInOne = 1024;

	BresenhamIterator &m_iPoints;

	float m_fCurrentDelta;
	float m_fStep;
	Vector2D m_prevPoint;

	void _actualize() {
		while (m_fCurrentDelta > 0.0f) {
			m_prevPoint = m_iPoints.point();
			m_iPoints.moveNext();
			--m_fCurrentDelta;
		}
	}

public:
	FloatStepIterator(BresenhamIterator &_iPoints, float _fStartIdx, float _fStep) : m_iPoints(_iPoints), m_fCurrentDelta(_fStartIdx), m_fStep(_fStep),
		m_prevPoint(_iPoints.point()) {
		_actualize();
	}

	void operator ++() {
		m_fCurrentDelta += m_fStep;
		_actualize();
	}

//	int calculate(std::function<FunctionInt2D> _func) const {
//		assert(m_fCurrentDelta >= -1 && m_fCurrentDelta <= 0);
//		if (m_fCurrentDelta <= -0.9)
//			return _func(m_prevPoint.x, m_prevPoint.y);

//		const Vector2D curPoint = m_iPoints.point();
//		if (m_fCurrentDelta >= -0.1)
//			return _func(curPoint.x, curPoint.y);

//		const float fCoef = -m_fCurrentDelta;
//		const float fRes = _func(m_prevPoint.x, m_prevPoint.y)*fCoef + _func(curPoint.x, curPoint.y)*(1 - fCoef);
//		return fast_floor(fRes + 0.5);
//	}

	inline
	bool needSecond() const {
		return m_fCurrentDelta > -0.9;
	}

	inline
	bool needFirst() const {
		return m_fCurrentDelta < -0.1;
	}

	float getCoef() const {
		return -m_fCurrentDelta;
	}

	const Vector2D &firstPoint() const {
		return m_prevPoint;
	}

	Vector2D secondPoint() const {
		return m_iPoints.point();
	}

	inline
	int calculate(int _nVal1, int _nVal2) {
		if (!needSecond())
			return _nVal1;

		if (!needFirst())
			return _nVal2;

		const float fCoef = getCoef();
		const float fRes = _nVal1*fCoef + _nVal2*(1 - fCoef);
		return fast_floor(fRes + 0.5);
	}
};

class StripIterator {
	BresenhamIterator m_it1, m_it2, m_it3;
	float m_fSecondErrorInStep;
	float m_fCurrentError;
	int m_nStep;
	bool m_bSwapped;

public:
	StripIterator(const Vector2D &_p11, const Vector2D &_p12, const Vector2D &_p21, const Vector2D &_p22) :
	m_it1(_p11.x, _p11.y, _p12.x, _p12.y), m_it2(_p21.x, _p21.y, _p22.x, _p22.y), m_it3(_p11.x, _p11.y, _p21.x, _p21.y), m_fCurrentError(0), m_nStep(0), m_bSwapped(false) {
		if (m_it1.getStepsCount() > m_it2.getStepsCount()) {
			std::swap(m_it1, m_it2);
			m_bSwapped = true;
		}

		if (m_it1.getStepsCount() == 0)
			return;

		m_fSecondErrorInStep = float(m_it2.getStepsCount())/m_it1.getStepsCount();
		assert(m_fSecondErrorInStep >= 1);
	}

	bool moveNext() {
		if (!m_it3.eof() && m_it3.moveNext())
			return true;

		if (!m_it1.moveNext())
			return false;

		++m_nStep;
		m_fCurrentError += m_fSecondErrorInStep;

		while (m_fCurrentError > 0.5) {
			m_it2.moveNext();
			m_fCurrentError -= 1;
		}

		m_it3 = BresenhamIterator(m_it1.getX(), m_it1.getY(), m_it2.getX(), m_it2.getY());
		assert(m_fCurrentError >= -0.5 && m_fCurrentError <= 0.5);
		return true;
	}

	int getX() const {
		return m_it3.getX();
	}

	int getY() const {
		return m_it3.getY();
	}

	int getStep() const {
		return m_nStep;
	}

	Vector2D point() const {
		return Vector2D(getX(), getY());
	}

	bool eof() const {
		return m_it1.eof() && m_it3.eof();
	}

	Vector2D getMainLinePoint() const {
		return m_bSwapped ? m_it2.point() : m_it1.point();
	}
};

#endif //__BRESENHAM__
