#ifndef _GRADIENT_H_
#define _GRADIENT_H_

#include <list>
#include <vector>
#include <memory>
#include <map>
#include <algorithm>

#include "BaseElements/Primitives2D.h"
#include "BaseElements/BresenhamIterator.h"
#include "BaseElements/Interpolations.h"
#include "BaseElements/Correlation.h"
#include "LevelData.h"
#include "Application.h"

class GradientArrayMinMax {
public:
	static const size_t ms_cArraySize = 96;

	Vector2D direction;
	Vector2D ort;
	size_t cGradCount;
	int gradArrayX[ms_cArraySize];

	float fGradXMinIdx;
	float fGradXMaxIdx;
	float fBrasenhamCoef;
	int nGradMin;
	int nGradMax;

	float fSinCorrelation;

	GradientArrayMinMax() {

	}

	void updateMinMax();

	int directionMaxCoordProjection() const {
		return std::max(std::abs(direction.x), std::abs(direction.y));
	}

	int ortMaxCoordProjection() const {
		return std::max(std::abs(ort.x), std::abs(ort.y));
	}

private:
	bool _detectProblems(size_t _cGradXMaxIdx, size_t _cGradXMinIdx);
	void _updateSinCorrelation(float _fBegin, float _fEnd, float _fRadsInStepOld);

	void _interpolateVals(size_t _cMax, size_t _cMin) {
		if (_cMax != 0 && _cMax != cGradCount - 1) {
			fGradXMaxIdx = splineInterpolate(gradArrayX[_cMax - 1], gradArrayX[_cMax], gradArrayX[_cMax + 1], _cMax - 1, 1);
		} else
			fGradXMaxIdx = _cMax;

		if (_cMin != 0 && _cMin != cGradCount - 1) {
			fGradXMinIdx = splineInterpolate(gradArrayX[_cMin - 1], gradArrayX[_cMin], gradArrayX[_cMin + 1], _cMin - 1, 1);
		} else
			fGradXMinIdx = _cMin;

		assert(fGradXMinIdx >= 0 && fGradXMinIdx < cGradCount && fGradXMaxIdx >= 0 && fGradXMaxIdx < cGradCount);
	}
};

class Gradient {
public:
	static const size_t ms_cArraySize = 32;
	static const size_t ms_cPixelsBufWidth = 30;
	static const size_t ms_cPixelsBufSize = ms_cPixelsBufWidth*ms_cPixelsBufWidth;

	Vector2DF directionF;
	Vector2DF ortF;

	ArrayToCorrelationCalc<int, ms_cArraySize> gradArrayX;
	ArrayToCorrelationCalc<int, ms_cArraySize> gradArrayY;
	ArrayToCorrelationCalc<int, ms_cArraySize> hessian;
	ArrayToCorrelationCalc<int, ms_cArraySize> ortArrayX;

	ArrayToCorrelationCalc<int, ms_cArraySize> gradArrayXLong;
	ArrayToCorrelationCalc<int, ms_cArraySize> gradArrayYLong;
	ArrayToCorrelationCalc<int, ms_cArraySize> hessianLong;
	ArrayToCorrelationCalc<int, ms_cArraySize> ortArrayXLong;

	float fGradLen;
	float fOrtLen;
	float fOrtZoom;
	float fSinCorrelation;
	float fBrasenhamCoef;
	float fOrtfBrasenhamCoef;
	float fCentreOffset;

	Vector2D minMaxDir;

	mutable ArrayToCorrelationCalc<int, ms_cPixelsBufSize> pixelsBuf;

	Gradient() : fGradLen(0), fSinCorrelation(0), pixelsBuf(false) {

	}

	void calcGradLen(const GradientArrayMinMax &_minMax) {
		fSinCorrelation = _minMax.fSinCorrelation;
		_updateBrasenhamCoef(_minMax);
		float fBegin = std::min(_minMax.fGradXMaxIdx, _minMax.fGradXMinIdx);
		float fEnd = std::max(_minMax.fGradXMaxIdx, _minMax.fGradXMinIdx);
		float fDelta = fEnd - fBegin;
		fGradLen = fDelta/fBrasenhamCoef;
		directionF = directionF.getNormalized()*fGradLen;
		const float fCentreIndex = (float(_minMax.cGradCount) - 1)/2;
		fCentreOffset = ((fBegin + fEnd)/2 - fCentreIndex)/fDelta;
		minMaxDir = _minMax.direction;

		fOrtLen = fGradLen*fOrtZoom;
		ortF = ortF.getNormalized()*fOrtLen;
	}

	void calcOrt(const class KeyPoint3D &_point, const GradientArrayMinMax &_minMax, const IntegralTransform &_integral, bool _bLongCompare) {
		assert(fGradLen != 0);
		_calculateArrayAlongOrt(_point, _integral, _bLongCompare);
	}

	void calcGrad(const class KeyPoint3D &_point, const GradientArrayMinMax &_minMax, const LevelData &_levelData, const IntegralTransform &_integral, bool _bLongCompare) {
		_calculateArrayAlongGrad(_point, _minMax, _levelData, _integral, _bLongCompare);
	}

	void update(const class KeyPoint3D &_point, const GradientArrayMinMax &_minMax, const LevelData &_levelData, const IntegralTransform &_integral) {
		calcGradLen(_minMax);
		_calculateArrayAlongGrad(_point, _minMax, _levelData, _integral, false);
		_calculateArrayAlongGrad(_point, _minMax, _levelData, _integral, true);
		_calculateArrayAlongOrt(_point, _integral, false);
		_calculateArrayAlongOrt(_point, _integral, true);
	}

	void fillPixelsBuffer(const byte *_pBuf, const Vector2D &_screenPoint) const;
private:
	void _calculateArrayAlongGrad(const class KeyPoint3D &_point, const GradientArrayMinMax &_minMax, const LevelData &_levelData,
		const IntegralTransform &_integral, bool _bLong)
	;
	void _calculateArrayAlongOrt(const KeyPoint3D &_point, const IntegralTransform &_integral, bool _bLong);
	void _calculateArrayAlongVector(const Vector2D &_begin, const Vector2D &_end, const Rect2D &_possibleRect, int _nHaarSize,
		int *_pArray, const int *_pEnd, const IntegralTransform &_integral, float _fStart
	);
	void _updateBrasenhamCoef(const GradientArrayMinMax &_minMax);
};

#endif //_GRADIENT_H_
