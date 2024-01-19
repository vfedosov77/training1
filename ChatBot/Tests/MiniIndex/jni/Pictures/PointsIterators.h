#ifndef _POINTS_ITERATORS_H_
#define _POINTS_ITERATORS_H_

#include "Hessian.h"
#include "KeyPointsImagesComparer.h"
#include "Dynamic/DynamicContext.h"

void levelMaximumsIteratorTest();

template<class DataProvider, size_t cStepShift>
class LevelMaximumsIterator;

class NearestKeyPointsIterator {
	static constexpr size_t ms_cStepShift = 1;

	struct GridDataProvider {
		HessianTransform &m_hessian;
		DynamicGridWithThreshold &m_grid;
		std::vector<DynamicGridWithThreshold::GridCell> *m_pCells;
		std::vector<DynamicGridWithThreshold::GridCell> *m_pPrevCells;
		std::vector<DynamicGridWithThreshold::GridCell> *m_pNextCells;

		const LevelData *pCurrentLevelData;
		Rect2D allowedRect;

		GridDataProvider(HessianTransform &_hessian, int _nCurLevelId) : m_hessian(_hessian), m_grid(_hessian.getDynamicGrid()) {
			setCurLevel(_nCurLevelId);
		}

		inline
		int getValue(int _nX, int _nY) {
			return m_grid.getValue(*m_pCells, _nX, _nY);
		}

		void setCurLevel(int _nLevel) {
			m_pCells = &m_hessian.getDynamicGrid().getLevelCells(_nLevel);
			m_pPrevCells = _nLevel == 0 ? nullptr : &m_grid.getLevelCells(_nLevel - 1);
			m_pNextCells = _nLevel == g_cLevelsCount - 1 ? nullptr : &m_grid.getLevelCells(_nLevel + 1);
			pCurrentLevelData = &m_hessian.getLevelData(_nLevel);
			allowedRect = m_pNextCells == nullptr ? pCurrentLevelData->allowedRect : m_hessian.getLevelData(_nLevel + 1).allowedRect;
		}
	} m_provider;

	typedef LevelMaximumsIterator<GridDataProvider, DynamicGridWithThreshold::ms_nStepShift> MaximumIterator;

	MaximumIterator *m_pLevelIt;

	const KeyPoint3D &m_basePoint;
	Vector2D m_prevPicturePoint;
	HessianTransform &m_hessian;
	bool m_bUseLevels;
	int m_nLevelId, m_nCurLevelId;

	std::vector<KeyPoint> m_result;
	std::vector<KeyPoint>::iterator m_iCurrent;
	std::vector<size_t> m_zooms;
	size_t m_rememberedPosition;
	bool m_bFarSearch;

	bool _isLastLevel() const {
		return m_nCurLevelId > m_nLevelId || (m_nCurLevelId < m_nLevelId && m_nLevelId == g_cLevelsCount - 1);
	}

	void _nextLevel() {
		m_nCurLevelId = m_nCurLevelId == m_nLevelId ? (m_nLevelId == 0 ? m_nLevelId + 1 : m_nLevelId - 1) : m_nLevelId + 1;
		m_provider.setCurLevel(m_nCurLevelId);
		_createLevelIterator();
	}

	bool _addNext();
	void _createLevelIterator();
	bool _testPoint(const Vector2D &_point, int _nValue, KeyPoint &_kp);

	NearestKeyPointsIterator(const NearestKeyPointsIterator &_other);
public:
	NearestKeyPointsIterator(const KeyPoint3D &_basePoint, Vector2D &_prevPicturePoint, HessianTransform &_hessian, bool _bUseLevels, bool _bFarSearch);

	~NearestKeyPointsIterator();

	operator bool() {
		return m_iCurrent != m_result.end();
	}

	void operator ++ ();

	KeyPoint & operator *() {
		assert(m_iCurrent != m_result.end());
		return *m_iCurrent;
	}

	void rememberAndReset() {
		m_rememberedPosition = m_iCurrent - m_result.begin();
		m_iCurrent = m_result.begin();
	}

	void restorePosition() {
		m_iCurrent = m_result.begin() + m_rememberedPosition;
	}

	bool isCurrentPosRemembered() const {
		return m_iCurrent == m_result.begin() + m_rememberedPosition;
	}

	const KeyPoint3D &getBasePoint() const {
		return m_basePoint;
	}

	const HessianTransform &getHessian() const {
		return m_hessian;
	}
};

class TracingPointsIterator : public Counted {
	TracingPointData &m_tracing;
	DynamicContext &m_context;
	NearestKeyPointsIterator m_iPoints;
	KeyPointsImagesComparer &m_comparer;
	Picture &m_curPicture;
	const FloorDetector &m_detector;
	bool m_bDetected;
	bool m_bUseGradientDetection;

	bool _tryMakeAccordingPoint(const KeyPair &_base, KeyPoint &_point, KeyPair &_result, bool *_pbTryCalcGrad = nullptr) {
		KeyPoint3D &keyPoint3d = m_context.pointsBuffer.back();
		keyPoint3d.gradientDirection.clear();
		_result.pFirst = &m_iPoints.getBasePoint();
		_result.pSecond = &keyPoint3d;
		if (m_detector.tryRestorePointGradient(_base, _result, m_iPoints.getHessian().getLevelData(_point.getHessianArrayLevelId()), m_comparer, _result, _pbTryCalcGrad))
			return true;

		return false;
	}

	bool _testPoint();
	bool _tryFillPairKeyPoint(KeyPoint &_point);

	TracingPointsIterator(const TracingPointsIterator &_other);

public:
	TracingPointsIterator(TracingPointData &_tracing, HessianTransform &_hessian, bool _bUseLevels,
		KeyPointsImagesComparer &_comparer, Picture &_curPicture, const FloorDetector &_detector,
		DynamicContext &_context, bool _bFarSearch) :
		m_tracing(_tracing), m_context(_context), m_iPoints(*_tracing.pBasePoint, _tracing.prevPicturePoint, _hessian, _bUseLevels, _bFarSearch), m_comparer(_comparer),
		m_curPicture(_curPicture), m_detector(_detector), m_bDetected(false), m_bUseGradientDetection(true)
	{
		assert(_tracing.pBasePoint != nullptr);
		if (!m_tracing.detected.isValid())
			_testPoint();
	}

	operator bool() {
		return m_iPoints;
	}

	KeyPair *operator *() {
		return m_bDetected ? &m_tracing.candidates.back() : nullptr;
	}

	bool isDetected() const {
		return m_bDetected || isApproved();
	}

	bool isApproved() const {
		return m_tracing.detected.isValid();
	}

	void operator ++ () {
		assert(!m_tracing.detected.isValid());
		assert(m_iPoints);
		++m_iPoints;
		_testPoint();
	}

	void rememberAndReset() {
		assert(!m_tracing.detected.isValid());
		m_iPoints.rememberAndReset();
		m_bUseGradientDetection = false;
		_testPoint();
		m_bUseGradientDetection = true;
	}

	void restorePosition() {
		assert(!m_tracing.detected.isValid());
		m_iPoints.restorePosition();
		_testPoint();
	}

	void moveToRemembered() {
		assert(!m_tracing.detected.isValid());
		m_bUseGradientDetection = false;
		while (m_iPoints && !isApproved() && !m_iPoints.isCurrentPosRemembered()) {
			this->operator ++();
		}

		m_bUseGradientDetection = true;
	}
};

#endif //_POINTS_ITERATORS_H_
