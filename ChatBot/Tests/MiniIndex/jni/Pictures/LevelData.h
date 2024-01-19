#ifndef _LEVEL_DATA_
#define _LEVEL_DATA_

#include "IntegralTransform.h"

constexpr int g_nTrashold = 4500;
constexpr int g_nDynamicThreshol = g_nTrashold*2/3;

namespace {
const size_t g_cLevelsCount = 6;
const int pLevelZooms[] = {3, 5, 7, 9, 12, 15};
const int pHessianSizesX[] = {9, 15, 21, 27, 36, 45};
const int pDiagBoundSizesX[] = {1, 2, 2, 3, 4, 5};
const int pDiagCentreSizesX[] = {1, 1, 2, 3, 4, 5};
const int pLineBoundSizesX[] = {2, 3, 5, 6, 8, 10};
const float g_f2PiSqrt = ::sqrt(2*g_PI);
const float g_fLevelGaussiaMultipliers[] = {1.0f/(2*4*g_f2PiSqrt), 1.0f/(2*6*g_f2PiSqrt), 1.0f/(2*9*g_f2PiSqrt), 1.0f/(2*12*g_f2PiSqrt), 1.0f/(2*16*g_f2PiSqrt), 1.0f/(2*20*g_f2PiSqrt)};
const float g_fLevelGaussiaExpMultiplier[] = {-1.0f/(2*4*4), -1.0f/(2*6*6), -1.0f/(2*9*9), -1.0f/(2*12*12), -1.0f/(2*16*16), -1.0f/(2*20*20)};
}

struct LevelConstants {
	static const int c_nCoefficient = 1000;

	LevelConstants(size_t _cLevelId, int _nWidth = g_nWidth) : nIntegralWidth(_nWidth) {
		assert(_cLevelId < sizeof(pHessianSizesX)/sizeof(int));

		const int nHessianSizeY = pHessianSizesX[_cLevelId]*_nWidth;
		const int nDiagCentreSizeY = pDiagCentreSizesX[_cLevelId]*_nWidth;
		const int nLineBoundSizesY = pLineBoundSizesX[_cLevelId]*_nWidth;

		nHessianSize = pHessianSizesX[_cLevelId];
		nScreenEdgeMinDistance = nHessianSize/2;

		nWeightDivider = pHessianSizesX[_cLevelId]*pHessianSizesX[_cLevelId]*pHessianSizesX[_cLevelId]*pHessianSizesX[_cLevelId]/c_nCoefficient;

		nVericalTopLeft = nLineBoundSizesY;
		nVericalTopRigth = nVericalTopLeft + pHessianSizesX[_cLevelId];
		nVerticalBottomLeft = nHessianSizeY - nLineBoundSizesY;
		nVerticalBottomRigth = nVerticalBottomLeft + pHessianSizesX[_cLevelId];

		const int nOneThirdX = pHessianSizesX[_cLevelId]/3;
		const int nOneThirdY = nHessianSizeY/3;

		nVerticalCentreTopLeft = nVericalTopLeft + nOneThirdX;
		nVerticalCentreTopRigth = nVerticalCentreTopLeft + nOneThirdX;
		nVerticalCentreBottomLeft = nVerticalBottomLeft + nOneThirdX;
		nVerticalCentreBottomRigth = nVerticalCentreBottomLeft + nOneThirdX;

		nHorizontalTopLeft = pLineBoundSizesX[_cLevelId];
		nHorizontalTopRigth = pHessianSizesX[_cLevelId] - pLineBoundSizesX[_cLevelId];
		nHorizontalBottomLeft = nHorizontalTopLeft + nHessianSizeY;
		nHorizontalBottomRigth = nHorizontalTopRigth + nHessianSizeY;

		nHorizontalCentreTopLeft = nHorizontalTopLeft + nOneThirdY;
		nHorizontalCentreBottomLeft = nHorizontalCentreTopLeft + nOneThirdY;
		nHorizontalCentreTopRigth  = nHorizontalTopRigth + nOneThirdY;
		nHorizontalCentreBottomRigth = nHorizontalCentreTopRigth + nOneThirdY;

		const int nDiagBoundX = pDiagBoundSizesX[_cLevelId];
		const int nDiagCentreBoundX = pDiagCentreSizesX[_cLevelId];
		const int nSize = pHessianSizesX[_cLevelId];
		const int nDiagSquareSize = (nSize - 2*nDiagBoundX - nDiagCentreBoundX) / 2;
		const int nDiagSecondSquareX = nDiagBoundX + nDiagSquareSize + nDiagCentreBoundX;

		nDiagonalLeftTopLeft = nDiagBoundX + pDiagBoundSizesX[_cLevelId]*_nWidth;
		nDiagonalLeftTopRigth = nDiagonalLeftTopLeft + nDiagSquareSize;
		nDiagonalLeftTopCentreLeft = nDiagonalLeftTopLeft + nDiagSquareSize*_nWidth;
		nDiagonalLeftTopCentreRigth = nDiagonalLeftTopRigth + nDiagSquareSize*_nWidth;
		nDiagonalLeftBottomCentreLeft = nDiagonalLeftTopCentreLeft + nDiagCentreSizeY;
		nDiagonalLeftBottomCentreRigth = nDiagonalLeftTopCentreRigth + nDiagCentreSizeY;
		nDiagonalLeftBottomLeft = nDiagonalLeftBottomCentreLeft + nDiagSquareSize*_nWidth;
		nDiagonalLeftBottomRigth = nDiagonalLeftBottomCentreRigth + nDiagSquareSize*_nWidth;

		nDiagonalRigthTopLeft = nDiagonalLeftTopLeft + nDiagSecondSquareX;
		nDiagonalRigthTopRigth = nDiagonalLeftTopRigth + nDiagSecondSquareX;
		nDiagonalRigthTopCentreLeft = nDiagonalLeftTopCentreLeft + nDiagSecondSquareX;
		nDiagonalRigthTopCentreRigth = nDiagonalLeftTopCentreRigth + nDiagSecondSquareX;
		nDiagonalRigthBottomCentreLeft = nDiagonalLeftBottomCentreLeft + nDiagSecondSquareX;
		nDiagonalRigthBottomCentreRigth = nDiagonalLeftBottomCentreRigth + nDiagSecondSquareX;
		nDiagonalRigthBottomLeft = nDiagonalLeftBottomLeft + nDiagSecondSquareX;
		nDiagonalRigthBottomRigth = nDiagonalLeftBottomRigth + nDiagSecondSquareX;
	}

