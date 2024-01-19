#include "Gradient.h"
#include "KeyPoint.h"

bool GradientArrayMinMax::_detectProblems(size_t _cGradXMaxIdx, size_t _cGradXMinIdx) {
    const size_t cDelta = std::abs(int(_cGradXMaxIdx) - int(_cGradXMinIdx)) > 10 ? 5 : 3;

    const size_t cMaxEnd = std::min(_cGradXMaxIdx + cDelta, cGradCount);
    int nPrev = nGradMax;
    for (size_t c = _cGradXMaxIdx + 1; c < cMaxEnd; ++c) {
        if (gradArrayX[c] >= nPrev)
            return true;

        nPrev = gradArrayX[c];
    }

    const int nMaxBegin = std::max(int(_cGradXMaxIdx - cDelta), -1);
    nPrev = nGradMax;
    for (int c = int(_cGradXMaxIdx) - 1; c != nMaxBegin; --c) {
        if (gradArrayX[c] >= nPrev)
            return true;

        nPrev = gradArrayX[c];
    }

    const size_t cMinEnd = std::min(_cGradXMinIdx + cDelta, cGradCount);
    nPrev = nGradMin;
    for (size_t c = _cGradXMinIdx + 1; c < cMinEnd; ++c) {
        if (gradArrayX[c] <= nPrev)
            return true;

        nPrev = gradArrayX[c];
    }

    const int nMinBegin = std::max(int(_cGradXMinIdx - cDelta), -1);
    nPrev = nGradMin;
    for (int c = int(_cGradXMinIdx) - 1; c != nMinBegin; --c) {
        if (gradArrayX[c] <= nPrev)
            return true;

        nPrev = gradArrayX[c];
    }
    return false;
}

void GradientArrayMinMax::_updateSinCorrelation(float _fBegin, float _fEnd, float _fRadsInStepOld) {
    const float f90DegSteps = (_fEnd - _fBegin)/2;
    InterpolatedArrayIterator<int> iVals(gradArrayX, cGradCount, _fBegin - f90DegSteps, 1, _fEnd + f90DegSteps);
    const Vector2DF coef = HarmonicCalculator::instance().getFourierCoef(iVals, InterpolatedArrayIterator<int>::ms_end, _fRadsInStepOld, 0);
    fSinCorrelation = coef.y;
}

