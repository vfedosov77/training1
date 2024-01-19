#include "PointsIterators.h"
#include "Grid.h"

namespace {
static constexpr int g_nFarSearchDistance = 57;
static constexpr int g_nNearDistance = 25;
}

template<class DataProvider, size_t cStepShift>
class LevelMaximumsIterator : public NearestPointsIterator<cStepShift> {
	DataProvider &m_provider;
	const size_t m_cBufYShift;
	const size_t m_cRowSize;
	const size_t m_cBufSize;
	int *const m_pBuf;
	const int *const m_pEnd;
	int m_nCountToSkip;
	Vector2D m_topLeft;
	int m_nCurrentDelta;
	Vector2D m_currentDeltaVec;
	int m_nCurValue;
	int m_nThreshold;

	using NearestPointsIterator<cStepShift>::m_cCurMove;
	using NearestPointsIterator<cStepShift>::m_current;
	using NearestPointsIterator<cStepShift>::m_bStateChanged;
	using NearestPointsIterator<cStepShift>::m_bGap;
	using NearestPointsIterator<cStepShift>::m_nStep;

	void _updateDelta() {
		switch(m_cCurMove) {
		case NearestPointsIterator<cStepShift>::mTopLeftToRigth:
			m_nCurrentDelta = m_cRowSize - 1;
			m_currentDeltaVec = Vector2D(-m_nStep, m_nStep);
			return;
		case NearestPointsIterator<cStepShift>::mTopRigthToBottom:
			m_nCurrentDelta = -1 - m_cRowSize;
			m_currentDeltaVec = Vector2D(-m_nStep, -m_nStep);
			return;
		case NearestPointsIterator<cStepShift>::mBottomRigthToLeft:
			m_nCurrentDelta = -int(m_cRowSize) + 1;
			m_currentDeltaVec = Vector2D(m_nStep, -m_nStep);
			return;
		case NearestPointsIterator<cStepShift>::mBottomLeftToTop:
			m_nCurrentDelta = 1 + m_cRowSize;
			m_currentDeltaVec = Vector2D(m_nStep, m_nStep);
			return;
		default:
			assert(false);
		}
	}

	static
	int _getNearestBiggerPow2Shilft(int _nVal) {
		assert(_nVal > 16 && _nVal < 128);
		if (_nVal < 32)
			return 5;
		if (_nVal < 64)
			return 6;
		return 7;
	}

	inline
	int *_getCurTestPointer() {
		int *pVal = _getCurWritePointer() + m_nCurrentDelta;
		assert(pVal >= m_pBuf && pVal < m_pEnd);
		return pVal;
	}

	inline
	int *_getCurWritePointer() {
		int *pVal = m_pBuf + ((m_current.x - m_topLeft.x)>>cStepShift) + ((m_current.y - m_topLeft.y)<<m_cBufYShift);
		assert(pVal >= m_pBuf && pVal < m_pEnd);
		return pVal;
	}

	inline
	void setCurValue(int _nVal) {
		int *pVal = _getCurWritePointer();
		assert(pVal >= m_pBuf && pVal < m_pEnd);
		*pVal = _nVal;
	}

