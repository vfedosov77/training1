#include "BaseElements/GeometryFunctions.h"
#include "Draw/Dumper.h"
#include "Pictures/PicturesStorage.h"
#include "FloorDetector.h"

bool FloorDetector::_fillAndCompareGradient(const Gradient &grad21, KeyPair &_toRestore, Vector2DF reverseAngle, float fZoom, const Vector2DF &point22,
	const LevelData &_data,	KeyPointsImagesComparer &_comparer, KeyPair &_result, bool *_pbTryCalcGrad
) const {
	const Vector2DF gradEnd21(Vector2DF(_toRestore.pFirst->screenPoint) + grad21.directionF);
	const Vector2DF gradDirection21 = m_plane1.get2DCoord(m_picture1.getFrustum().toPlane(gradEnd21, m_picture1.getEarthPlane())) - m_plane1.get2DCoord(_toRestore.pFirst->point);

	Vector2DF gradDirection22 = rotateByVector(gradDirection21, reverseAngle);
	gradDirection22 *= fZoom;
	const Vector3D gradEnd22 = m_plane2.get3DCoord(point22 + gradDirection22);
	KeyPoint3D *pSecond = const_cast<KeyPoint3D *>(_toRestore.pSecond);
	const Vector2DF directionF = m_picture2.getFrustum().toScreenF(gradEnd22) - Vector2DF(pSecond->screenPoint);

	pSecond->gradientDirection.push_back(Gradient());
	Gradient &grad = pSecond->gradientDirection.back();
	grad.directionF = directionF;
	const Vector2DF ortDirection22(-gradDirection22.y, gradDirection22.x);
	const Vector3D ortEnd22 = m_plane2.get3DCoord(point22 + ortDirection22);
	const Vector2DF ortF = m_picture2.getFrustum().toScreenF(ortEnd22) - Vector2DF(pSecond->screenPoint);
	grad.ortF = ortF;
	grad.fOrtZoom = ortF.length()/directionF.length();

	GradientArrayMinMax minMax;
	//TODO: optimize this.
	const float fGradLen = directionF.length();
	const float fGradsZoom = directionF.length()/grad21.directionF.length();
	minMax.direction = (directionF.getNormalized()*(grad21.minMaxDir.length()*fGradsZoom)).toInt();
	minMax.ort = (ortF*2).toInt();
	minMax.cGradCount = std::max(std::abs(minMax.direction.x), std::abs(minMax.direction.y))*2 + 1;
	minMax.fBrasenhamCoef = minMax.directionMaxCoordProjection()/minMax.direction.length();
	const float fDelta = fGradLen*minMax.fBrasenhamCoef;
	const float fCentreIndex = (float(minMax.cGradCount) - 1)/2 + fDelta*grad21.fCentreOffset;

	minMax.fGradXMinIdx = grad21.fSinCorrelation > 0 ? fCentreIndex - fDelta/2 : fCentreIndex + fDelta/2;
	minMax.fGradXMaxIdx = grad21.fSinCorrelation > 0 ? fCentreIndex + fDelta/2 : fCentreIndex - fDelta/2;
	minMax.fSinCorrelation = grad21.fSinCorrelation;
	assert(minMax.fGradXMinIdx >= -1 && minMax.fGradXMinIdx <= minMax.cGradCount);
	assert(minMax.fGradXMaxIdx >= -1 && minMax.fGradXMaxIdx <= minMax.cGradCount);

	grad.calcGradLen(minMax);
	const bool bLongCompare = KeyPointsImagesComparer::isLongCompare(grad, grad21);
	grad.calcOrt(*pSecond, minMax, m_picture2.getIntegral(), bLongCompare);
	if (!_comparer.compareAlongOrt(grad21, grad, _pbTryCalcGrad)) {
		pSecond->gradientDirection.pop_back();
		return false;
	}

	grad.calcGrad(*pSecond, minMax, _data, m_picture2.getIntegral(), bLongCompare);
	int nSecondXOffset;
	if (!_comparer.compareAlongGrad(grad21, grad, nSecondXOffset, _pbTryCalcGrad) ||
		!_comparer.compareBuffs(grad21, grad, *_toRestore.pFirst, *pSecond, nSecondXOffset, &_result)
	) {
		pSecond->gradientDirection.pop_back();
		return false;
	}
	return true;
}