void GradientArrayMinMax::updateMinMax() {
    fBrasenhamCoef = float(directionMaxCoordProjection())/direction.length();

    const size_t cMiddle = cGradCount/2;
    nGradMax = gradArrayX[cMiddle];
    nGradMin = nGradMax;
    int nCurDelta = 0;
    size_t cGradXMaxIdx = cMiddle;
    size_t cGradXMinIdx = cMiddle;
    bool bMaxCommited = false;
    bool bMinCommited = false;
    size_t cPrevMaxIdx = cMiddle;
    size_t cPrevMinIdx = cMiddle;
    for (size_t cForward = cMiddle; cForward != cGradCount; ++cForward) {
        const int nCur = gradArrayX[cForward];
        if (nCur > nGradMax) {
            if (!bMaxCommited || nCur > nGradMax + nCurDelta) {
                nGradMax = nCur;
                cGradXMaxIdx = cForward;
                bMaxCommited = false;
            }
        } else {
            if (!bMaxCommited) {
                if (cForward > cGradXMaxIdx + 2) {
                    nCurDelta = std::max(nCurDelta, std::abs(nGradMax/8));
                    bMaxCommited = true;
                    cPrevMaxIdx = cGradXMaxIdx;
                }
            }
        }
        if (nCur < nGradMin) {
            if (!bMinCommited || nCur < nGradMin - nCurDelta) {
                nGradMin = nCur;
                cGradXMinIdx = cForward;
                bMinCommited = false;
            }
        } else {
            if (!bMinCommited) {
                if (cForward > cGradXMinIdx + 2) {
                    nCurDelta = std::max(nCurDelta, std::abs(nGradMin/8));
                    bMinCommited = true;
                    cPrevMinIdx = cGradXMinIdx;
                }
            }
        }
    }

    if (cGradXMaxIdx == cGradCount - 1) {
        cGradXMaxIdx = cPrevMaxIdx;
        nGradMax = gradArrayX[cGradXMaxIdx];
    }

    if (cGradXMinIdx == cGradCount - 1) {
        cGradXMinIdx = cPrevMinIdx;
        nGradMin = gradArrayX[cGradXMinIdx];
    }

    bMaxCommited = bMinCommited = false;
    size_t cMaxDelta = cGradXMaxIdx - cMiddle;
    size_t cMinDelta = cGradXMinIdx - cMiddle;

    for (size_t cBackward = cMiddle - 1; cBackward != size_t(-1); --cBackward) {
        const int nCur = gradArrayX[cBackward];
        if (nCur > nGradMax) {
            if (!bMaxCommited || nCur > nGradMax + nCurDelta) {
                nGradMax = nCur;
                cGradXMaxIdx = cBackward;
                bMaxCommited = false;
                cMaxDelta = 0;
            }
        } else {
            if (!bMaxCommited) {
                if (cMaxDelta != 0 && cMiddle > cMaxDelta + cBackward) {
                    bMaxCommited = true;
                    cPrevMaxIdx = cGradXMaxIdx;
                } else if (cMaxDelta == 0 && cBackward + 2 < cGradXMaxIdx) {
                    nCurDelta = std::max(nCurDelta, std::abs(nGradMax/8));
                    bMaxCommited = true;
                    cPrevMaxIdx = cGradXMaxIdx;
                }
            }
        }
        if (nCur < nGradMin || (cMinDelta != 0 && nCur < (nGradMin + nCurDelta))) {
            if (!bMinCommited || nCur < nGradMin - nCurDelta) {
                nGradMin = nCur;
                cGradXMinIdx = cBackward;
                bMinCommited = false;
                cMinDelta = 0;
            }
        }  	else {
            if (!bMinCommited) {
                if (cMinDelta != 0 && cMiddle - cBackward > cMinDelta) {
                    bMinCommited = true;
                    cPrevMinIdx = cGradXMinIdx;
                } else if (cMinDelta == 0) {
                    if (cBackward + 2 < cGradXMinIdx) {
                        nCurDelta = std::max(nCurDelta, std::abs(nGradMin/8));
                        bMinCommited = true;
                        cPrevMinIdx = cGradXMinIdx;
                    }
                }
            }
        }
    }

    if (cGradXMaxIdx == 0 || (!bMaxCommited && cPrevMaxIdx != cMiddle)) {
        cGradXMaxIdx = cPrevMaxIdx;
        nGradMax = gradArrayX[cGradXMaxIdx];
    }

    if (cGradXMinIdx == 0 || (!bMinCommited && cPrevMinIdx != cMiddle)) {
        cGradXMinIdx = cPrevMinIdx;
        nGradMin = gradArrayX[cGradXMinIdx];
    }

    if (nGradMin == nGradMax) {
        cGradCount = 0;
        return;
    }

    _interpolateVals(cGradXMaxIdx, cGradXMinIdx);

    const float fBeginOld = std::min(fGradXMaxIdx, fGradXMinIdx);
    const float fEndOld = std::max(fGradXMaxIdx, fGradXMinIdx);
    const float fRadsInStepOld = M_PI/(fEndOld - fBeginOld);

    if (!_detectProblems(cGradXMaxIdx, cGradXMinIdx)) {
        _updateSinCorrelation(fBeginOld, fEndOld, fRadsInStepOld);
        return;
    }

    float fMaxSize2 = 0;
    Vector2DF maxVec;
    float fMaxZoomDelta;
    const float fMinEdgeDistance = std::min(fBeginOld, float(cGradCount) - 1 - fEndOld);
    const float fDeltaX = (fEndOld - fBeginOld)/6;

    if ((fMinEdgeDistance - fDeltaX/3)*fRadsInStepOld < M_PI/3) {
        _updateSinCorrelation(fBeginOld, fEndOld, fRadsInStepOld);
        return;
    }

    const float fStepX = fDeltaX/10;

    for (float fDeltaZoom = -fDeltaX; fDeltaZoom <= fDeltaX; fDeltaZoom += fStepX) {
        const float fBegin = fBeginOld - fDeltaZoom;
        const float fEnd = fEndOld + fDeltaZoom;
        const float fRadsInStep = M_PI/(fEnd - fBegin);
        const float f90DegSteps = (fEnd - fBegin)/2;
        InterpolatedArrayIterator<int> iVals(gradArrayX, cGradCount, fBegin - f90DegSteps, 1, fEnd + f90DegSteps);
        Vector2DF coef = HarmonicCalculator::instance().getFourierCoef(iVals, InterpolatedArrayIterator<int>::ms_end, fRadsInStep);
        const float fSize2 = coef.length2();
        if (fSize2 > fMaxSize2) {
            maxVec = coef;
            fMaxSize2 = fSize2;
            fMaxZoomDelta = fDeltaZoom;
        }
    }

    //If result on edges.
    if (fMaxZoomDelta == -fDeltaX || fMaxZoomDelta + fStepX > fDeltaX) {
        _updateSinCorrelation(fBeginOld, fEndOld, fRadsInStepOld);
        return;
    }

    float fMinX = std::numeric_limits<float>::max();
    float fBestDeltaX;
    const float fBegin = fBeginOld - fMaxZoomDelta;
    const float fEnd = fEndOld + fMaxZoomDelta;
    const float fRadsInStep = M_PI/(fEnd - fBegin);
    const float f90DegSteps = (fEnd - fBegin)/2;
    for (float fDeltaByX = -fDeltaX; fDeltaByX <= fDeltaX; fDeltaByX += fStepX) {
        const float fStartAngle = -fDeltaByX*fRadsInStep;
        InterpolatedArrayIterator<int> iVals(gradArrayX, cGradCount, fBegin - f90DegSteps, 1, fEnd + f90DegSteps);
        Vector2DF coef = HarmonicCalculator::instance().getFourierCoef(iVals, InterpolatedArrayIterator<int>::ms_end, fRadsInStep, fStartAngle);
        if (std::abs(coef.x) < fMinX) {
            maxVec = coef;
            fMinX = std::abs(coef.x);
            fBestDeltaX = fDeltaByX;
        }
    }

    //If result on edges.
    if (fBestDeltaX == -fDeltaX || fMaxZoomDelta + fStepX > fDeltaX) {
        _updateSinCorrelation(fBeginOld, fEndOld, fRadsInStepOld);
        return;
    }

    bool bBySinForm = fGradXMaxIdx > fGradXMinIdx;
    fGradXMaxIdx = fGradXMaxIdx + (bBySinForm ? fBestDeltaX + fMaxZoomDelta : fBestDeltaX - fMaxZoomDelta);
    fGradXMinIdx = fGradXMinIdx + (bBySinForm ? fBestDeltaX - fMaxZoomDelta : fBestDeltaX + fMaxZoomDelta);
    fSinCorrelation = maxVec.y;
}

