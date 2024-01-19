#include <iterator>
#include <cstdlib>
#include <math.h>
#include <algorithm>

#include "Hessian.h"

HessianTransform::HessianTransform(const Picture &_picture, Rect2D _rect) : m_integral(_picture.getIntegral()), m_rect(_rect),
	m_bUseDynamicOnly(_rect.empty()), m_dynamicGrid(m_levels, m_integral)
{
	m_rect.nX0 = m_rect.nX0&(~1);
	m_rect.nY0 = m_rect.nY0&(~1);

	const size_t cLevelsCount = sizeof(pHessianSizesX)/sizeof(int);
	m_levels.reserve(cLevelsCount);
	for (size_t cLevelId = 0; cLevelId < cLevelsCount; ++cLevelId) {
		if (!m_bUseDynamicOnly) {
			m_levels.push_back(LevelData(cLevelId, Vector2D(m_rect.nX0, m_rect.nY0), m_rect.width(), m_rect.height()));
			m_levels.back().init();
			m_levels.back().calculate(m_integral, g_nTrashold);
		} else {
			m_levels.push_back(LevelData(cLevelId));
		}
	}
}

bool HessianTransform::_compareWithLevel(const LevelData &_level, Vector2D _point, int _nCurVal, int _nShift, int (&_n9matrix)[9]) {
	_point.x += _nShift;
	_point.y += _nShift;

	if (_point.x < 0 || _point.x >= _level.nWidth || _point.y < 0 || _point.y >= _level.nHeight)
		return true;

	const bool bLeftOut = _point.x == 0;
	const bool bRigthOut = _point.x == _level.nWidth - 1;
	const bool bTopOut = _point.y == 0;
	const bool bBottomOut = _point.y == _level.nHeight - 1;

	const int *pCurrent = _level.getPointerTo(_point.x, _point.y);
	assert(pCurrent < _level.pDataEnd);

	if (_nCurVal > (_n9matrix[4] = *pCurrent) &&
		(bLeftOut || (_n9matrix[3] = _nCurVal > *(pCurrent - 1))) &&
		(bRigthOut || (_n9matrix[5] = _nCurVal > *(pCurrent + 1))))
	{
		const int *pPrevRow = pCurrent - _level.nWidth;
		assert(pPrevRow < _level.pDataEnd);

		if (bTopOut || (_nCurVal > (_n9matrix[1] = *(pPrevRow)) &&
			_nCurVal > (_n9matrix[0] = *(pPrevRow - 1)) &&
			_nCurVal > (_n9matrix[2] = *(pPrevRow + 1))))
		{
			const int *pNextRow = pCurrent + _level.nWidth;
			assert(bBottomOut || pNextRow < _level.pDataEnd);

			if (bBottomOut || (_nCurVal > (_n9matrix[7] = *(pNextRow)) &&
				_nCurVal > (_n9matrix[6] = *(pNextRow - 1)) &&
				_nCurVal > (_n9matrix[8] = *(pNextRow + 1))))
			{
				if (bLeftOut) {
					_n9matrix[0] = _n9matrix[2];
					_n9matrix[3] = _n9matrix[5];
					_n9matrix[6] = _n9matrix[8];
				} else if (bRigthOut) {
					_n9matrix[2] = _n9matrix[0];
					_n9matrix[5] = _n9matrix[3];
					_n9matrix[8] = _n9matrix[6];
				}

				if (bTopOut) {
					_n9matrix[0] = _n9matrix[6];
					_n9matrix[1] = _n9matrix[7];
					_n9matrix[2] = _n9matrix[8];
				} else if (bBottomOut) {
					_n9matrix[6] = _n9matrix[0];
					_n9matrix[7] = _n9matrix[1];
					_n9matrix[8] = _n9matrix[2];
				}
				return true;
			}
		}
	}

	return false;
}


static
float _determinant(int (&_mA)[9]) {
	return _mA[0]*(float(_mA[4])*_mA[8] - float(_mA[7])*_mA[5]) -
		_mA[1]*(float(_mA[3])*_mA[8] - float(_mA[6])*_mA[5]) +
		_mA[2]*(float(_mA[3])*_mA[7] - float(_mA[6])*_mA[4]);

//	return float(_mA[0])*_mA[4]*_mA[8] + float(_mA[3])*_mA[7]*_mA[2] + float(_mA[1])*_mA[5]*_mA[6] -
//		float(_mA[2])*_mA[4]*_mA[6] - float(_mA[5])*_mA[7]*_mA[0] - float(_mA[1])*_mA[3]*_mA[8]
//	;
}

