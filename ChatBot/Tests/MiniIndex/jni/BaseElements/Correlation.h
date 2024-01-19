#ifndef _CORRELATION_H_
#define _CORRELATION_H_

#include <math.h>
#include <memory.h>
#include <vector>

#include "Primitives2D.h"
#include "Interpolations.h"

template<typename T>
inline
float correlation(const T *_pFirst, const T *_pSecond, size_t _cLen) {
	const T *pEnd = _pFirst + _cLen;
	float fAverage1 = 0, fAverage2 = 0;
	const T *pVal2 = _pSecond;
	for (const T *pVal1 = _pFirst; pVal1 != pEnd; ++pVal1, ++pVal2) {
		fAverage1 += *pVal1;
		fAverage2 += *pVal2;
	}

	fAverage1 /= _cLen;
	fAverage2 /= _cLen;

	float fCov = 0;
	float fDisp1 = 0, fDisp2 = 0;
	pVal2 = _pSecond;
	for (const T *pVal1 = _pFirst; pVal1 != pEnd; ++pVal1, ++pVal2) {
		const float fDelta1 = float(*pVal1) - fAverage1;
		const float fDelta2 = float(*pVal2) - fAverage2;
		fCov += fDelta1*fDelta2;
		fDisp1 += fDelta1*fDelta1;
		fDisp2 += fDelta2*fDelta2;
	}

	const float fNormDiv = ::sqrt(fDisp1*fDisp2);

	if (fNormDiv == 0)
		return 0;

	return fCov/fNormDiv;
}

template<>
inline
float correlation(const float *_pFirst, const float *_pSecond, size_t _cLen) {
	const float *pEnd = _pFirst + _cLen;
	float fAverage1 = 0, fAverage2 = 0;
	for (const float *pVal1 = _pFirst, *pVal2 = _pSecond; pVal1 != pEnd; ++pVal1, ++pVal2) {
		fAverage1 += *pVal1;
		fAverage2 += *pVal2;
	}

	fAverage1 /= _cLen;
	fAverage2 /= _cLen;

	float fCov = 0;
	float fDisp1 = 0, fDisp2 = 0;
	for (const float *pVal1 = _pFirst, *pVal2 = _pSecond; pVal1 != pEnd; ++pVal1, ++pVal2) {
		const float fDelta1 = *pVal1 - fAverage1;
		const float fDelta2 = *pVal2 - fAverage2;
		fCov += fDelta1*fDelta2;
		fDisp1 += fDelta1*fDelta1;
		fDisp2 += fDelta2*fDelta2;
	}

	const float fNormDiv = ::sqrt(fDisp1*fDisp2);

	if (fNormDiv == 0)
		return 0;

	return fCov/fNormDiv;
}

inline
float correlation(const float *_pFirst, const int *_pSecond, size_t _cLen) {
	const float *pEnd = _pFirst + _cLen;
	float fAverage1 = 0, fAverage2 = 0;
	const int *pVal2 = _pSecond;
	for (const float *pVal1 = _pFirst; pVal1 != pEnd; ++pVal1, ++pVal2) {
		fAverage1 += *pVal1;
		fAverage2 += *pVal2;
	}

	fAverage1 /= _cLen;
	fAverage2 /= _cLen;

	float fCov = 0;
	float fDisp1 = 0, fDisp2 = 0;
	pVal2 = _pSecond;
	for (const float *pVal1 = _pFirst; pVal1 != pEnd; ++pVal1, ++pVal2) {
		const float fDelta1 = *pVal1 - fAverage1;
		const float fDelta2 = float(*pVal2) - fAverage2;
		fCov += fDelta1*fDelta2;
		fDisp1 += fDelta1*fDelta1;
		fDisp2 += fDelta2*fDelta2;
	}

	const float fNormDiv = ::sqrt(fDisp1*fDisp2);

	if (fNormDiv == 0)
		return 0;

	return fCov/fNormDiv;
}

template <typename T>
inline
float recalculateAndGetSD(T _sum, T *_pArray, T *_pEnd, size_t _cSize) {
	typedef typename CoordTraits<T>::MultTypeTrait MultType;
	MultType dispersion = 0;
	_sum /= _cSize;
	for (T *pVal = _pArray; pVal != _pEnd; ++pVal) {
		*pVal -= _sum;
		dispersion += MultType(*pVal)*(*pVal);
	}

	return ::sqrt(dispersion);
}


template <>
inline
float recalculateAndGetSD<int>(int _sum, int *_pArray, int *_pEnd, size_t _cSize) {
	typedef typename CoordTraits<int>::MultTypeTrait MultType;
	MultType dispersion = 0;
	_sum *= 4;
	_sum = _sum/int(_cSize);
	for (int *pVal = _pArray; pVal != _pEnd; ++pVal) {
		*pVal = *pVal*4 - _sum;
		const MultType dispersionDelta = MultType(*pVal)*(*pVal);
		assert(dispersion < std::numeric_limits<MultType>::max() - std::abs(dispersionDelta));
		dispersion += dispersionDelta;
	}

	assert(dispersion >= 0);
	return ::sqrt(float(dispersion));
}