	bool _test() {
		if (m_nCountToSkip != 0)
			return false;

		int *pCur = _getCurTestPointer();

		if (*pCur < m_nThreshold)
			return false;

		if (*pCur <= *(pCur + 1) || *pCur <= *(pCur - 1))
			return false;


		int *pPrevRow = pCur - m_cRowSize - 1;
		assert(pPrevRow >= m_pBuf && pPrevRow < m_pEnd);

		if (*pCur <= *(pPrevRow + 2) || *pCur <= *(pPrevRow + 1) || *pCur <= *(pPrevRow))
			return false;

		int *pNextRow = pCur + m_cRowSize - 1;
		assert(pNextRow >= m_pBuf && pNextRow < m_pEnd);

		if (*pCur <= *(pNextRow + 2) || *pCur <= *(pNextRow + 1) || *pCur <= *(pNextRow))
			return false;

		m_nCurValue = *pCur;

//		int nHalfSize = m_provider.pCurrentLevelData->constants.nScreenEdgeMinDistance;
//		const int *pInt = m_provider.m_hessian.getIntegral().getPointerTo(current() - Vector2D(nHalfSize, nHalfSize));
//		assert(m_provider.pCurrentLevelData->constants.calculate(pInt, 0) == m_nCurValue);

		return true;
	}

public:
	LevelMaximumsIterator(DataProvider &_provider, const Vector2D &_centre, size_t _cSearchDistance, const Rect2D &_rect, int _nThreshold) : NearestPointsIterator<cStepShift>(_centre, _cSearchDistance, _rect),
		m_provider(_provider), m_cBufYShift(_getNearestBiggerPow2Shilft(2*_cSearchDistance) - 2*cStepShift),
		m_cRowSize(1<<(m_cBufYShift + cStepShift)), m_cBufSize(m_cRowSize*m_cRowSize),
		m_pBuf(new int[m_cBufSize]), m_pEnd(m_pBuf + m_cBufSize), m_nCountToSkip(0), m_nCurrentDelta(m_cRowSize + 1),
		m_currentDeltaVec(m_nStep, m_nStep), m_nThreshold(_nThreshold)
	{
		Rect2D rect = _rect;
		rect.inflate(m_nStep);

		if (!rect.contains(_centre)) {
			NearestPointsIterator<cStepShift>::m_bHasPoints = false;
			return;
		}

		const int nHalfWidth = m_cRowSize*(1<<(cStepShift - 1));
		m_topLeft = _centre - Vector2D(nHalfWidth, nHalfWidth);

		for (size_t c = 0; c != 8; ++c) {
			setCurValue(m_provider.getValue(m_current.x, m_current.y));
			const bool bRes = NearestPointsIterator<cStepShift>::moveNext();
			assert(bRes);
		}

		setCurValue(m_provider.getValue(m_current.x, m_current.y));

		if (_test()) {
			m_nCountToSkip = 3;
			return;
		}

		m_nCountToSkip = 3;
		moveNext();
	}

	~LevelMaximumsIterator() {
		delete[] m_pBuf;
	}

	bool moveNext() {
		assert(!NearestPointsIterator<cStepShift>::eof());
		bool bRes = false;
		do {
			NearestPointsIterator<cStepShift>::resetStateChanged();
			//size_t cPrevDistance = NearestPointsIterator<cStepShift>::getDistance();
			if (!NearestPointsIterator<cStepShift>::moveNext())
				return false;

			setCurValue(m_provider.getValue(m_current.x, m_current.y));

			if (m_bStateChanged)
				_updateDelta();

			if (m_bGap)
				m_nCountToSkip = 3;

			if (m_current == NearestPointsIterator<cStepShift>::m_nextCorner) {
				if (m_cCurMove == NearestPointsIterator<cStepShift>::mBottomLeftToTop) {
					bRes = _test();
					m_nCountToSkip = 3;
					continue;
				}
				m_nCountToSkip = /* cPrevDistance == NearestPointsIterator<cStepShift>::getDistance() ? 2 :*/ 2;
			} else if (m_nCountToSkip != 0)
				--m_nCountToSkip;

			bRes = _test();
		} while(!bRes);

		return bRes;
	}

	Vector2D current() const {
		assert(!NearestPointsIterator<cStepShift>::eof());
		return m_current + m_currentDeltaVec;
	}

	int currentValue() const {
		return m_nCurValue;
	}
};