static
bool _solve (int (&_m)[9], int (&_v)[3], float (&_res)[3]) {
	float d = _determinant(_m);
	if( d == 0 )
		return false;
	d = 1/d;

	_res[0] = d*(_v[0]*(float(_m[4])*_m[8] - float(_m[5])*_m[7]) -
			_m[1]*(float(_v[1])*_m[8] - float(_m[5])*_v[2]) +
			_m[2]*(float(_v[1])*_m[7] - float(_m[4])*_v[2]));

	_res[1] = d*(_m[0]*(float(_v[1])*_m[8] - float(_m[5])*_v[2]) -
			_v[0]*(float(_m[3])*_m[8] - float(_m[5])*_m[6]) +
			_m[2]*(float(_m[3])*_v[2] - float(_v[1])*_m[6]));

	_res[2] = d*(_m[0]*(float(_m[4])*_v[2] - float(_v[1])*_m[7]) -
			_m[1]*(float(_m[3])*_v[2] - float(_v[1])*_m[6]) +
			_v[0]*(float(_m[3])*_m[7] - float(_m[4])*_m[6]));
	return true;
}

static
int _round(float value) {
	double intpart, fractpart;
	fractpart = modf(value, &intpart);
	if ((fabs(fractpart) != 0.5) || ((((int)intpart) % 2) != 0))
		return (int)(value + (value >= 0 ? 0.5 : -0.5));
	else
		return (int)intpart;
}

static inline
std::vector<LevelData>::iterator _next(const std::vector<LevelData>::iterator &_it) {
	std::vector<LevelData>::iterator newIt = _it;
	++newIt;
	return newIt;
}

static inline
std::vector<LevelData>::iterator _prev(const std::vector<LevelData>::iterator &_it) {
	std::vector<LevelData>::iterator newIt = _it;
	--newIt;
	return newIt;
}


int HessianTransform::_interpolateKeypoint(int (&N9)[3][9], int _nDx, int _nDy, int _nDs, KeyPoint& _kpt) {
	int b[3] = {
		-(N9[1][5]-N9[1][3])/2,
		-(N9[1][7]-N9[1][1])/2,
		-(N9[2][4]-N9[0][4])/2
	};

	int a[9] = {
		N9[1][3]-2*N9[1][4]+N9[1][5],            // 2nd deriv x, x
		(N9[1][8]-N9[1][6]-N9[1][2]+N9[1][0])/4, // 2nd deriv x, y
		(N9[2][5]-N9[2][3]-N9[0][5]+N9[0][3])/4, // 2nd deriv x, s
		(N9[1][8]-N9[1][6]-N9[1][2]+N9[1][0])/4, // 2nd deriv x, y
		N9[1][1]-2*N9[1][4]+N9[1][7],            // 2nd deriv y, y
		(N9[2][7]-N9[2][1]-N9[0][7]+N9[0][1])/4, // 2nd deriv y, s
		(N9[2][5]-N9[2][3]-N9[0][5]+N9[0][3])/4, // 2nd deriv x, s
		(N9[2][7]-N9[2][1]-N9[0][7]+N9[0][1])/4, // 2nd deriv y, s
		N9[0][4]-2*N9[1][4]+N9[2][4]			 // 2nd deriv s, s
	};

	float x[3];
	bool bOk = _solve(a, b, x) && (x[0] != 0 || x[1] != 0 || x[2] != 0) &&
		std::abs(x[0]) <= 1 && std::abs(x[1]) <= 1 && std::abs(x[2]) <= 1;

	if (bOk) {
		//_kpt.point.x += _round(x[0]*_nDx);
		//_kpt.point.y += _round(x[1]*_nDy);
		_kpt.nSize = _kpt.nSize + _round(x[2]*_nDs);
	}
	return bOk;
}