bool FloorDetector::tryRestorePointGradient(const KeyPair &_base, KeyPair &_toRestore, const LevelData &_data, KeyPointsImagesComparer &_comparer, KeyPair &_result, bool *_pbTryCalcGrad) const {
	KeyPoint3D *pSecond = const_cast<KeyPoint3D *>(_toRestore.pSecond);
	pSecond->gradientDirection.clear();

	if (_toRestore.pFirst->bHessianSign != pSecond->bHessianSign)
		return false;

	const Vector2DF point22 = m_plane2.get2DCoord(pSecond->point);
	const Vector2DF dir1 = m_plane1.get2DCoord(_toRestore.pFirst->point) - m_plane1.get2DCoord(_base.pFirst->point);
	const Vector2DF dir2 = point22 - m_plane2.get2DCoord(_base.pSecond->point);
	if (dir1.empty() || dir2.empty())
		return false;

	const Vector2DF angleDir = rotateByVector(dir2, dir1);
	Vector2DF reverseAngle(angleDir.x, -angleDir.y);
	reverseAngle.normalize();

	const float fZoom = dir2.length()/dir1.length();
	assert(_base.pFirst->gradientDirection.size() >= 1);

	const std::list<Gradient> &dirs1 = _base.pFirst->gradientDirection;
	const std::list<Gradient> &dirs2 = _base.pSecond->gradientDirection;
	for (auto iGrad11 = dirs1.begin(); iGrad11 != dirs1.end(); ++iGrad11) {
		for (auto iGrad12 = dirs2.begin(); iGrad12 != dirs2.end(); ++iGrad12) {
			const Vector2DF gradEnd11(Vector2DF(_base.pFirst->screenPoint) + iGrad11->directionF);
			const Vector2DF gradDirection11 = m_plane1.get2DCoord(m_picture1.getFrustum().toPlane(gradEnd11, m_picture1.getEarthPlane())) - m_plane1.get2DCoord(_base.pFirst->point);
			const Vector2DF gradEnd12(Vector2DF(_base.pSecond->screenPoint) + iGrad12->directionF);
			const Vector2DF gradDirection12 = m_plane2.get2DCoord(m_picture2.getFrustum().toPlane(gradEnd12, m_picture2.getEarthPlane())) - m_plane2.get2DCoord(_base.pSecond->point);

			const float fGradLensQuotient = gradDirection12.length()/gradDirection11.length();
			if (std::abs((fGradLensQuotient - fZoom)/fZoom) > ms_fMaxZoomDeltaCoef)
				continue;

			Vector2DF angleGradVec = rotateByVector(gradDirection12, gradDirection11);
			if (getTanAngle(angleGradVec, angleDir) > ms_fAngleTanMaxDelta)
				continue;

			const std::list<Gradient> &dirs21 = _toRestore.pFirst->gradientDirection;
			for (auto iGrad21 = dirs21.begin(); iGrad21 != dirs21.end(); ++iGrad21) {
				const Gradient &grad21 = *iGrad21;
				if (_fillAndCompareGradient(grad21, _toRestore, reverseAngle, fZoom, point22, _data, _comparer, _result, _pbTryCalcGrad))
					return true;
			}
		}
	}

	return false;
}

void FloorDetector::fillPairData(KeyPairData &_data) const {
	const KeyPair &pair = *_data.pPair;
	const Gradient &grad1 = *pair.pGrad1;
	const Gradient &grad2 = *pair.pGrad2;
	_data.planePoint1 = m_plane1.get2DCoord(pair.pFirst->point);
	_data.planePoint2 = m_plane2.get2DCoord(pair.pSecond->point);

	_data.planeGradDir1 = m_plane1.get2DCoord(m_picture1.getFrustum().toPlane(Vector2DF(pair.pFirst->screenPoint) +
		grad1.directionF, m_picture1.getEarthPlane())) - _data.planePoint1
	;
	_data.planeGradDir2 = m_plane2.get2DCoord(m_picture2.getFrustum().toPlane(Vector2DF(pair.pSecond->screenPoint) +
		 grad2.directionF, m_picture2.getEarthPlane())) - _data.planePoint2
	;

	_data.fGradLen1 = _data.planeGradDir1.length();
	_data.fGradLen2 = _data.planeGradDir2.length();
	_data.fGradLensQuotient = _data.fGradLen1/_data.fGradLen2;

	_data.setDirection(rotateByVector(_data.planeGradDir1, _data.planeGradDir2));
}