	bool calcSign(const int *_pTopLeft) const {
		int nVert = _pTopLeft[nVericalTopLeft] + _pTopLeft[nVerticalBottomRigth] - _pTopLeft[nVericalTopRigth] - _pTopLeft[nVerticalBottomLeft];
		nVert -= 3*(_pTopLeft[nVerticalCentreTopLeft] + _pTopLeft[nVerticalCentreBottomRigth] - _pTopLeft[nVerticalCentreTopRigth] - _pTopLeft[nVerticalCentreBottomLeft]);
#ifndef NDEBUG
		int nHor = _pTopLeft[nHorizontalTopLeft] + _pTopLeft[nHorizontalBottomRigth] - _pTopLeft[nHorizontalTopRigth] - _pTopLeft[nHorizontalBottomLeft];
		nHor -= 3*(_pTopLeft[nHorizontalCentreTopLeft] + _pTopLeft[nHorizontalCentreBottomRigth] - _pTopLeft[nHorizontalCentreTopRigth] - _pTopLeft[nHorizontalCentreBottomLeft]);
		if ((nHor >= 0) != (nVert >= 0))
			calculate(_pTopLeft, 0);

		assert((nHor >= 0) == (nVert >= 0));
#endif
		return nVert >= 0;
	}

	bool inBounds(const int *_pVal, const int *_pBegin, const int *_pEnd) const {
		if (_pVal >= _pBegin && _pVal < _pEnd)
			return true;

		assert(false);
		return false;
	}

