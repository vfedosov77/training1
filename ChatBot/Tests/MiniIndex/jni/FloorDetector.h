#ifndef _FLOOR_DETECTOR_
#define _FLOOR_DETECTOR_

#include <list>
#include <vector>
#include <math.h>
#include <map>
#include <set>

#include "BaseElements/Accelerometer.h"
#include "BaseElements/Colors.h"
#include "BaseElements/BresenhamIterator.h"
#include "Pictures/KeyPointsImagesComparer.h"
#include "Pictures/Hessian.h"
#include "Application.h"

struct EdgeAndEvidences {
	Edge edge;
	int nWeigth;

	bool operator <(const EdgeAndEvidences &_other) const {
		return nWeigth < _other.nWeigth;
	}
};

extern std::list<EdgeAndEvidences> g_remeberedEdges;
extern Plane g_remeberedEarth;
extern Vector2D g_rememberedPoint;

class FloorDetector {
public:
	struct KeyPairData : public AngleComparableBase<Vector2DF> {
		static const int nCoordCoef = 200;

		float fZoom;

		Vector3D offset;
		KeyPair *pPair;

		Vector2DF planePoint1;
		Vector2DF planePoint2;
		Vector2DF planeGradDir1;
		Vector2DF planeGradDir2;

		float fGradLen1;
		float fGradLen2;
		float fGradLensQuotient;

		mutable int nDistance2;

		Vector2DF angleVector;

		size_t getId() const {
			return pPair->cId;
		}

		void setDirection(const Vector2DF &_vec) {
			angleVector = _vec;
			AngleComparableBase<Vector2DF>::setDirection(_vec);
		}

		KeyPairData() : pPair(nullptr) {}

		KeyPairData(KeyPair &_pair) : pPair(&_pair) {
			//ulnHash = fast_floor((_fAngle + M_PI)*128) + fast_floor(_offset.x/nCoordCoef)<<16 + fast_floor(_offset.y/nCoordCoef)<<32 + fast_floor(_offset.x/nCoordCoef)<<48;
		}

		bool isValid() const {
			return pPair != nullptr;
		}

		void invalidate() {
			pPair = nullptr;
		}
	};

	struct CompareData {
		KeyPairData *pData1;
		KeyPairData *pData2;

		float fZoom;

		bool operator<(const CompareData &_other) const {
			if (pData1 < _other.pData1)
				return true;

			return pData1 == _other.pData1 ? pData2 < _other.pData2 : false;
		}
	};
private:
	static constexpr float ms_fAngleTanMaxDelta = 0.3;
	static constexpr float ms_fMaxZoomDeltaCoef = 0.25;
	static constexpr float ms_fMaxFinalZoomDeltaCoef = 0.1;

	const Picture &m_picture1;
	const Picture &m_picture2;

	PlaneWith2DCoordinates m_plane1;
	PlaneWith2DCoordinates m_plane2;
	Vector2DF m_average1;
	Vector2DF m_average2;
	Vector2DF m_rotationVec;
	float m_fZoom;

	const KeyPointsImagesComparer &m_comparer;

	std::list<KeyPoint3D> m_pointsBuffer;
	std::list<KeyPair> m_pairs;
	std::list<KeyPairData> m_pairsData;
	std::set<CompareData> m_pairsCompareData;
	std::map<KeyPairData *, std::set<KeyPairData *>> m_graphMap;
	std::list<KeyPairData *> m_planesKeys;

	void _extractPlanes();
	bool _testLinesCorrelation(const KeyPairData &_data1, const KeyPairData &_data2) const;
	Vector2DF _reclalculateToSecond(Vector2DF _point) const;
	Vector3D _reclalculateToSecond(const Vector3D &_pointInFirst) const;

	bool tryDetect(const KeyPointsImagesComparer &_comparer, std::list<KeyPair> &_pairs, bool _bMustFind);

	bool _fillAndCompareGradient(const Gradient &grad21, KeyPair &_toRestore, Vector2DF reverseAngle, float fZoom, const Vector2DF &point22,
		const LevelData &_data, KeyPointsImagesComparer &_comparer, KeyPair &_result, bool *_pbTryCalcGrad = nullptr
	) const;

	CompareData *_getCompareData(KeyPairData *_pData1, KeyPairData *_pData2) {
		CompareData fake;
		if (_pData1->getId() < _pData2->getId()) {
			fake.pData1 = _pData1;
			fake.pData2 = _pData2;
		} else {
			fake.pData1 = _pData2;
			fake.pData2 = _pData1;
		}
		auto iData = m_pairsCompareData.find(fake);
		if (iData == m_pairsCompareData.end()) {
			assert(false);
			return nullptr;
		}

		return const_cast<CompareData *>(&*iData);
	}

public:
	FloorDetector(const Picture &_picture1, const Picture &_picture2,
		const KeyPointsImagesComparer &_comparer, std::list<KeyPair> &_pairs, bool _bMustFind = true
	);

	bool calculateFrustum(OrientedFrustum &_frustum) const;

	bool detected() const {
		return m_fZoom != 0;
	}

	bool tryRestorePointGradient(const KeyPair &_base, KeyPair &_toRestore, const LevelData &_data, KeyPointsImagesComparer &_comparer, KeyPair &_result, bool *_pbTryCalcGrad = nullptr) const;
	void fillPairData(KeyPairData &_data) const;
	bool isPairCorrect(KeyPairData &_data1, KeyPairData &_data2, CompareData &_compareData, bool _bUseImagesCompare) const;
	int detectPlanesRelationsWithFilter(std::list<KeyPairData *> &_planesKeys, OrientedFrustum &_calculatedFrustum);
	void detectPlanesRelations(std::list<KeyPairData *> &_planesKeys);
	void dump(const std::list<KeyPairData *> &_planesKeys, std::list<KeyPair> &_pairs, const std::list<KeyPoint3D> *_pPoints1 = nullptr, const std::list<KeyPoint3D> *_pPoints2 = nullptr);

	std::list<KeyPairData *> &getPlanesKeys() {
		return m_planesKeys;
	}

	std::list<KeyPair> &getPairs() {
		return m_pairs;
	}

	std::list<KeyPoint3D> &getPointsBuffer() {
		return m_pointsBuffer;
	}

	void clear() {
		m_pointsBuffer.clear();
		m_pairsCompareData.clear();
		m_graphMap.clear();
		m_planesKeys.clear();
		m_pairsData.clear();
	}
};

#endif //_FLOOR_DETECTOR_