bool FloorDetector::isPairCorrect(KeyPairData &_data1, KeyPairData &_data2, CompareData &_compareData, bool _bUseImagesCompare) const {
	const Vector2DF dirBetween1 = _data2.planePoint1 - _data1.planePoint1;
	const Vector2DF dirBetween2 = _data2.planePoint2 - _data1.planePoint2;

	const Vector2DF angleDir = rotateByVector(dirBetween1, dirBetween2);

	if (getTanAngle(_data1.angleVector, angleDir) > ms_fAngleTanMaxDelta)
		return false;

	if (getTanAngle(_data2.angleVector, angleDir) > ms_fAngleTanMaxDelta)
		return false;

	if (std::abs((_data1.fGradLensQuotient - _data2.fGradLensQuotient)/std::max(_data1.fGradLensQuotient, _data2.fGradLensQuotient)) > 2*ms_fMaxZoomDeltaCoef)
		return false;

	const float fDirsQuotient = dirBetween1.length()/dirBetween2.length();

	if (std::abs((_data1.fGradLensQuotient - fDirsQuotient)/fDirsQuotient) > ms_fMaxZoomDeltaCoef)
		return false;

	if (std::abs((_data2.fGradLensQuotient - fDirsQuotient)/fDirsQuotient) > ms_fMaxZoomDeltaCoef)
		return false;

	_compareData.fZoom = fDirsQuotient;
	_compareData.pData1 = &_data1;
	_compareData.pData2 = &_data2;

	if (!_bUseImagesCompare || (_data1.pPair->bApproved && _data2.pPair->bApproved))
		return true;

	const bool bRes = m_comparer.compareImages(*_data1.pPair, *_data2.pPair);

	if (bRes) {
		_data1.pPair->bApproved = true;
		_data2.pPair->bApproved = true;
	} else {
		if (_data1.pPair->bApproved)
			++_data2.pPair->cDiscardCounter;
		else if (_data2.pPair->bApproved)
			++_data1.pPair->cDiscardCounter;
	}

	return bRes;
}

template<typename T>
static inline
float _getMaxDencityValue(T _begin, T _end, float _fMaxDelta) {
	if (_begin == _end)
		return 0;

	T front = _begin;
	T last = _begin;
	++front;
	float fMaxVal = 0;
	size_t cMaxCount = 0;
	size_t cCurCount = 1;
	for (; front != _end; ++front, ++cCurCount) {
		while (*front - *last > _fMaxDelta) {
			++last;
			--cCurCount;
		}

		if (cCurCount > cMaxCount) {
			cMaxCount = cCurCount;
			fMaxVal = (*front + *last)/2;
		}
	}

	return fMaxVal;
}

bool FloorDetector::_testLinesCorrelation(const KeyPairData &_data1, const KeyPairData &_data2) const {
	return true;
	static const size_t cArraySize = 30;
	static float pArray1[cArraySize];
	static float pArray2[cArraySize];
	m_picture1.fillLineGradients(Edge(_data1.pPair->pFirst->screenPoint, _data2.pPair->pFirst->screenPoint), pArray1, cArraySize);
	m_picture2.fillLineGradients(Edge(_data1.pPair->pSecond->screenPoint, _data2.pPair->pSecond->screenPoint), pArray2, cArraySize);
	const float fRes = correlation(pArray1, pArray2, cArraySize);
	return fRes > 0.6;
}