	bool testBounds(const int *_pTopLeft, const int *_pBegin, const int *_pEnd) const {
		if (!inBounds(_pTopLeft, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nVericalTopLeft, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nVerticalBottomRigth, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nVericalTopRigth, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nVerticalBottomLeft, _pBegin, _pEnd))
			return false;

		if (!inBounds(_pTopLeft + nVerticalCentreTopLeft, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nVerticalCentreBottomRigth, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nVerticalCentreTopRigth, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nVerticalCentreBottomLeft, _pBegin, _pEnd))
			return false;

		if (!inBounds(_pTopLeft + nHorizontalTopLeft, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nHorizontalBottomRigth, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nHorizontalTopRigth, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nHorizontalBottomLeft, _pBegin, _pEnd))
			return false;

		if (!inBounds(_pTopLeft + nHorizontalCentreTopLeft, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nHorizontalCentreBottomRigth, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nHorizontalCentreTopRigth, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nHorizontalCentreBottomLeft, _pBegin, _pEnd))
			return false;

		if (!inBounds(_pTopLeft + nDiagonalLeftTopLeft, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nDiagonalLeftTopCentreRigth, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nDiagonalLeftTopRigth, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nDiagonalLeftTopCentreLeft, _pBegin, _pEnd))
			return false;

		if (!inBounds(_pTopLeft + nDiagonalRigthBottomCentreLeft, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nDiagonalRigthBottomRigth, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nDiagonalRigthBottomCentreRigth, _pBegin, _pEnd))
			return false;
		if (!inBounds(_pTopLeft + nDiagonalRigthBottomLeft, _pBegin, _pEnd))
			return false;

		return true;
	}

	int calculate(const int *_pTopLeft, int _nThrashold) const {
		int nVert = _pTopLeft[nVericalTopLeft] + _pTopLeft[nVerticalBottomRigth] - _pTopLeft[nVericalTopRigth] - _pTopLeft[nVerticalBottomLeft];
		nVert -= 3*(_pTopLeft[nVerticalCentreTopLeft] + _pTopLeft[nVerticalCentreBottomRigth] - _pTopLeft[nVerticalCentreTopRigth] - _pTopLeft[nVerticalCentreBottomLeft]);

		int nHor = _pTopLeft[nHorizontalTopLeft] + _pTopLeft[nHorizontalBottomRigth] - _pTopLeft[nHorizontalTopRigth] - _pTopLeft[nHorizontalBottomLeft];
		nHor -= 3*(_pTopLeft[nHorizontalCentreTopLeft] + _pTopLeft[nHorizontalCentreBottomRigth] - _pTopLeft[nHorizontalCentreTopRigth] - _pTopLeft[nHorizontalCentreBottomLeft]);

		if ((nVert >= 0) != (nHor >= 0))
			return 0;

		const int64_t lnFirstPart = int64_t(nVert)*nHor;

		if (lnFirstPart < _nThrashold)
			return 0;

		int nDiag = _pTopLeft[nDiagonalLeftTopLeft] + _pTopLeft[nDiagonalLeftTopCentreRigth] - _pTopLeft[nDiagonalLeftTopRigth] - _pTopLeft[nDiagonalLeftTopCentreLeft];
		nDiag -= _pTopLeft[nDiagonalLeftBottomCentreLeft] + _pTopLeft[nDiagonalLeftBottomRigth] - _pTopLeft[nDiagonalLeftBottomCentreRigth] - _pTopLeft[nDiagonalLeftBottomLeft];
		nDiag -= _pTopLeft[nDiagonalRigthTopLeft] + _pTopLeft[nDiagonalRigthTopCentreRigth] - _pTopLeft[nDiagonalRigthTopRigth] - _pTopLeft[nDiagonalRigthTopCentreLeft];
		nDiag += _pTopLeft[nDiagonalRigthBottomCentreLeft] + _pTopLeft[nDiagonalRigthBottomRigth] - _pTopLeft[nDiagonalRigthBottomCentreRigth] - _pTopLeft[nDiagonalRigthBottomLeft];

		nDiag = nDiag*9/10;

		const int64_t lnDelta = lnFirstPart - int64_t(nDiag)*nDiag;

		if (lnDelta < _nThrashold)
			return 0;

		//assert(int64_t(nVert)*int64_t(nHor) < std::numeric_limits<int>::max() && int64_t(nDiag)*int64_t(nDiag) < std::numeric_limits<int>::max());
		return lnDelta/nWeightDivider;
	}