void Gradient::_calculateArrayAlongGrad(const KeyPoint3D &_point, const GradientArrayMinMax &_minMax, const LevelData &_levelData,
    const IntegralTransform &_integral, bool _bLong)
{
//	static int nCompare = 0;
//	std::stringstream stream1;
//	stream1 << "/home/vfedosov/temp/KeyPoints/point_graph_calc" << nCompare++ << ".jpg";
//	saveDiagramm(stream1.str().data(), const_cast<int *>(_minMax.gradArrayX), _minMax.cGradCount, _minMax.fGradXMinIdx, _minMax.fGradXMaxIdx);

    int *pArrayX = _bLong ? gradArrayXLong.begin() : gradArrayX.begin();
    int *pArrayY = _bLong ? gradArrayYLong.begin() : gradArrayY.begin();
    int *pHessians = _bLong ? hessianLong.begin() : hessian.begin();

    const bool bOrtEmpty = ortF.empty();
    const int nHaarSize = (_point.nSize + 2)/4;
    float fBegin = std::min(_minMax.fGradXMaxIdx, _minMax.fGradXMinIdx);
    float fEnd = std::max(_minMax.fGradXMaxIdx, _minMax.fGradXMinIdx);
    float fDelta = fEnd - fBegin;

	if (!_bLong)
        fDelta *= 0.5;
	else
        fDelta *= 2.5f;

    fBegin -= fDelta;
    fEnd += fDelta;

    BresenhamIterator iPoints = BresenhamIterator::create(_point.screenPoint - _minMax.direction, _point.screenPoint + _minMax.direction);
    const int nStepsNeed = fast_floor(fBegin);
    iPoints.passSteps(nStepsNeed);
    iPoints.setUnlimited();
    FloatStepIterator iFloat(iPoints, fBegin - nStepsNeed, (fEnd - fBegin)/float(Gradient::ms_cArraySize - 1));
    Rect2D allowedRect = _levelData.allowedRect;
    allowedRect.inflate(1);
    const Vector2D &direction = _minMax.direction;
    const Vector2D &ort = _minMax.ort;
    const int *pEndX = _bLong ? gradArrayXLong.end() : gradArrayX.end();
    int nVal1 = 0, nVal2 = 0, nValY1 = 0, nValY2 = 0;
    while(pArrayX != pEndX) {
        const Vector2D curPoint = iPoints.point();
        if (allowedRect.contains(curPoint)) {
            if (iFloat.needFirst()) {
                const Vector2D calculated = _integral.calcGradVector(iFloat.firstPoint(), nHaarSize);
                nVal1 = calculated.x*direction.x + calculated.y*direction.y;
                if (!bOrtEmpty)
                    nValY1 = calculated.x*ort.x + calculated.y*ort.y;
            }
            if (iFloat.needSecond()) {
                const Vector2D calculated = _integral.calcGradVector(iFloat.secondPoint(), nHaarSize);
                nVal2 = calculated.x*direction.x + calculated.y*direction.y;
                if (!bOrtEmpty)
                    nValY2 = calculated.x*ort.x + calculated.y*ort.y;
            }

            *pArrayX = iFloat.calculate(nVal1, nVal2);

            if (bOrtEmpty)
                *pArrayY = 0;
            else
                *pArrayY = iFloat.calculate(nValY1, nValY2);

            *pHessians = iFloat.calculate(iFloat.needFirst() ? _levelData.getValueAtPoint(iFloat.firstPoint(), _integral) : 0,
                iFloat.needSecond() ? _levelData.getValueAtPoint(iFloat.secondPoint(), _integral) : 0
            );
        } else {
            *pArrayX = 0;
            *pArrayY = 0;
            *pHessians = 0;
        }

        ++pArrayX;
        ++pArrayY;
        ++pHessians;
        ++iFloat;
    }

    if (_bLong) {
        gradArrayXLong.update();
        gradArrayYLong.update();
        hessianLong.update();
    } else {
        gradArrayX.update();
        gradArrayY.update();
        hessian.update();
    }
}


