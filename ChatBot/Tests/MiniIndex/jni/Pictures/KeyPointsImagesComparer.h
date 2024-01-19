#ifndef _KEY_POINTS_IMAGES_COMPARER_
#define _KEY_POINTS_IMAGES_COMPARER_

#include <string.h>

#include "BaseElements/BresenhamIterator.h"
#include "Picture.h"


struct KeyPair {
	KeyPair() : fCorrelation(0), bApproved(false), cDiscardCounter(0) {
		static size_t cCurId = 0;
		cId = cCurId++;
	}

	bool isDiscarded() const {
		return cDiscardCounter > 2;
	}

	const KeyPoint3D *pFirst, *pSecond;
	const Gradient *pGrad1, *pGrad2;
	float fCorrelation;
	float fZoom;
	float fAngle;
	bool bApproved;
	size_t cDiscardCounter;
	size_t cId;
};

class KeyPointsImagesComparer {
	Picture &m_firstPicture;
	Picture &m_secondPicture;
	const IntegralTransform &m_firstIntegral;
	const IntegralTransform &m_secondIntegral;
	const byte *m_pFirstBuf;
	const byte *m_pSecondBuf;
	Vector3D m_firstVerticalDir;
	Vector3D m_secondVerticalDir;
	std::map<int, size_t> m_pointsIdsToMatchCount;

	float _getCompareZoom(const KeyPoint3D &_first, const KeyPoint3D &_second, float _fGradLen1, float _fOrtLen1,
		float _fGradLen2, float _fOrtLen2, bool &_bSmall
	) const;
public:
	KeyPointsImagesComparer(Picture &_firstPicture, Picture &_secondPicture) :
		m_firstPicture(_firstPicture), m_secondPicture(_secondPicture),
		m_firstIntegral(_firstPicture.getIntegral()), m_secondIntegral(_secondPicture.getIntegral()),
		m_pFirstBuf(_firstPicture.getBuf()), m_pSecondBuf(_secondPicture.getBuf()),
		m_firstVerticalDir(_firstPicture.getVerticalDirection()), m_secondVerticalDir(_secondPicture.getVerticalDirection())
	{

	}

	static
	inline bool isLongCompare(const Gradient &_firstGrad, const Gradient &_secondGrad) {
		return std::min(_firstGrad.fGradLen, _secondGrad.fGradLen) < 9;
	}

	bool compareAlongOrt(const Gradient &_firstGrad, const Gradient &_secondGrad, bool *_pbNear = nullptr);
	bool compareAlongGrad(const Gradient &_firstGrad, const Gradient &_secondGrad, int &_nSecondXOffset, bool *_pbNear);
	bool compareBuffs(const Gradient &_firstGrad, const Gradient &_secondGrad, const KeyPoint3D &_first, const KeyPoint3D &_second, int nSecondXOffset, KeyPair *_pPair);
	bool compare(const KeyPoint3D &_first, const KeyPoint3D &_second, KeyPair *pPair = nullptr, bool *_pbNear = nullptr);
	bool compareImages(const KeyPair &_pair1, const KeyPair &_pair2) const;

	size_t getPointsMatchesCount(const KeyPoint3D &_point) const {
		auto iPoint = m_pointsIdsToMatchCount.find(_point.nId);
		return iPoint == m_pointsIdsToMatchCount.end() ? 0 : iPoint->second;
	}

	bool isNonUnique(const KeyPair &_pair) const {
		static const size_t cMaxRelationCount = 2;
		return getPointsMatchesCount(*_pair.pFirst) > cMaxRelationCount || getPointsMatchesCount(*_pair.pSecond) > cMaxRelationCount;
	}

	void clearStatistic() {
		m_pointsIdsToMatchCount.clear();
	}
};

#endif