	int calculate(const int *_pTopLeft) const {
		int nVert = _pTopLeft[nVericalTopLeft] + _pTopLeft[nVerticalBottomRigth] - _pTopLeft[nVericalTopRigth] - _pTopLeft[nVerticalBottomLeft];
		nVert -= 3*(_pTopLeft[nVerticalCentreTopLeft] + _pTopLeft[nVerticalCentreBottomRigth] - _pTopLeft[nVerticalCentreTopRigth] - _pTopLeft[nVerticalCentreBottomLeft]);

		int nHor = _pTopLeft[nHorizontalTopLeft] + _pTopLeft[nHorizontalBottomRigth] - _pTopLeft[nHorizontalTopRigth] - _pTopLeft[nHorizontalBottomLeft];
		nHor -= 3*(_pTopLeft[nHorizontalCentreTopLeft] + _pTopLeft[nHorizontalCentreBottomRigth] - _pTopLeft[nHorizontalCentreTopRigth] - _pTopLeft[nHorizontalCentreBottomLeft]);

		const int64_t lnFirstPart = int64_t(nVert)*nHor;

		int nDiag = _pTopLeft[nDiagonalLeftTopLeft] + _pTopLeft[nDiagonalLeftTopCentreRigth] - _pTopLeft[nDiagonalLeftTopRigth] - _pTopLeft[nDiagonalLeftTopCentreLeft];
		nDiag -= _pTopLeft[nDiagonalLeftBottomCentreLeft] + _pTopLeft[nDiagonalLeftBottomRigth] - _pTopLeft[nDiagonalLeftBottomCentreRigth] - _pTopLeft[nDiagonalLeftBottomLeft];
		nDiag -= _pTopLeft[nDiagonalRigthTopLeft] + _pTopLeft[nDiagonalRigthTopCentreRigth] - _pTopLeft[nDiagonalRigthTopRigth] - _pTopLeft[nDiagonalRigthTopCentreLeft];
		nDiag += _pTopLeft[nDiagonalRigthBottomCentreLeft] + _pTopLeft[nDiagonalRigthBottomRigth] - _pTopLeft[nDiagonalRigthBottomCentreRigth] - _pTopLeft[nDiagonalRigthBottomLeft];

		nDiag = nDiag*9/10;

		//assert(int64_t(nVert)*int64_t(nHor) < std::numeric_limits<int>::max() && int64_t(nDiag)*int64_t(nDiag) < std::numeric_limits<int>::max());
		return (lnFirstPart - int64_t(nDiag)*nDiag)/nWeightDivider;
	}

	int nIntegralWidth;

	int nHessianSize;
	int nScreenEdgeMinDistance;

	int nWeightDivider;

	int nVericalTopLeft;
	int nVericalTopRigth;
	int nVerticalBottomLeft;
	int nVerticalBottomRigth;
	int nVerticalCentreTopLeft;
	int nVerticalCentreTopRigth;
	int nVerticalCentreBottomLeft;
	int nVerticalCentreBottomRigth;

	int nHorizontalTopLeft;
	int nHorizontalTopRigth;
	int nHorizontalBottomLeft;
	int nHorizontalBottomRigth;
	int nHorizontalCentreTopLeft;
	int nHorizontalCentreTopRigth;
	int nHorizontalCentreBottomLeft;
	int nHorizontalCentreBottomRigth;

	int nDiagonalLeftTopLeft;
	int nDiagonalLeftTopRigth;
	int nDiagonalLeftTopCentreLeft;
	int nDiagonalLeftTopCentreRigth;
	int nDiagonalLeftBottomCentreLeft;
	int nDiagonalLeftBottomCentreRigth;
	int nDiagonalLeftBottomLeft;
	int nDiagonalLeftBottomRigth;

	int nDiagonalRigthTopLeft;
	int nDiagonalRigthTopRigth;
	int nDiagonalRigthTopCentreLeft;
	int nDiagonalRigthTopCentreRigth;
	int nDiagonalRigthBottomCentreLeft;
	int nDiagonalRigthBottomCentreRigth;
	int nDiagonalRigthBottomLeft;
	int nDiagonalRigthBottomRigth;
};

struct LevelData {
	static const int c_nStep = 2;
	LevelConstants constants;
	int nHessianSize;
	int nScreenEdgeMinDistance;
	Rect2D allowedRect;
	Vector2D topLeft;
	int nWidth;
	int nHeight;
	int nSize;

	int *pData;
	int *pDataEnd;
	int nShift;
	size_t cId;