void Gradient::_calculateArrayAlongVector(const Vector2D &_begin, const Vector2D &_end, const Rect2D &_possibleRect, int _nHaarSize,
    int *_pArray, const int *_pEnd, const IntegralTransform &_integral, float _fStart
) {
    const Vector2D delta = _end - _begin;
    BresenhamIterator iPoints = BresenhamIterator::create(_begin, _end);
    const float fRange = float(iPoints.getStepsCount()) - 2*_fStart;
    const int nStepsNeed = fast_floor(_fStart);
    iPoints.passSteps(nStepsNeed);
    FloatStepIterator iFloat(iPoints, _fStart - nStepsNeed, fRange/float(Gradient::ms_cArraySize - 1));
    iPoints.setUnlimited();

    while(_pArray != _pEnd) {
        if (_possibleRect.contains(iPoints.point())) {
            int nVal1, nVal2;
            if (iFloat.needFirst()) {
                const Vector2D vec = _integral.calcGradVector(iFloat.firstPoint(), _nHaarSize);
                nVal1 = vec.x*delta.x + vec.y*delta.y;
            }
            if (iFloat.needSecond()) {
                const Vector2D vec = _integral.calcGradVector(iFloat.secondPoint(), _nHaarSize);
                nVal2 = vec.x*delta.x + vec.y*delta.y;
            }

            *(_pArray++) = iFloat.calculate(nVal1, nVal2);
        } else
            *(_pArray++) = 0;

        ++iFloat;
    }
}