void levelMaximumsIteratorTest() {
	class TestDataProvider {
	public:
		int getValue(int _nX, int _nY) {
			if (_nX%4 == 0 && _nY%4 == 0)
				return 2;
			Vector2D cur(_nX, _nY);
			Vector2D nearest((_nX + 2)&(~3), (_nY + 2)&(~3));
			assert(nearest.distance2(cur) <= 8);
			return nearest.distance2(cur) < 5 ? 1 : 0;
		}
	} provider;

	Rect2D rect(20, 20, 60, 60);
	Vector2D centre(30, 30);
	NearestPointsIterator<1> it(centre, 40, rect);

	std::vector<Vector2D> vec0;
	std::set<Vector2D> points;
	while (!it.eof()) {
		assert(points.find(it.currentPoint()) == points.end());
		assert(rect.contains(it.currentPoint()));
		points.insert(it.currentPoint());
		vec0.push_back(it.currentPoint());
		it.resetStateChanged();
		it.moveNext();
	}

	assert(points.size() == size_t(rect.width()*rect.height()/4));

	LevelMaximumsIterator<TestDataProvider, 1> iterator(provider, centre, 40, rect, 1);
	std::set<Vector2D> results;
	std::vector<Vector2D> vec;
	while (!iterator.eof()) {
		assert(iterator.current().x%4 == 0 && iterator.current().y%4 == 0);
		assert(results.find(iterator.current()) == results.end());
		results.insert(iterator.current());
		vec.push_back(iterator.current());
		iterator.moveNext();
	}
	assert(results.size() == 81);
}


NearestKeyPointsIterator::NearestKeyPointsIterator(const KeyPoint3D &_basePoint, Vector2D &_prevPicturePoint, HessianTransform &_hessian, bool _bUseLevels,
	bool _bFarSearch) : m_provider(_hessian, _basePoint.nLevelId), m_pLevelIt(nullptr),
	m_basePoint(_basePoint), m_prevPicturePoint(_prevPicturePoint), m_hessian(_hessian), m_bUseLevels(_bUseLevels), m_iCurrent(m_result.begin()),
	m_rememberedPosition(0), m_bFarSearch(_bFarSearch)
{
	m_nLevelId = m_nCurLevelId = _basePoint.nLastMatchLevelId == -1 ? _basePoint.nLevelId : _basePoint.nLastMatchLevelId;
	m_provider.setCurLevel(m_nCurLevelId);
	_createLevelIterator();
	_addNext();
}

NearestKeyPointsIterator::~NearestKeyPointsIterator() {
	delete m_pLevelIt;
}

void NearestKeyPointsIterator::operator ++ () {
	assert(m_iCurrent != m_result.end() || !m_pLevelIt->eof());
	++m_iCurrent;
	if (m_iCurrent == m_result.end()) {
		if (!m_pLevelIt->eof())
			m_pLevelIt->moveNext();

		_addNext();
	}
}

void NearestKeyPointsIterator::_createLevelIterator() {
	delete m_pLevelIt;

	const int nThreshold = std::max(g_nDynamicThreshol, m_basePoint.nValue/6);
	if (m_bFarSearch)
		m_pLevelIt = new MaximumIterator(m_provider, m_prevPicturePoint, g_nFarSearchDistance, m_provider.allowedRect, nThreshold);
	else
		m_pLevelIt = new MaximumIterator(m_provider, m_prevPicturePoint, g_nNearDistance, m_provider.allowedRect, nThreshold);
}

