#ifndef _INTEGRAL_TRANSFORM_
#define _INTEGRAL_TRANSFORM_

#include "BaseElements/Colors.h"

class IntegralTransform {
	int m_nStep;
	int m_nRowSize;
	int m_nRowsCount;
	int m_nSize;

	int *m_pData;
	int *m_pEnd;

	Rect2D m_rect;

public:
	IntegralTransform() : m_pData(nullptr), m_pEnd(nullptr), m_rect(Application::getScreenRect()) {

	}

	IntegralTransform(IntegralTransform &&_other) : m_nStep(_other.m_nStep), m_nRowSize(_other.m_nRowSize), m_nRowsCount(_other.m_nRowsCount),
		m_nSize(_other.m_nSize), m_pData(_other.m_pData), m_pEnd(_other.m_pEnd), m_rect(_other.m_rect)
	{
		_other.m_pData = nullptr;
		_other.m_pEnd = nullptr;
	}

	IntegralTransform(unsigned char *_pBuf) : m_nStep(1), m_nRowSize(g_nWidth/m_nStep), m_nRowsCount(g_nHeight/m_nStep),
		m_nSize(m_nRowSize*m_nRowsCount), m_pData(new int[m_nSize]), m_rect(Application::getScreenRect())
	{
		int *pData = m_pData;
		unsigned char *pBuf = _pBuf;
		int *pRowEnd = m_pData + m_nRowSize;
		*pData = *pBuf;
		++pData;
		++pBuf;

		for (; pData != pRowEnd; ++ pData) {
			const int nCurVal = *pBuf;
			*pData = *(pData - 1) + nCurVal;
			++pBuf;
		}

		m_pEnd = m_pData + m_nSize;

		while (pData != m_pEnd) {
			pRowEnd += m_nRowSize;
			*pData = int(*pBuf) + *(pData - m_nRowSize);
			++pData;
			++pBuf;

			for (; pData != pRowEnd; ++pData, ++pBuf)
				*pData = int(*pBuf) - *(pData - m_nRowSize - 1) + *(pData - m_nRowSize) + *(pData - 1);
		}
	}

	IntegralTransform(unsigned char *_pBuf, int _nStartX, int _nStartY, int _nWidth, int _nHeight, int _nSourceBufRowSize) : m_nStep(1), m_nRowSize(_nWidth/m_nStep), m_nRowsCount(_nHeight/m_nStep),
		m_nSize(m_nRowSize*m_nRowsCount), m_pData(new int[m_nSize]), m_rect(_nStartX, _nStartY, _nStartX + _nWidth, _nStartY + _nHeight)
	{
		int *pData = m_pData;
		unsigned char *pBuf = _pBuf + _nStartY*_nSourceBufRowSize + _nStartX;
		int *pRowEnd = m_pData + _nWidth;
		*pData = *pBuf;
		++pData;
		++pBuf;

		for (; pData != pRowEnd; ++ pData) {
			const int nCurVal = *pBuf;
			*pData = *(pData - 1) + nCurVal;
			++pBuf;
		}

		m_pEnd = m_pData + m_nSize;
		const int nRowsDelta = _nSourceBufRowSize - _nWidth;

		while (pData != m_pEnd) {
			pRowEnd += m_nRowSize;
			pBuf += nRowsDelta;
			*pData = int(*pBuf) + *(pData - m_nRowSize);
			++pData;
			++pBuf;

			for (; pData != pRowEnd; ++pData, ++pBuf)
				*pData = int(*pBuf) - *(pData - m_nRowSize - 1) + *(pData - m_nRowSize) + *(pData - 1);
		}
	}

	~IntegralTransform() {
		delete[] m_pData;
	}

	const Rect2D &getRect() const {
		return m_rect;
	}