void FloorDetector::_extractPlanes() {
	size_t cMaxNeighboars = 0;
	KeyPairData *pNodeWithMaxNeighboars = nullptr;
	for (auto iPair = m_graphMap.begin(); iPair != m_graphMap.end(); ++iPair) {
		if (iPair->second.size() > cMaxNeighboars) {
			cMaxNeighboars = iPair->second.size();
			pNodeWithMaxNeighboars = iPair->first;
		}
	}

	if (cMaxNeighboars < 2)
		return;

	m_planesKeys.push_back(pNodeWithMaxNeighboars);
	std::set<KeyPairData *> &neghboars = m_graphMap[pNodeWithMaxNeighboars];

	std::vector<float> zooms;
	for (auto iNeig = neghboars.begin(); iNeig != neghboars.end(); ++iNeig) {
		KeyPairData *pNeighboar = *iNeig;
		CompareData *pData = _getCompareData(pNodeWithMaxNeighboars, pNeighboar);
		zooms.push_back(pData->fZoom);
	}

	std::sort(zooms.begin(), zooms.end());
	const float fZoom = _getMaxDencityValue(zooms.begin(), zooms.end(), ms_fMaxFinalZoomDeltaCoef);
	const float fMaxZoomDelta = ms_fMaxFinalZoomDeltaCoef*fZoom;
	const size_t cConnectionsMin = neghboars.size()/2;
	std::set<KeyPairData *> additional;

	for (auto iNeig = neghboars.begin(); iNeig != neghboars.end(); ++iNeig) {
		KeyPairData *pNeighboar = *iNeig;
		CompareData *pData = _getCompareData(pNodeWithMaxNeighboars, pNeighboar);
		if (std::abs(pData->fZoom - fZoom) > fMaxZoomDelta)
			continue;

		std::set<KeyPairData *> &connections = m_graphMap[pNeighboar];
		size_t cConnections = 1;
		for (auto iCon = connections.begin(); iCon != connections.end(); ++iCon) {
			KeyPairData *pOtherNeighboar = *iCon;
			if (neghboars.find(pOtherNeighboar) != neghboars.end() && _testLinesCorrelation(*pNeighboar, *pOtherNeighboar))
				++cConnections;
			else {
				CompareData *pOtherData = _getCompareData(pOtherNeighboar, pNeighboar);
				if (std::abs(pOtherData->fZoom - fZoom) <= fMaxZoomDelta)
					additional.insert(pOtherNeighboar);
			}
		}

		if (cConnections >= cConnectionsMin)
			m_planesKeys.push_back(pNeighboar);
	}

	for (auto iNeig = additional.begin(); iNeig != additional.end(); ++iNeig) {
		KeyPairData *pNeighboar = *iNeig;
		std::set<KeyPairData *> &connections = m_graphMap[pNeighboar];
		size_t cConnections = 0;
		for (auto iOtherNeig = connections.begin(); iOtherNeig != connections.end(); ++iOtherNeig)
			if (neghboars.find(*iOtherNeig) != neghboars.end() && _testLinesCorrelation(*pNeighboar, **iOtherNeig))
				++cConnections;

		if (cConnections >= cConnectionsMin)
			m_planesKeys.push_back(pNeighboar);
	}

//	for (auto iKey = m_planesKeys.begin(); iKey != m_planesKeys.end(); ++iKey)
//		if ((*iKey)->pPair == nullptr)
//			iKey = m_planesKeys.erase(iKey);
//		else
//			++iKey;
}

Vector2DF FloorDetector::_reclalculateToSecond(Vector2DF _point) const {
	_point -= m_average1;
	_point = rotateByVector(_point, m_rotationVec);
	_point *= m_fZoom;
	_point += m_average2;
	return _point;
}

Vector3D FloorDetector::_reclalculateToSecond(const Vector3D &_pointInFirst) const {
	Vector2DF point = m_plane1.get2DCoord(_pointInFirst);
	return m_plane2.get3DCoord(_reclalculateToSecond(point));
}