	LevelData(size_t _cLevelId, Vector2D _topLeft = Vector2D(), int _nWidth = g_nWidth, int _nHeight = g_nHeight, int _nIntegralWidth = g_nWidth) :
		constants(_cLevelId, _nIntegralWidth),
		nHessianSize(constants.nHessianSize),
		nScreenEdgeMinDistance(constants.nScreenEdgeMinDistance),
		allowedRect(nScreenEdgeMinDistance + _topLeft.x, nScreenEdgeMinDistance + _topLeft.y,
			_nWidth + _topLeft.x - nScreenEdgeMinDistance - c_nStep, _nHeight + _topLeft.y - nScreenEdgeMinDistance - c_nStep
		),
		topLeft(_topLeft),
		nWidth((_nWidth - nHessianSize)/c_nStep),
		nHeight((_nHeight - nHessianSize)/c_nStep),
		nSize(nWidth*nHeight),
		pData(nullptr),
		pDataEnd(nullptr),
		nShift((nHessianSize - pHessianSizesX[0])/(2*c_nStep)),
		cId(_cLevelId)
	{

	}

	bool getSign(const Vector2D &_pointInLevel, const IntegralTransform &_integral) {
		const int *pInt = _integral.begin();
		return constants.calcSign(pInt + 2*_integral.getWidth()*_pointInLevel.y + 2*_pointInLevel.x);
	}

	void calculate(const IntegralTransform &_integral, int _nThreshold) {
		assert(_integral.getWidth() == constants.nIntegralWidth);
		assert(topLeft.x + nWidth + nHessianSize <= _integral.getWidth());
		assert(topLeft.y + nHeight + nHessianSize <= _integral.getHeight());
		const int *pInt = _integral.begin() + _integral.getWidth()*topLeft.y + topLeft.x;
		int *pResult = pData;

		for (int nRow = 0; nRow < nHeight; ++nRow) {
			const int *pNext = pInt + 2*_integral.getWidth();
			const int *pRowEnd = pInt + 2*nWidth;

			for (; pInt != pRowEnd; pInt += 2) {
				*pResult = constants.calculate(pInt, _nThreshold);
				assert(pResult != pData + nSize);
				++pResult;

			}

			pInt = pNext;
		}
	}

	void init() {
		pData = new int[nSize];
		pDataEnd = pData + nSize;
	}

	~LevelData() {
		delete[] pData;
	}

	int getValueAtPoint(Vector2D _point, const IntegralTransform &_integral) const {
		assert (constants.nIntegralWidth == _integral.getWidth());
//		_point -= topLeft;
		assert(_point.x >= 0 && _point.y >= 0);
		_point.x -= nScreenEdgeMinDistance;
		_point.y -= nScreenEdgeMinDistance;

		_point.x -= _integral.getRect().nX0;
		_point.y -= _integral.getRect().nY0;

		return constants.calculate(_integral.getPointerTo(_point));
//		const int nX = _point.x/2;
//		const int nY = _point.y/2;
//		const int *pVal = getPointerTo(nX, nY);
//		if ((_point.x&1) == 0 || nX == nWidth - 1) {
//			if ((_point.y&1) == 0 || nY == nHeight - 1)
//				return *pVal;

//			assert(pVal + nWidth < pDataEnd);
//			return (*pVal + *(pVal + nWidth))/2;
//		}

//		if ((_point.y&1) == 0 || nY == nHeight - 1) {
//			assert(pVal + 1 < pDataEnd);
//			return (*pVal + *(pVal + 1))/2;
//		}

//		assert(pVal + nWidth + 1 < pDataEnd);
//		return (*pVal + *(pVal + nWidth + 1))/2;
	}

	inline
	const int *getPointerTo(int _nX, int _nY) const {
		assert(_nX < nWidth && _nY < nHeight);
		return pData + nWidth*_nY + _nX;
	}

	class MaximumsIterator {
		const LevelData &m_data;
		int *m_pCurrent;
		int *m_pCurRowEnd;
		int m_nCurRow;
		int m_nCurValue;
		bool m_bCurNegative;
		Vector2D m_current;

	public:
		int m_n9matrix[3][9];

		MaximumsIterator(const LevelData &_data) : m_data(_data), m_pCurrent(_data.pData + _data.nWidth),
			m_pCurRowEnd(m_pCurrent + _data.nWidth), m_nCurRow(1)
		{

		}