void HessianTransform::findKeyPoints() {
	assert(!m_bUseDynamicOnly);
	for (auto it = m_levels.begin(); it != m_levels.end(); ++it) {
		const Vector2D levelTopLeft = it->topLeft;
		const int nCurShift = it->nShift;
		const int nFullShift = pHessianSizesX[it->cId]/2;
		LevelData::MaximumsIterator maxIt = it->getMaximums();
		const bool bFirstLevel = it == m_levels.begin();
		const bool bLastLevel = _next(it) == m_levels.end();
		int (&n9matrix)[3][9] = maxIt.m_n9matrix;

		while (maxIt.moveNext()) {
			Vector2D p = maxIt.current();
			const int nValue = maxIt.value();

			if (!bFirstLevel) {
				auto prevIt = _prev(it);
				if (!_compareWithLevel(*prevIt, p, nValue, nCurShift - prevIt->nShift, n9matrix[0]))
					continue;
			} else
				memset(n9matrix[0], 0, 9*sizeof(int));

			if (!bLastLevel) {
				auto nextIt = _next(it);
				if (!_compareWithLevel(*nextIt, p, nValue, nCurShift - nextIt->nShift, n9matrix[2]))
					continue;
			} else
				memset(n9matrix[2], 0, 9*sizeof(int));

			Vector2D pointInPicture = p*2 + levelTopLeft;
			pointInPicture.x += nFullShift;
			pointInPicture.y += nFullShift;
			GridCell &cell = m_grid.getCellByPoint(pointInPicture.x, pointInPicture.y);

			if (cell.nThreshold >= nValue)
				continue;

			Vector2D maxPoint = pointInPicture;
			int nMaxVal = nValue;
			const LevelData &level = m_levels[it->cId];
			size_t cPointNum = 0;
			for (int nCurY = pointInPicture.y - 1 - nFullShift, nEndY = nCurY + 3; nCurY != nEndY; ++nCurY) {
				for (int nCurX = pointInPicture.x - 1 - nFullShift, nEndX = nCurX + 3; nCurX != nEndX; ++nCurX) {
					if (cPointNum++ == 4)
						continue;

					const int nVal = level.constants.calculate(m_integral.getPointerTo(nCurX, nCurY));
					if (nVal > nMaxVal) {
						nMaxVal = nVal;
						maxPoint = Vector2D(nCurX + nFullShift, nCurY + nFullShift);
					}
				}
			}

			GridCell &cell2 = m_grid.getCellByPoint(maxPoint.x, maxPoint.y);
			if (cell2.nThreshold >= nMaxVal)
				continue;

			KeyPoint &kp = cell2.addKeyPoint(maxPoint, nMaxVal);
			kp.pointInLevel = p;
			kp.nSize = pHessianSizesX[it->cId];
			kp.bHessianSign = it->getSign(p, m_integral);
			kp.nLevelId = it->cId;

			if (!bLastLevel && !bFirstLevel && !_interpolateKeypoint(n9matrix, 2, 2, 6, kp)) {
				auto iLast = cell.points.end();
				--iLast;
				cell.points.erase(iLast);
			}
		}
	}

	m_grid.removeMinorPointsAndCollectPointsForLines(m_pointsForLinesDetection);
}

void HessianTransform::test() {
	unsigned char *pBuf = new unsigned char[g_nWidth*g_nHeight];

	for (int i = 0; i < g_nWidth; ++i)
		for (int j = 0; j < g_nHeight; ++j)
			pBuf[i + j*g_nWidth] = j + i;

	Picture pic(pBuf);
	HessianTransform hessian(pic);
	LevelConstants consts0(0);
	assert(consts0.calculate(hessian.m_integral.begin(), g_nTrashold) == 0);
	LevelConstants consts1(1);
	assert(consts1.calculate(hessian.m_integral.begin(), g_nTrashold) == 0);
	LevelConstants consts2(2);
	assert(consts2.calculate(hessian.m_integral.begin(), g_nTrashold) == 0);
	LevelConstants consts3(3);
	assert(consts3.calculate(hessian.m_integral.begin(), g_nTrashold) == 0);
	delete[] pBuf;
}

int getLevelIdBySize(int _nSize) {
	int nPrev = 0;
	for (int nLevel = 0; nLevel != sizeof(pHessianSizesX)/sizeof(int); ++nLevel) {
		if (_nSize < (pHessianSizesX[nLevel] + nPrev)/2)
			return nLevel == 0 ? 0 : nLevel - 1;

		nPrev = pHessianSizesX[nLevel];
	}
	return sizeof(pHessianSizesX)/sizeof(int) - 1;
}