bool FloorDetector::calculateFrustum(OrientedFrustum &_frustum) const {
	if (!detected())
		return false;

	const OrientedFrustum &baseFrustum = m_picture1.getOrientedFrustum();
	const Vector2DF plane2Pos = m_plane2.get2DProjection(PicturePosition::defaultPosition.position);
	Vector2DF pos2D = plane2Pos;
	float fHeigth = m_plane2.m_plane.distance(PicturePosition::defaultPosition.position);
	pos2D -= m_average2;
	Vector2DF reverseRotation = m_rotationVec;
	reverseRotation.y = -reverseRotation.y;
	pos2D = rotateByVector(pos2D, reverseRotation);
	pos2D /= m_fZoom;
	pos2D += m_average1;
	Vector3D position = m_plane1.get3DCoord(pos2D);
	fHeigth /= m_fZoom;
	position += m_plane1.m_plane.normal*fHeigth;

	Vector2DF dir2D = m_plane2.get2DProjection(PicturePosition::defaultPosition.direction + PicturePosition::defaultPosition.position) - plane2Pos;
	float fDirHeigth = m_plane2.m_plane.distance(m_plane2.m_plane.point + PicturePosition::defaultPosition.direction);
	dir2D = rotateByVector(dir2D, reverseRotation);
	//dir2D /= m_fZoom;
	Vector3D direction = m_plane1.get3DDirection(dir2D);
	//fDirHeigth /= m_fZoom;
	direction += m_plane1.m_plane.normal*fDirHeigth;

	Vector2DF topDir2D = m_plane2.get2DProjection(PicturePosition::defaultPosition.topDirection + PicturePosition::defaultPosition.position) - plane2Pos;
	float fTopDirHeigth = m_plane2.m_plane.distance(m_plane2.m_plane.point + PicturePosition::defaultPosition.topDirection);
	topDir2D = rotateByVector(topDir2D, reverseRotation);
	//topDir2D /= m_fZoom;
	Vector3D topDirection = m_plane1.get3DDirection(topDir2D);
	//fTopDirHeigth /= m_fZoom;
	topDirection += m_plane1.m_plane.normal*fTopDirHeigth;

	position = baseFrustum.fromLocalCoordinates(position);
	direction = baseFrustum.fromLocalCoordinates(direction) - baseFrustum.getPosition();
	topDirection = baseFrustum.fromLocalCoordinates(topDirection) - baseFrustum.getPosition();

	const Frustum &localFrustum = m_picture2.getFrustum();
	_frustum = OrientedFrustum(localFrustum.getFocalLength(), localFrustum.getWidth(), localFrustum.getHeight(), position, direction, topDirection);
	return true;
}

int FloorDetector::detectPlanesRelationsWithFilter(std::list<KeyPairData *> &_planesKeys, OrientedFrustum &_calculatedFrustum) {
	if (_planesKeys.size() < 4) {
		detectPlanesRelations(_planesKeys);
		calculateFrustum(_calculatedFrustum);
		return -1;
	}

	static int nNormalDistance2 = 225;
	bool bHasIncorrect;
	KeyPairData *pErased = nullptr;
	const int nCount = _planesKeys.size();
	int nRemoved = -1;
	do {
		bHasIncorrect = false;
		detectPlanesRelations(_planesKeys);
		if (calculateFrustum(_calculatedFrustum)) {
			for (auto iPair = _planesKeys.begin(); iPair != _planesKeys.end(); ++iPair) {
				KeyPairData *pPair = *iPair;
				const Vector2D calculatedScreenPoint =  _calculatedFrustum.toScreen(pPair->pPair->pFirst->point);
				const int nDistance2 = calculatedScreenPoint.distance2(pPair->pPair->pSecond->screenPoint);

				if (nDistance2 > nNormalDistance2)
					bHasIncorrect = true;
			}
		} else
			bHasIncorrect = true;

		if (bHasIncorrect) {
			if (pErased != nullptr)
				_planesKeys.push_back(pErased);

			pErased = _planesKeys.front();
			_planesKeys.erase(_planesKeys.begin());
		}
	} while(bHasIncorrect && nRemoved++ < nCount);

	if (bHasIncorrect) {
		assert(pErased != nullptr);
		_planesKeys.push_back(pErased);
		detectPlanesRelations(_planesKeys);
		calculateFrustum(_calculatedFrustum);
	}

	return bHasIncorrect ? -1 : nRemoved;
}