template <typename T, size_t cSize>
class SolverData {
	float m_matrix[2][2];
	T m_pDerivatives[cSize];
	T m_pDistanceCoefficients[cSize];
	T *m_pCurDeriv;
	T *m_pCurDist;

	friend class Solver;
public:
	SolverData() : m_pCurDeriv(m_pDerivatives), m_pCurDist(m_pDistanceCoefficients) {
		m_matrix[0][0] = 0;
		m_matrix[1][1] = 0;
		m_matrix[0][1] = 0;
		m_matrix[1][0] = 0;
	}

	void disable() {
		m_pCurDeriv = m_pCurDist = nullptr;
	}

	bool disabled() const {
		return m_pCurDeriv == nullptr;
	}

	void addPoint(int _distanceFromMid2, T _derivative2) {
		assert(m_pCurDeriv < m_pDerivatives + cSize);
		assert(m_pCurDist < m_pDistanceCoefficients + cSize);

		const T distCoef = _derivative2*_distanceFromMid2;
		const float fDiagonal = (float(distCoef)*_derivative2)/2;// /8
		m_matrix[0][0] += float(_derivative2)*_derivative2;// /4
		m_matrix[1][1] += (float(distCoef)*distCoef)/4;// /16
		m_matrix[0][1] += fDiagonal;
		m_matrix[1][0] += fDiagonal;
		*m_pCurDeriv = _derivative2*2;//*4
		*m_pCurDist = distCoef;//*4

		++m_pCurDeriv;
		++m_pCurDist;
	}

	void resetPointers() {
		m_pCurDeriv = m_pDerivatives;
		m_pCurDist = m_pDistanceCoefficients;
	}
};

template <typename T, size_t cSize>
class ArrayToCorrelationCalc {
	typedef typename CoordTraits<T>::MultTypeTrait MultType;

	T m_pArray[cSize];
	T * m_pEnd;
	float m_fStandartDeviation;

	SolverData<T, cSize> data;

public:
	ArrayToCorrelationCalc(const ArrayToCorrelationCalc<T, cSize> &_other) : m_pEnd(m_pArray + cSize), m_fStandartDeviation(_other.m_fStandartDeviation), data(_other.data) {
		::memcpy(m_pArray, _other.m_pArray, sizeof(m_pArray));
		data.resetPointers();
	}

	ArrayToCorrelationCalc(bool _bUseSolverData = true) : m_pEnd(m_pArray + cSize),
		m_fStandartDeviation(std::numeric_limits<float>::max())
	{
		if (!_bUseSolverData)
			data.disable();
	}

	bool initialized() const {
		return m_fStandartDeviation != std::numeric_limits<float>::max();
	}

	const SolverData<T, cSize> &getSolverData() const {
		return data;
	}

	float getStandartDeviation() const {
		return m_fStandartDeviation;
	}

	void update() {
		T sum = 0;

		for (T *pVal = m_pArray; pVal != m_pEnd; ++pVal) {
			assert(sum < std::numeric_limits<T>::max() - std::abs(*pVal));
			sum += *pVal;
		}

		m_fStandartDeviation = recalculateAndGetSD<T>(sum, m_pArray, m_pEnd, cSize);
		assert(m_fStandartDeviation >= 0 && m_fStandartDeviation < std::numeric_limits<int>::max());

		if (data.disabled())
			return;

		T prev = *m_pArray;
		int nDistFromMid2 = -int(cSize);
		for (T *pVal = m_pArray; pVal != m_pEnd; ++pVal) {
			T *pNext = pVal + 1;
			T deriv = (pNext == m_pEnd ? *pVal : *pNext) - prev;
			data.addPoint(nDistFromMid2, deriv);
			prev = *pVal;
			nDistFromMid2 += 2;
		}
	}

	float calculate(const ArrayToCorrelationCalc &_other) const {
		assert(size() == _other.size());
		MultType covariance = 0;
		for (const T *pVal1 = m_pArray, *pVal2 = _other.m_pArray; pVal1 != m_pEnd; ++pVal1, ++pVal2) {
			const MultType deltaCov = MultType(*pVal1)*(*pVal2);
			assert(covariance < std::numeric_limits<MultType>::max() - std::abs(deltaCov));
			covariance += deltaCov;
		}
		return float(covariance)/(m_fStandartDeviation*_other.m_fStandartDeviation);
	}