	void swap(IntegralTransform &_other) {
		std::swap(m_nStep, _other.m_nStep);
		std::swap(m_nRowSize, _other.m_nRowSize);
		std::swap(m_nRowsCount, _other.m_nRowsCount);
		std::swap(m_nSize, _other.m_nSize);
		std::swap(m_pData, _other.m_pData);
		std::swap(m_pEnd, _other.m_pEnd);
		std::swap(m_rect, _other.m_rect);
	}

	const int *begin() const {
		return m_pData;
	}

	int getWidth() const {
		return m_nRowSize;
	}

	int getHeight() const {
		return m_nRowsCount;
	}

	int getSize() const {
		return m_nSize;
	}

	const int *end() const {
		return m_pEnd;
	}

	inline
	const int *getPointerTo(const Vector2D &_point) const  {
		const int *pVal =  m_pData + _point.x + _point.y*getWidth();
		assert(pVal >= m_pData && pVal <= m_pEnd);
		return pVal;
	}

	inline
	const int *getPointerTo(int _nX, int _nY) const  {
		const int *pVal =  m_pData + _nX + _nY*getWidth();
		assert(pVal >= m_pData && pVal <= m_pEnd);
		return pVal;
	}

	inline
	const int *nextRow(const int *_pCur) const {
		const int * pRes = _pCur + getWidth();
		assert(pRes <= m_pEnd);
		return pRes;
	}

	inline
	const int *prevRow(const int *_pCur) const {
		const int * pRes = _pCur - getWidth();
		assert(pRes >= m_pData);
		return pRes;
	}

	inline
	const int *moveByY(const int *_pCur, int _nRows) const {
		const int * pRes = _pCur + _nRows*getWidth();
		assert(pRes >= m_pData && pRes <= m_pEnd);
		return pRes;
	}

	int calcHorHaar(const Vector2D &_point, int _nStep) const {
		assert(_point.x >= _nStep && _point.y >= _nStep && _point.x < getWidth() - _nStep && _point.y < getHeight() - _nStep);
		const int *pTopLeft = begin() + _point.x - _nStep + (_point.y - _nStep)*getWidth();
		const int *pMiddleLeft = pTopLeft + _nStep*getWidth();
		const int *pMiddleRigth = pMiddleLeft + _nStep*2;
		const int nNegativeHalf = *pMiddleRigth - *pMiddleLeft - *(pTopLeft + 2*_nStep) + *pTopLeft;
		const int *pBottomLeft = pMiddleLeft + _nStep*getWidth();
		const int *pBottomRigth = pBottomLeft + _nStep*2;
		const int nPositiveHalf = *pBottomRigth - *pBottomLeft - *pMiddleRigth + *pMiddleLeft;
		return nPositiveHalf - nNegativeHalf;
	}

	int calcVertHaar(const Vector2D &_point, int _nStep) const {
		assert(_point.x >= _nStep && _point.y >= _nStep && _point.x < getWidth() - _nStep && _point.y < getHeight() - _nStep);
		const int *pTopLeft = begin() + _point.x - _nStep + (_point.y - _nStep)*getWidth();
		const int *pTopMiddle = pTopLeft + _nStep;
		const int *pTopRigth = pTopMiddle + _nStep;
		const int *pBottomLeft = pTopLeft + 2*_nStep*getWidth();
		const int *pBottomMiddle = pBottomLeft + _nStep;
		const int *pBottomRigth = pBottomMiddle + _nStep;
		const int nNegativeHalf = *pBottomMiddle - *pTopMiddle - *pBottomLeft + *pTopLeft;
		const int nPositiveHalf = *pBottomRigth - *pBottomMiddle - *pTopRigth + *pTopMiddle;
		return nPositiveHalf - nNegativeHalf;
	}

	Vector2D calcGradVector(const Vector2D &_point, int _nStep) const {
		return Vector2D(calcVertHaar(_point, _nStep), calcHorHaar(_point, _nStep));
	}
};

#endif //_INTEGRAL_TRANSFORM_