void FloorDetector::detectPlanesRelations(std::list<KeyPairData *> &_planesKeys) {
	m_average1 = Vector2DF();
	m_average2 = Vector2DF();

	for (auto iPair = _planesKeys.begin(); iPair != _planesKeys.end(); ++iPair) {
		m_average1 += (*iPair)->planePoint1;
		m_average2 += (*iPair)->planePoint2;
	}

	m_average1 /= _planesKeys.size();
	m_average2 /= _planesKeys.size();
	Vector2DF sumOfAngles;
	size_t cCount = 0;
	float fDistSum1 = 0;
	float fDistSum2 = 0;
	for (auto iPair = _planesKeys.begin(); iPair != _planesKeys.end(); ++iPair) {
		KeyPairData *pData = *iPair;
		const Vector2DF point1 = pData->planePoint1 - m_average1;
		const Vector2DF point2 = pData->planePoint2 - m_average2;
		const float fDist1 = point1.length();
		const float fDist2 = point2.length();
		if (fDist1 == 0 || fDist2 == 0)
			continue;

		fDistSum1 += fDist1;
		fDistSum2 += fDist2;
		const Vector2DF angleVector = rotateByVector(point1, point2.getNormalized());
		sumOfAngles += angleVector;
		++cCount;
	}

	if (sumOfAngles.empty() || fDistSum1 == 0) {
		m_fZoom = 0;
		return;
	}

	m_rotationVec = sumOfAngles.getNormalized();
	m_fZoom = fDistSum2/fDistSum1;
}

bool FloorDetector::tryDetect(const KeyPointsImagesComparer &_comparer, std::list<KeyPair> &_pairs, bool _bMustFind) {
	for (auto iPair = _pairs.begin(); iPair != _pairs.end(); ++iPair) {
//		dump1.dumpKeypoint(*pair.pFirst);
//		dump1.number(pair.pFirst->screenPoint, cNum);
//		dump2.dumpKeypoint(*pair.pSecond);
//		dump2.number(pair.pSecond->screenPoint, cNum);
		KeyPairData data(*iPair);
		fillPairData(data);
		m_pairsData.push_back(data);
	}

	m_pairsData.sort();
	std::list<KeyPairData *> equalAngle;
	std::list<KeyPairData *> equalAngleMax;
	size_t cAngleMaxCount = 0;

	if (!_bMustFind)
		for (auto iPair = m_pairsData.begin(); iPair != m_pairsData.end(); ++iPair) {
			KeyPairData &data = *iPair;
			equalAngle.push_back(&data);

			while (equalAngle.size() > 1 && getTanAngle(equalAngle.front()->angleVector, data.angleVector) > ms_fAngleTanMaxDelta)
				equalAngle.pop_front();

			if (equalAngle.size() > cAngleMaxCount) {
				cAngleMaxCount = equalAngle.size();
				equalAngleMax = equalAngle;
			}
		}
	else
		for (auto iPair = m_pairsData.begin(); iPair != m_pairsData.end(); ++iPair) {
			KeyPairData &data = *iPair;
			equalAngleMax.push_back(&data);
		}

	assert(m_pairsCompareData.empty());
	for (auto iPair = equalAngleMax.begin(); iPair != equalAngleMax.end(); ++iPair) {
		KeyPairData &data1 = **iPair;
		if (data1.pPair->isDiscarded() || _comparer.isNonUnique(*data1.pPair))
			continue;

		for (auto iPair2 = equalAngleMax.begin(); iPair2 != equalAngleMax.end(); ++iPair2) {
			KeyPairData &data2 = **iPair2;
			if (data1.pPair->cId >= data2.pPair->cId || data2.pPair->isDiscarded() || _comparer.isNonUnique(*data2.pPair))
				continue;

//			if (pData1->pPair->cId != 5 || pData2->pPair->cId != 17)
//				continue;

			CompareData compareData;
			if (isPairCorrect(data1, data2, compareData, !_bMustFind)) {
				assert(compareData.pData1->getId() < compareData.pData2->getId());
				m_pairsCompareData.insert(compareData);
				//dump1.line(pData1->pPair->pFirst->screenPoint, pData2->pPair->pFirst->screenPoint, nPairNum, cv::Scalar(255, 0, 0));
				//dump2.line(pData1->pPair->pSecond->screenPoint, pData2->pPair->pSecond->screenPoint, nPairNum, cv::Scalar(255, 0, 0));
			} else {
				//dump1.line(pData1->pPair->pFirst->screenPoint, pData2->pPair->pFirst->screenPoint, nPairNum, cv::Scalar(0, 0, 255));
				//dump2.line(pData1->pPair->pSecond->screenPoint, pData2->pPair->pSecond->screenPoint, nPairNum, cv::Scalar(0, 0, 255));
			}
		}
	}

	for (auto iPair = m_pairsCompareData.begin(); iPair != m_pairsCompareData.end(); ++iPair) {
		m_graphMap[iPair->pData1].insert(iPair->pData2);
		m_graphMap[iPair->pData2].insert(iPair->pData1);
	}

	_extractPlanes();
	detectPlanesRelations(m_planesKeys);

	PicturesStorage::instance().updateStatistic();
	for (auto iKey = m_planesKeys.begin(); iKey != m_planesKeys.end(); ++iKey)
		PicturesStorage::instance().markPointAsSuccess(*(*iKey)->pPair->pFirst, *(*iKey)->pPair->pSecond);

	return m_fZoom != 0;
}