void Gradient::_calculateArrayAlongOrt(const KeyPoint3D &_point, const IntegralTransform &_integral, bool _bLong) {
    const int nHaarSize = (_point.nSize + 2)/4;
	const Rect2D possibleRect(nHaarSize + 1, nHaarSize + 1, g_nWidth - nHaarSize - 1, g_nHeight - nHaarSize - 1);
    float fStart;
    Vector2D delta;

    if (_bLong) {
        const Vector2DF longOrt = ortF*3;
        delta = longOrt.toInt();
        fStart = std::abs(delta.x) >= std::abs(delta.y) ?  std::abs(delta.x) - std::abs(longOrt.x) : std::abs(delta.y) - std::abs(longOrt.y);
    } else {
        delta = ortF.toInt();
        fStart = std::abs(delta.x) >= std::abs(delta.y) ?  std::abs(delta.x) - std::abs(ortF.x) : std::abs(delta.y) - std::abs(ortF.y);
    }

    _calculateArrayAlongVector(_point.screenPoint - delta, _point.screenPoint + delta, possibleRect,
        nHaarSize, _bLong ? ortArrayXLong.begin() : ortArrayX.begin(), _bLong ? ortArrayXLong.end() : ortArrayX.end(), _integral, fStart
    );

    if (_bLong)
        ortArrayXLong.update();
    else
        ortArrayX.update();
}

void Gradient::_updateBrasenhamCoef(const GradientArrayMinMax &_minMax) {
    fBrasenhamCoef = _minMax.fBrasenhamCoef;
    if (fOrtZoom != 0)
        fOrtfBrasenhamCoef = float(_minMax.ortMaxCoordProjection())/_minMax.ort.length();
    else
        fOrtfBrasenhamCoef = 1;
}

void Gradient::fillPixelsBuffer(const byte *_pBuf, const Vector2D &_screenPoint) const {
    const Vector2DF grad = directionF;
    const Vector2DF ort = ortF.getNormalized()*fOrtLen;
    const Vector2DF bottomLeft = Vector2DF(_screenPoint) - grad - ort;
    const float fStepCoef = 2.9f/(ms_cPixelsBufWidth - 1);
    const Vector2DF gradStep = grad*fStepCoef;
    const Vector2DF ortStep = ort*fStepCoef;

    int *pCur = pixelsBuf.begin();

    for (size_t j = 0; j < ms_cPixelsBufWidth; ++j) {
        Vector2DF curPos = bottomLeft + ortStep*j;
        for (size_t i = 0; i < ms_cPixelsBufWidth; ++i) {
            const float fVal = getInterpolatedValueAt(_pBuf, curPos.x, curPos.y)*8;
            assert(fVal < std::numeric_limits<int>::max());
            *pCur = fast_floor(fVal);

            ++pCur;
            curPos += gradStep;
        }
    }

    assert(pCur == pixelsBuf.end());
    pixelsBuf.update();
}