		bool moveNext() {
			assert(m_pCurrent != m_pCurRowEnd);
			assert(m_pCurrent > m_data.pData && m_pCurrent < m_data.pDataEnd);
			m_pCurrent += 2;

			while (true) {
				if (m_pCurrent >= m_pCurRowEnd - 1) {
					m_pCurrent = m_pCurRowEnd + 2;
					m_pCurRowEnd += m_data.nWidth;
					++m_nCurRow;

					if (m_pCurRowEnd == m_data.pDataEnd) {
						m_pCurrent = m_pCurRowEnd;
						return false;
					}
				}

				m_nCurValue = *m_pCurrent;

				if (m_nCurValue < 0) {
					m_bCurNegative = true;
					//m_nCurValue = -m_nCurValue;
				} else
					m_bCurNegative = false;

				if (m_nCurValue > g_nTrashold && m_nCurValue > (m_n9matrix[1][3] = *(m_pCurrent - 1)) && m_nCurValue > (m_n9matrix[1][5] = *(m_pCurrent + 1))) {
					int *pPrevRow = m_pCurrent - m_data.nWidth;
					assert(pPrevRow < m_data.pDataEnd);

					if (m_nCurValue > (m_n9matrix[1][1] = *(pPrevRow)) && m_nCurValue > (m_n9matrix[1][0] = *(pPrevRow - 1)) && m_nCurValue > (m_n9matrix[1][2] = *(pPrevRow + 1))) {
						int *pNextRow = m_pCurrent + m_data.nWidth;
						assert(pNextRow < m_data.pDataEnd);

						if (m_nCurValue > (m_n9matrix[1][7] = *(pNextRow)) && m_nCurValue > (m_n9matrix[1][6] = *(pNextRow - 1)) && m_nCurValue > (m_n9matrix[1][8] = *(pNextRow + 1))) {
							m_current.x = pNextRow - m_pCurRowEnd;
							m_current.y = m_nCurRow;
							m_n9matrix[1][4] = m_nCurValue;
							return true;
						}
					}
				}


				++m_pCurrent;
			}

			return false;
		}

		Vector2D current() const {
			assert(m_pCurrent != m_pCurRowEnd && m_pCurrent != m_data.pData + m_data.nWidth);
			return m_current;
		}

		int value() const {
			return m_nCurValue;
		}

		bool isNegative() {
			return m_bCurNegative;
		}
	};

	MaximumsIterator getMaximums() const {
		return MaximumsIterator(*this);
	}
};

struct FragmentLevelData {
	static const int c_nStep = 2;
	LevelConstants constants;
	const IntegralTransform &integral;
	int nWidth;
	int nHeigth;
	int nHessianSize;
	int nFullShift;
	Rect2D allowedRect;

	int nShift;
	size_t cId;

	FragmentLevelData(size_t _cLevelId, IntegralTransform &_integral) : constants(_cLevelId, _integral.getWidth()),
		integral(_integral),
		nWidth(_integral.getWidth()),
		nHeigth(_integral.getHeight()),
		nHessianSize(pHessianSizesX[_cLevelId]),
		nFullShift(nHessianSize/2),
		allowedRect(nFullShift, nFullShift, nWidth - nFullShift - c_nStep, nHeigth - nFullShift - c_nStep),
		nShift((nHessianSize - pHessianSizesX[0])/(2*c_nStep)),
		cId(_cLevelId)
	{

	}

	inline
	const int *getIntegralPointerTo(const Vector2D &_point) const {
		assert(allowedRect.contains(_point));
		return integral.getPointerTo(_point - Vector2D(nFullShift, nFullShift));
	}

	inline
	int calcByTopLeft(const int *_pTopLeft) const {
		assert(_pTopLeft >= integral.begin() && _pTopLeft < integral.end());
		return constants.calculate(_pTopLeft, g_nTrashold);
	}

