#ifndef _INTERPOLATIONS_H_
#define _INTERPOLATIONS_H_

#include "Application.h"

template<typename T>
class InterpolatedArrayIterator {
	const T *m_pArray;
	const T *m_pLast;
	size_t m_cSizeMinusOne;

	float m_fCurIdx;
	float m_fStep;
	float m_fEndIdx;

	inline
	float _interpolateValue(float _fAt) const {
		if (_fAt < 0)
			return _fAt <= -1 ? 0 : float(*m_pArray)*(_fAt + 1);

		const int nIndex = fast_floor(_fAt);
		if (_fAt >= m_cSizeMinusOne)
			return nIndex > m_cSizeMinusOne ? 0 : float(*m_pLast)*(_fAt - m_cSizeMinusOne);

		const float fCur = float(m_pArray[nIndex]);
		return fCur + (_fAt - nIndex)*(float(m_pArray[nIndex + 1]) - fCur);
	}

	//Fake object to test the end of iterations.
	InterpolatedArrayIterator() : m_pArray(nullptr), m_pLast(nullptr), m_cSizeMinusOne(0), m_fCurIdx(0), m_fStep(0), m_fEndIdx(0) {

	}

public:
	static const InterpolatedArrayIterator ms_end;

	InterpolatedArrayIterator(const T *_pArray, size_t _cSize, float _fFromIdx, float _fStep) : m_pArray(_pArray),
		m_pLast(_pArray + _cSize - 1), m_cSizeMinusOne(_cSize - 1), m_fCurIdx(_fFromIdx), m_fStep(_fStep),
		m_fEndIdx(_fStep > 0 ? std::numeric_limits<float>::max() : std::numeric_limits<float>::min())
	{
		assert(_cSize > 0);
	}

	InterpolatedArrayIterator(const T *_pArray, size_t _cSize, float _fFromIdx, float _fStep, float _fEnd) : m_pArray(_pArray),
		m_pLast(_pArray + _cSize - 1), m_cSizeMinusOne(_cSize - 1), m_fCurIdx(_fFromIdx), m_fStep(_fStep), m_fEndIdx(_fEnd)
	{
		assert(_cSize > 0);
	}

	void setCurIdx(float _fCurIdx) {
		m_fCurIdx = _fCurIdx;
	}

	float operator*() const {
		assert(*this != ms_end);
		return _interpolateValue(m_fCurIdx);
	}

	float next() const {
		return _interpolateValue(m_fCurIdx + m_fStep);
	}

	float prev() const {
		return _interpolateValue(m_fCurIdx - m_fStep);
	}

	void operator ++() {
		m_fCurIdx += m_fStep;
	}

	operator bool() const {
		return m_fStep > 0 ? m_fCurIdx < m_fEndIdx : m_fCurIdx > m_fEndIdx;
	}

	bool operator==(const InterpolatedArrayIterator &_other) const {
		assert(&_other == ms_end);
		return m_fStep > 0 ? m_fCurIdx >= m_fEndIdx : m_fCurIdx <= m_fEndIdx;
	}

	bool operator!=(const InterpolatedArrayIterator &_other) const {
		assert(&_other == &ms_end);
		return m_fStep > 0 ? m_fCurIdx < m_fEndIdx : m_fCurIdx > m_fEndIdx;
	}

	float getCurrentIdx() const {
		return m_fCurIdx;
	}
};

template<typename T>
const InterpolatedArrayIterator<T> InterpolatedArrayIterator<T>::ms_end;

inline
float splineInterpolate(float _fVal1, float _fVal2, float _fVal3, float _fStartX, float _fStepX) {
	const float fK3 = _fVal1;
	constexpr float fInverse00 = -1.0f;
	constexpr float fInverse10 = 0.5f;
	constexpr float fInverse01 = 2.0f;
	constexpr float fInverse11 = -0.5f;
	const float fIndependent1 = _fVal2 - fK3;
	const float fIndependent2 = _fVal3 - fK3;

	const float fK1 = fIndependent1*fInverse00 + fIndependent2*fInverse10;
	const float fK2 = fIndependent1*fInverse01 + fIndependent2*fInverse11;

	if (fK1 == 0)
		return _fStartX + _fStepX;

	const float fXExtr = -fK2/(2*fK1);
	if (fXExtr < 0 || fXExtr > 2)
		return _fStartX + _fStepX;

	return _fStartX + fXExtr*_fStepX;
}

inline
float getInterpolatedValueAt(const byte *_pBuf, float _fX, float _fY) {
	if (_fX < 0 || _fY < 0 || _fX > g_nWidth - 1 || _fY > g_nHeight - 1)
		return 0;

	int nX = fast_floor(_fX);
	int nY = fast_floor(_fY);

	float fValueFloorXY = *(_pBuf + nX + nY*g_nWidth);
	float fValueCeilXFloorY = nX == g_nWidth - 1 ? fValueFloorXY : *(_pBuf + nX + 1 + nY*g_nWidth);
	float fValueFloorY = fValueFloorXY + (fValueCeilXFloorY - fValueFloorXY)*(_fX - nX);

	if (nY == g_nHeight - 1)
		return fValueFloorY;

	float fValueFloorXCeilY = *(_pBuf + nX + (nY + 1)*g_nWidth);
	float fValueCeilXCeilY = nX == g_nWidth - 1 ? fValueFloorXCeilY : *(_pBuf + nX + 1 + (nY + 1)*g_nWidth);
	float fValueCeilY = fValueFloorXCeilY + (fValueCeilXCeilY - fValueFloorXCeilY)*(_fX - nX);

	return fValueFloorY + (fValueCeilY - fValueFloorY)*(_fY - nY);
}

#endif //_INTERPOLATIONS_H_