	float calculateWithShift1(const ArrayToCorrelationCalc &_other) const {
		assert(size() == _other.size());
		MultType covariance = 0;
		const T *pVal1 = m_pArray, *pVal2 = _other.m_pArray;
		const MultType deltaCov = MultType(*pVal1)*(*pVal2);
		assert(covariance < std::numeric_limits<MultType>::max() - std::abs(deltaCov));
		covariance += deltaCov;
		++pVal1;

		for (; pVal1 != m_pEnd; ++pVal1, ++pVal2) {
			const MultType deltaCov = MultType(*pVal1)*(*pVal2);
			assert(covariance < std::numeric_limits<MultType>::max() - std::abs(deltaCov));
			covariance += deltaCov;
		}
		return float(covariance)/(m_fStandartDeviation*_other.m_fStandartDeviation);
	}

	float calculateWithZoom1(const ArrayToCorrelationCalc &_other) const {
		assert(size() == _other.size());
		MultType covariance = 0;
		const size_t cQuarter = cSize/4;
		const T *pFirstStretchedPoint = _other.m_pArray + cQuarter;
		const T *pSecondStretchedPoint = _other.m_pArray + cSize - cQuarter - 1;
		for (const T *pVal1 = m_pArray, *pVal2 = _other.m_pArray + 1; pVal1 != m_pEnd; ++pVal1) {
			const MultType deltaCov = MultType(*pVal1)*(*pVal2);
			assert(covariance < std::numeric_limits<MultType>::max() - std::abs(deltaCov));
			covariance += deltaCov;

			if (pVal2 == pFirstStretchedPoint)
				pFirstStretchedPoint = nullptr;
			else if (pVal2 == pSecondStretchedPoint)
				pSecondStretchedPoint = nullptr;
			else
				++pVal2;
		}
		return float(covariance)/(m_fStandartDeviation*_other.m_fStandartDeviation);
	}

	inline
	T *begin() {
		return m_pArray;
	}

	inline
	const T *begin() const {
		return m_pArray;
	}

	inline
	const T *end() const {
		return m_pEnd;
	}

	size_t size() const {
		return cSize;
	}

	inline
	T operator[] (size_t _cIdx) const {
		assert(_cIdx < cSize);
		return m_pArray[_cIdx];
	}
};

class HarmonicCalculator {
	public:
	static constexpr size_t ms_cArraySize = 128;
	static constexpr float ms_fStep = 2*M_PI/(ms_cArraySize - 1);
	float m_sinusArray[ms_cArraySize];
	float m_cosinusArray[ms_cArraySize];

	HarmonicCalculator() {
		float fAngle = -M_PI;
		for (size_t cIdx = 0; cIdx < ms_cArraySize; ++cIdx) {
			const float fSinValue = ::sin(fAngle);
			m_sinusArray[cIdx] = fSinValue;
			m_cosinusArray[cIdx >= 32 ? cIdx - 32 : cIdx + 96] = fSinValue;
			fAngle += ms_fStep;
		}
	}

public:
	static HarmonicCalculator &instance() {
		static HarmonicCalculator inst;
		return inst;
	}

	template<typename Iterator>
	Vector2DF getFourierCoef(Iterator _iBegin, const Iterator &_iEnd, float _fRadInStep, float _fStartAngle = 0) {
		float fStartIndex = 0;
		if (_fStartAngle != 0) {
			fStartIndex = _fStartAngle/ms_fStep;
			if (fStartIndex < -0.1)
				fStartIndex += ms_cArraySize;

			assert(fStartIndex >= -0.1 && fStartIndex < ms_cArraySize);
		}

		static std::vector<float> val1, val2, val3;
		val1.clear();
		val2.clear();
		val3.clear();
		const float fStep = _fRadInStep/ms_fStep;
		InterpolatedArrayIterator<float> iSinuses(m_sinusArray, ms_cArraySize, fStartIndex, fStep, ms_cArraySize);
		InterpolatedArrayIterator<float> iCosinuses(m_cosinusArray, ms_cArraySize, fStartIndex, fStep, ms_cArraySize);
		for (; _iBegin != _iEnd; ++_iBegin) {
			const float fCurValue = *_iBegin;
			val1.push_back(fCurValue);
			val2.push_back(*iSinuses);
			val3.push_back(*iCosinuses);

			++iSinuses;
			++iCosinuses;
			if (!iSinuses) {
				iSinuses.setCurIdx(iSinuses.getCurrentIdx() - ms_cArraySize);
				iCosinuses.setCurIdx(iCosinuses.getCurrentIdx() - ms_cArraySize);
			}
		}

//		CvPlot::clear("GradX1");
//		CvPlot::plot("GradX1", val1.data(), val1.size());
//		CvPlot::label("1");
//		CvPlot::plot("GradX1", val2.data(), val2.size());
//		CvPlot::label("2");
//		CvPlot::plot("GradX1", val3.data(), val3.size());
//		CvPlot::label("3");

		//return Vector2DF(fCosSum, fSinSum);
		return Vector2DF(correlation(&*val1.begin(), &*val3.begin(), val1.size()), correlation(&*val1.begin(), &*val2.begin(), val1.size()));
	}
};

#endif