	bool findNearestMax(const Vector2D &_point, size_t _cMaxDistance, Vector2D &_result) {
		assert(allowedRect.contains(_point));
		const size_t cFieldWidth = 2*_cMaxDistance + 3;
		const size_t cBufSize = cFieldWidth*cFieldWidth;
		int *pBuf = new int[cBufSize];
		int *pBufCentre = pBuf + cBufSize/2;
		*pBufCentre = calcByTopLeft(getIntegralPointerTo(_point));

		_fillNextSqueareContour(_point, 1, pBufCentre, cFieldWidth);
		if (_isMax(pBufCentre, cFieldWidth)) {
			_result = _point;
			return true;
		}

		for (size_t cCurDist = 1; cCurDist <= _cMaxDistance; ++cCurDist) {
			_fillNextSqueareContour(_point, cCurDist + 1, pBufCentre, cFieldWidth);

			int *pBufTopLeft = pBufCentre - cCurDist - cFieldWidth*cCurDist;
			int *pBufTopRigth = pBufTopLeft + 2*cCurDist;
			int *pBufBottomLeft = pBufTopLeft + 2*cCurDist*cFieldWidth;

			int *pBufCurTop = pBufTopLeft, *pBufCurBottom = pBufBottomLeft;
			while (pBufCurTop <= pBufTopRigth) {
				if (_isMax(pBufCurTop, cFieldWidth)) {
					_result = _getPointByBufPointer(pBufCurTop, pBufCentre, cFieldWidth);
					return true;
				}

				if (_isMax(pBufCurBottom, cFieldWidth)) {
					_result = _getPointByBufPointer(pBufCurBottom, pBufCentre, cFieldWidth);
					return true;
				}

				++pBufCurTop;
				++pBufCurBottom;
			}

			int *pBufCurLeft = pBufTopLeft + cFieldWidth, *pBufCurRigth = pBufTopRigth + cFieldWidth;
			while (pBufCurLeft != pBufBottomLeft) {
				if (_isMax(pBufCurLeft, cFieldWidth)) {
					_result = _getPointByBufPointer(pBufCurLeft, pBufCentre, cFieldWidth);
					return true;
				}

				if (_isMax(pBufCurRigth, cFieldWidth)) {
					_result = _getPointByBufPointer(pBufCurRigth, pBufCentre, cFieldWidth);
					return true;
				}

				pBufCurLeft += cFieldWidth;
				pBufCurRigth += cFieldWidth;
			}
		}

		return false;
	}

private:
	void _fillNextSqueareContour(const Vector2D &_centre, size_t _cDistance, int *_pBufCentre, size_t _cRowSize) {
		int *pBufTopLeft = _pBufCentre - _cDistance - _cRowSize*_cDistance;
		int *pBufTopRigth = pBufTopLeft + 2*_cDistance;
		int *pBufBottomLeft = pBufTopLeft + 2*_cDistance*_cRowSize;

		const int *pDataTopLeft = getIntegralPointerTo(_centre - Vector2D(_cDistance, _cDistance));
		const int *pDataTopRigth = pDataTopLeft + _cDistance*2;
		const int *pDataBottomLeft = integral.moveByY(pDataTopLeft, _cDistance*2);

		const int *pDataCurTop = pDataTopLeft, *pDataCurBottom = pDataBottomLeft;
		int *pBufCurTop = pBufTopLeft, *pBufCurBottom = pBufBottomLeft;
		while (pDataCurTop <= pDataTopRigth) {
			*pBufCurTop = calcByTopLeft(pDataCurTop);
			*pBufCurBottom = calcByTopLeft(pDataCurBottom);
			++pBufCurTop;
			++pBufCurBottom;
			++pDataCurTop;
			++pDataCurBottom;
		}

		const int *pDataCurLeft = integral.nextRow(pDataTopLeft), *pDataCurRigth = integral.nextRow(pDataTopRigth);
		int *pBufCurLeft = pBufTopLeft + _cRowSize, *pBufCurRigth = pBufTopRigth + _cRowSize;

		while (pDataCurLeft != pDataBottomLeft) {
			*pBufCurLeft = calcByTopLeft(pDataCurLeft);
			*pBufCurRigth = calcByTopLeft(pDataCurRigth);
			pDataCurLeft = integral.nextRow(pDataCurLeft);
			pDataCurRigth = integral.nextRow(pDataCurRigth);
			pBufCurLeft += _cRowSize;
			pBufCurRigth += _cRowSize;
		}
	}

	static
	bool _isMax(const int *pVal, size_t _cRowSize) {
		int nCur = *pVal;
		const int *pNextRow = pVal + _cRowSize;
		const int *pPrevRow = pVal - _cRowSize;
		return nCur >= *(pVal - 1) && nCur >= *(pVal + 1) &&
			nCur >= *(pPrevRow - 1) && nCur >= *(pPrevRow) && nCur >= *(pPrevRow + 1) &&
			nCur >= *(pNextRow - 1) && nCur >= *(pNextRow) && nCur >= *(pNextRow + 1)
		;
	}

	static
	Vector2D _getPointByBufPointer(int *_pBufBegin, int *_pPoint, size_t _cRowSize) {
		assert(_pPoint >= _pBufBegin && _pPoint < _pBufBegin + _cRowSize*_cRowSize);
		const int nDelta = _pPoint - _pBufBegin;
		return Vector2D(nDelta%_cRowSize, nDelta/_cRowSize);
	}
};

#endif //_LEVEL_DATA_