bool NearestKeyPointsIterator::_testPoint(const Vector2D &_point, int _nValue, KeyPoint &_kp) {
	const LevelData &level = *m_provider.pCurrentLevelData;
	const int *pIntVal = m_hessian.getIntegral().getPointerTo(Vector2D(_point.x - level.nScreenEdgeMinDistance, _point.y - level.nScreenEdgeMinDistance));
	if (level.constants.calcSign(pIntVal) != m_basePoint.bHessianSign)
		return false;
	//TODO: Test if better to calculate without grid?
	std::vector<DynamicGridWithThreshold::GridCell> *pPrevCells = m_provider.m_pPrevCells;
	std::vector<DynamicGridWithThreshold::GridCell> *pNextCells = m_provider.m_pNextCells;
	DynamicGridWithThreshold &grid = m_provider.m_grid;
	for (int dy = -1; dy <= 1; ++dy) {
		for (int dx = -1; dx <= 1; ++dx) {
			if (pPrevCells != nullptr) {
				const int nVal = grid.getValue(*pPrevCells, _point.x + dx, _point.y + dy);
				if (nVal > _nValue)
					return false;
			}

			if (pNextCells != nullptr) {
				const int nVal = grid.getValue(*pNextCells, _point.x + dx, _point.y + dy);
				if (nVal > _nValue)
					return false;
			}
		}
	}

	Vector2D maxPoint = _point;
	int nMaxVal = _nValue;
	size_t cPointNum = 0;
	for (int nCurY = _point.y - 1 - level.nScreenEdgeMinDistance, nEndY = nCurY + 3; nCurY != nEndY; ++nCurY) {
		for (int nCurX = _point.x - 1 - level.nScreenEdgeMinDistance, nEndX = nCurX + 3; nCurX != nEndX; ++nCurX) {
			if (cPointNum++ == 4)
				continue;

			const int nVal = level.constants.calculate(m_hessian.getIntegral().getPointerTo(nCurX, nCurY));
			if (nVal > nMaxVal) {
				nMaxVal = nVal;
				maxPoint = Vector2D(nCurX + level.nScreenEdgeMinDistance, nCurY + level.nScreenEdgeMinDistance);
			}
		}
	}

	_kp.point = maxPoint;
	_kp.bHessianSign = m_basePoint.bHessianSign;
	_kp.nLevelId = level.cId;
	_kp.nSize = level.nHessianSize;
	_kp.nValue = nMaxVal;
	return true;
}

bool NearestKeyPointsIterator::_addNext() {
	const size_t cPos = m_iCurrent - m_result.begin();
	while (!m_pLevelIt->eof() || (m_bUseLevels && !_isLastLevel())) {
		while (m_pLevelIt->eof()) {
			if (_isLastLevel())
				return false;
			_nextLevel();
		}

		const Vector2D point = m_pLevelIt->current();
		KeyPoint kp;
		if (_testPoint(point, m_pLevelIt->currentValue(), kp)) {
			m_result.push_back(kp);
			m_iCurrent = m_result.begin() + cPos;
			return true;
		}

		m_pLevelIt->moveNext();
	}

	return false;
}

bool TracingPointsIterator::_testPoint() {
	assert(!m_tracing.detected.isValid());
	m_bDetected = false;
	if (!m_iPoints)
		return false;

	KeyPoint &point = *m_iPoints;
	if (_tryFillPairKeyPoint(point)) {
		m_bDetected = true;
		return true;
	}

	return false;
}