void FloorDetector::dump(const std::list<KeyPairData *> &_planesKeys, std::list<KeyPair> &_pairs, const std::list<KeyPoint3D> *_pPoints1, const std::list<KeyPoint3D> *_pPoints2) {
#ifdef NOT_ANDROID
	ImageDump dump2(m_picture2.getBuf(), "/home/vfedosov/temp/out2.jpg", true);
	//ImageDump dump1(m_picture1.getBuf(), "/home/vfedosov/temp/out1.jpg");

	for (KeyPair &pair : _pairs) {
		//dump1.dumpKeypoint(*pair.pFirst);
		//dump1.number(pair.pFirst->screenPoint, pair.cId);
		dump2.dumpKeypoint(*pair.pSecond);
		dump2.number(pair.pSecond->screenPoint, pair.cId);
	}

	for (KeyPairData *pData1 : _planesKeys) {
		for (KeyPairData *pData2 : _planesKeys) {
			if (pData1->pPair == nullptr || pData2->pPair == nullptr)
				continue;

			//dump1.line(pData1->pPair->pFirst->screenPoint, pData2->pPair->pFirst->screenPoint);
			dump2.line(pData1->pPair->pSecond->screenPoint, pData2->pPair->pSecond->screenPoint);
		}
	}

	if (m_fZoom != 0) {
		static const Vector2DF points[] = {{g_nOneMeter - g_nOneMeter/5, g_nOneMeter/5}, {g_nOneMeter - g_nOneMeter/5, -g_nOneMeter/5},
			{g_nOneMeter + g_nOneMeter/5, -g_nOneMeter/5}, {g_nOneMeter + g_nOneMeter/5, g_nOneMeter/5}
		};

		Vector2DF source[4];
		Vector2DF recalculated[4];

		for (size_t c = 0; c != sizeof(points)/sizeof(Vector2DF); ++c) {
			Vector2DF v2 = _reclalculateToSecond(points[c]);
			const Vector3D first3D = m_plane1.get3DCoord(points[c]);
			source[c] = m_picture1.getFrustum().toScreenF(first3D);
			recalculated[c] = m_picture2.getFrustum().toScreenF(m_plane2.get3DCoord(v2));
			if (c > 0) {
				//dump1.line(source[c - 1].toInt(), source[c].toInt(), "", cv::Scalar(0, 0, 255));
				dump2.line(recalculated[c - 1].toInt(), recalculated[c].toInt(), "", cv::Scalar(0, 0, 255));
			}
		}

		//dump1.line(source[0].toInt(), source[3].toInt(), "", cv::Scalar(0, 0, 255));
		dump2.line(recalculated[0].toInt(), recalculated[3].toInt(), "", cv::Scalar(0, 0, 255));
	}

	if (_pPoints1 != nullptr)
		for (const KeyPoint3D &point : *_pPoints1)
			;//dump1.dumpKeypoint(point);

	if (_pPoints2 != nullptr)
		for (const KeyPoint3D &point : *_pPoints2)
			dump2.dumpKeypoint(point);
#endif //NDEBUG
}

FloorDetector::FloorDetector(const Picture &_picture1, const Picture &_picture2,
	const KeyPointsImagesComparer &_comparer, std::list<KeyPair> &_pairs, bool _bMustFind
) : m_picture1(_picture1), m_picture2(_picture2),
	m_plane1(PlaneWith2DCoordinates::createDefault(_picture1.getEarthPlane())),
	m_plane2(PlaneWith2DCoordinates::createDefault(_picture2.getEarthPlane())),
	m_comparer(_comparer)
{
	m_pairs.swap(_pairs);
	if (!m_pairs.empty() && !tryDetect(_comparer, m_pairs, false) && _bMustFind) {
		clear();
		tryDetect(_comparer, m_pairs, true);
	}
}