bool TracingPointsIterator::_tryFillPairKeyPoint(KeyPoint &_point) {
	assert(!m_tracing.detected.isValid());
	m_context.pointsBuffer.push_back(KeyPoint3D(_point, m_curPicture.getEarthPlane(), m_curPicture.getFrustum(), &m_curPicture));
	size_t cDetectedCount = 0;
	bool bCalcGrad = false;
	for (auto iTracing = m_context.tracingPoints.begin(); iTracing != m_context.tracingPoints.end(); ++iTracing) {
		TracingPointData &otherTracing = iTracing->get();
		if (&otherTracing == &m_tracing)
			continue;

		if (otherTracing.detected.isValid()) {
			++cDetectedCount;

			ComparedPair comparedPair(otherTracing.detected.pPair->pSecond->screenPoint, _point.point);
			if (m_context.comparedPairs.find(comparedPair) != m_context.comparedPairs.end())
				continue;

			m_context.comparedPairs.insert(comparedPair);

			KeyPair temp;
			if (_tryMakeAccordingPoint(*otherTracing.detected.pPair, _point, temp, &bCalcGrad)) {
				m_tracing.candidates.push_back(temp);
				m_tracing.detected.pPair = &m_tracing.candidates.back();
				FloorDetector::KeyPairData &data = m_tracing.detected;
				m_detector.fillPairData(data);
				FloorDetector::CompareData compareData;
				if (m_detector.isPairCorrect(otherTracing.detected, m_tracing.detected, compareData, false)) {
					m_tracing.updatePrevPictureData(&m_context.pointsBuffer.back());
					return true;
				} else {
					m_tracing.detected.invalidate();
					m_tracing.candidates.pop_back();
				}
			}
		} else {
			for (auto iCandidate = otherTracing.candidates.begin(); iCandidate != otherTracing.candidates.end(); ++iCandidate) {
				KeyPair &candidate = *iCandidate;
				ComparedPair comparedPair(candidate.pSecond->screenPoint, _point.point);
				if (m_context.comparedPairs.find(comparedPair) != m_context.comparedPairs.end())
					continue;

				m_context.comparedPairs.insert(comparedPair);

				KeyPair temp;
				if (_tryMakeAccordingPoint(candidate, _point, temp)) {
					m_tracing.candidates.push_back(temp);
					m_tracing.detected.pPair = &m_tracing.candidates.back();
					FloorDetector::KeyPairData &data1 = m_tracing.detected;
					//TODO: fill pairs data in context?
					m_detector.fillPairData(data1);
					otherTracing.detected.pPair = &candidate;
					m_detector.fillPairData(otherTracing.detected);
					FloorDetector::CompareData compareData;
					if (m_detector.isPairCorrect(otherTracing.detected, m_tracing.detected, compareData, false)) {
						otherTracing.updatePrevPictureData(candidate.pSecond);
						m_tracing.updatePrevPictureData(&m_context.pointsBuffer.back());
						return true;
					} else {
						m_tracing.detected.invalidate();
						m_tracing.candidates.pop_back();
						otherTracing.detected.invalidate();
					}
				}
			}
		}
	}

	if ((cDetectedCount < 3 && m_bUseGradientDetection) || bCalcGrad) {
		if (!m_bUseGradientDetection && m_tracing.calculatedGrads.find(_point.point) != m_tracing.calculatedGrads.end()) {
			m_context.pointsBuffer.pop_back();
			return false;
		}

		m_tracing.calculatedGrads.insert(_point.point);
		KeyPoint3D &keyPoint3d = m_context.pointsBuffer.back();
		keyPoint3d.gradientDirection.clear();
		m_curPicture.calcDescriptor(keyPoint3d, m_iPoints.getHessian().getLevelData(_point.getHessianArrayLevelId()), m_tracing.prevPictureDirection);

		KeyPair temp;
		if (m_comparer.compare(m_iPoints.getBasePoint(), keyPoint3d, &temp)) {
			m_tracing.candidates.push_back(temp);
			if (cDetectedCount != 0) {
				m_tracing.detected.pPair = &m_tracing.candidates.back();
				FloorDetector::KeyPairData &data = m_tracing.detected;
				m_detector.fillPairData(data);

				for (auto iTracing = m_context.tracingPoints.begin(); iTracing != m_context.tracingPoints.end(); ++iTracing) {
					TracingPointData &otherTracing = iTracing->get();
					if (&otherTracing != &m_tracing) {
						if (otherTracing.detected.isValid()) {
							FloorDetector::CompareData compareData;
							if (m_detector.isPairCorrect(otherTracing.detected, m_tracing.detected, compareData, false)) {
								m_tracing.updatePrevPictureData(&keyPoint3d);
								return true;
							}
						}
					}
				}

				m_tracing.detected.invalidate();
			}

			return true;
		}
	}

	m_context.pointsBuffer.pop_back();
	return false;
}
