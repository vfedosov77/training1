#include "BaseElements/GeometryFunctions.h"
#include "Picture.h"

size_t Picture::ms_cId = 0;
PicturePosition PicturePosition::defaultPosition = {{0, 0, 0}, {0, 0, 1}, {0, -1, 0}};

struct GradientDirection : public AngleComparableBase<Vector2D> {
	Vector2D direction;
	GradientDirection(const Vector2D &_vec) : AngleComparableBase(_vec), direction(_vec)
	{
		assert(!_vec.empty());
	}
};

struct GradientDirectionWithPoint : public AngleComparableBase<Vector2D> {
	Vector2D direction;
	Vector2D point;
	GradientDirectionWithPoint(const Vector2D &_vec, const Vector2D &_point) : AngleComparableBase(_vec), direction(_vec), point(_point)
	{
		assert(!_vec.empty());
	}
};

size_t fillLineGradientsByBresenham(const Edge &_line, int *_pArray, size_t _cHaarSize, const IntegralTransform &_integral) {
	const Vector2D vec = _line.getVector();
	const size_t cArraySize = std::max(std::abs(vec.x), std::abs(vec.y)) + 1;
	assert(cArraySize < GradientArrayMinMax::ms_cArraySize);
	BresenhamIterator it = BresenhamIterator::create(_line.v1, _line.v2);
	const Rect2D allowedRect(_cHaarSize, _cHaarSize, g_nWidth - _cHaarSize, g_nHeight - _cHaarSize);

	do {
		const Vector2D curPoint = it.point();
		if (allowedRect.contains(curPoint)) {
			const Vector2D calculated = _integral.calcGradVector(curPoint, _cHaarSize);
			*(_pArray++) = calculated.x*vec.x + calculated.y*vec.y;
		} else
			*(_pArray++) = 0;
	} while (it.moveNext());

	return cArraySize;
}

void calculateArrayAlongGrad(int _nKeyPointSize, const Vector2D &_point, GradientArrayMinMax &_gradient, const IntegralTransform &_integral) {
	const Vector2D &_grad = _gradient.direction;
	const int nHaarSize = (_nKeyPointSize + 2)/4;
	_gradient.cGradCount = fillLineGradientsByBresenham(Edge(_point - _grad, _point + _grad), _gradient.gradArrayX, nHaarSize, _integral);
	_gradient.updateMinMax();
}

#define cast_uint32_t static_cast<uint32_t>

float expLike(float _fVal) {
	assert(_fVal <= 0);
	static constexpr float fFirstY = 0.6;
	static constexpr float fFirstX = -0.510825624f;//::log(fFirstY);
	static constexpr float fFirstK = (1 - fFirstY)/fFirstX;
	static constexpr float fFirstB = 1;

	static constexpr float fSecondX = fFirstX*9;
	static constexpr float fSecondK = -(1 - fFirstY)/fSecondX;
	static constexpr float fSecondB = -fSecondK*fSecondX;

	if (_fVal >= fFirstX)
		return fFirstK*_fVal + fFirstB;

	return _fVal >= fSecondX ? fSecondK*_fVal + fSecondB : 0;
}

inline
Vector2D calcGradInPoint(const Vector2D &_point, const Vector2D &_centrePoint, int _nLevelId, const IntegralTransform &_integral, int _nWidth, int _nHeigth) {
	const float fDist2 = (_point - _centrePoint).length2();
	const float fGaussianPart = expLike(fDist2*g_fLevelGaussiaExpMultiplier[_nLevelId]);

	if (fGaussianPart == 0)
		return Vector2D();

	const int nStep =  pLevelZooms[_nLevelId];
	if (_point.x < nStep || _point.y < nStep || _point.x >= _nWidth - nStep || _point.y >= _nHeigth - nStep)
		return Vector2D();

	int nX = _integral.calcVertHaar(_point, nStep)*32;
	int nY = _integral.calcHorHaar(_point, nStep)*32;

	float fWeigth = fGaussianPart*g_fLevelGaussiaMultipliers[_nLevelId];
	return Vector2D(round(nX*fWeigth), round(nY*fWeigth));
}

void calcGradients(const Vector2D &_point, int _nLevelId, const IntegralTransform &_integral, int _nWidth, int _nHeigth, std::vector<Vector2D> &_bestSumms) {
	const int nStep = pLevelZooms[_nLevelId];
	const int nThirdOfStep = nStep/3;
	const int nRadius = nStep*4;
	std::vector<GradientDirection> gradients;

	static const int nLevelThresholds[] = {12, 36, 64, 108, 184, 300};
	const int nThreshold = nLevelThresholds[_nLevelId]*16;
	for (int nDX = -nRadius; nDX <= nRadius; nDX += nThirdOfStep) {
		for (int nDY = -nRadius; nDY <= nRadius; nDY += nThirdOfStep) {
			const int nX = _point.x - nDX;
			const int nY = _point.y - nDY;
			const Vector2D vec = calcGradInPoint(Vector2D(nX, nY), _point, _nLevelId, _integral, _nWidth, _nHeigth);

			if (vec.length2() > nThreshold)
				gradients.push_back(GradientDirection(vec));
		}
	}

	if (gradients.empty())
		return;

	std::sort(gradients.begin(), gradients.end());
	static const Vector2D rotateBy(fast_floor(1024*::cos(1.0f) + 0.5), fast_floor(1024*::sin(1.0f) + 0.5));
	std::list<GradientDirection *> dirs;
	Vector2D summ;
	std::vector<GradientDirection>::const_iterator iEnd;
	_bestSumms.push_back(Vector2D(0, 0));
	bool bWasLowLevel = false;
	int64_t lnLastLen2 = 0;
	//Move the window with size 1 rad and change longest summ vector in this window.
	bool bFirstRemoved = false;
	for (auto iDir = gradients.begin(); iDir != iEnd; ++iDir) {
		if (iDir == gradients.end()) {
			iDir = gradients.begin();
			if (!bFirstRemoved) {
				bFirstRemoved = true;
				iEnd = gradients.end();
			}
		}

		summ += iDir->direction;
		dirs.push_back(&*iDir);
		const Vector2D &cur = iDir->direction;
		Vector2D rotated = rotateByNormalizedVector(cur.length2() > 0xFFFF ? cur : cur*256, rotateBy);

		while (rotated.isOnLeftSide(dirs.front()->direction) == -1 || cur.isOnLeftSide(dirs.front()->direction) == 1) {
			assert(dirs.size() > 1);
			const Vector2D &last = dirs.front()->direction;
			summ -= last;
			dirs.pop_front();

			if (!bFirstRemoved) {
				bFirstRemoved = true;
				iEnd = iDir;
			}
		}

		assert(!dirs.empty());

		if (!bFirstRemoved)
			continue;

		const int64_t lnLen2 = summ.length2();
		if (lnLen2 > lnLastLen2 || (bWasLowLevel && lnLen2 > lnLastLen2*2/3)) {
			if (bWasLowLevel)
				_bestSumms.push_back(summ);
			else
				_bestSumms.back() = summ;

			lnLastLen2 = lnLen2;
			bWasLowLevel = false;
		} else if (lnLen2 < lnLastLen2/3)
			bWasLowLevel = true;
	}

	std::sort(_bestSumms.begin(), _bestSumms.end(), [](const Vector2D &v1, const Vector2D &v2) {return v1.length2() < v2.length2();});
	std::vector<uint64_t> lens;
	for (auto iv = _bestSumms.begin(); iv != _bestSumms.end(); ++iv)
		lens.push_back(iv->length2());

	auto iFrom = std::lower_bound(lens.begin(), lens.end(), lens.back()/2);
	assert(iFrom != lens.end());
	_bestSumms.erase(_bestSumms.begin(), _bestSumms.begin() + (iFrom - lens.begin()));
	assert(_bestSumms.empty() || !_bestSumms.front().empty());
}

Vector2D calcGradient(const Vector2D &_point, int _nLevelId, const IntegralTransform &_integral, int _nWidth, int _nHeigth,
	const Vector2DF &_prevDirection
) {
	const int nStep = pLevelZooms[_nLevelId];
	const int nPartOfStep = nStep/2;
	const int nRadius = nStep*4;
	std::vector<GradientDirectionWithPoint> gradients;

	static const int nLevelThresholds[] = {12, 36, 64, 108, 184, 300};
	const int nThreshold = nLevelThresholds[_nLevelId]*16;

	for (int nDX = -nRadius; nDX <= nRadius; nDX += nPartOfStep) {
		for (int nDY = -nRadius; nDY <= nRadius; nDY += nPartOfStep) {
			const int nX = _point.x - nDX;
			const int nY = _point.y - nDY;
			const Vector2D vec = calcGradInPoint(Vector2D(nX, nY), _point, _nLevelId, _integral, _nWidth, _nHeigth);

			if (vec.length2() > nThreshold)
				gradients.push_back(GradientDirectionWithPoint(vec, Vector2D(nDX, nDY)));
		}
	}

	if (gradients.empty())
		return Vector2D();

	static const Vector2D rotateBy1Rad(fast_floor(1024*::cos(1.0f) + 0.5), fast_floor(1024*::sin(1.0f) + 0.5));

	static const size_t cRadiansCountInCircle = 6;
	Vector2D prevDirection = (_prevDirection*32).toInt();
	Vector2D radiansVectors[cRadiansCountInCircle];
	radiansVectors[0] = prevDirection;
	radiansVectors[1] = rotateByNormalizedVector(radiansVectors[0], rotateBy1Rad);
	radiansVectors[2] = rotateByNormalizedVector(radiansVectors[1], rotateBy1Rad);
	radiansVectors[3] = rotateByNormalizedVector(radiansVectors[2], rotateBy1Rad);
	radiansVectors[4] = rotateByNormalizedVector(radiansVectors[3], rotateBy1Rad);
	radiansVectors[5] = rotateByNormalizedVector(radiansVectors[4], rotateBy1Rad);

	Vector2D radiansCollinearSums[cRadiansCountInCircle];

	for (auto iDir = gradients.begin(); iDir != gradients.end(); ++iDir) {
		for (size_t c = 0; c != cRadiansCountInCircle; ++c) {
			if (radiansVectors[c].dotProduct32Bits(iDir->direction) > 0 && getTanAngle(radiansVectors[c], iDir->direction) <= 0.578)
				radiansCollinearSums[c] += iDir->direction;
		}
	}

	const int64_t lnMaxLength = radiansCollinearSums[0].length2()*9;
	for (size_t c = 1; c != cRadiansCountInCircle; ++c)
		if (radiansCollinearSums[c].length2() > lnMaxLength)
			return Vector2D();

	Vector2D summVec;
	for (auto iDir = gradients.begin(); iDir != gradients.end(); ++iDir) {
		if (radiansCollinearSums[0].dotProduct32Bits(iDir->direction) > 0 && getTanAngle(radiansCollinearSums[0], iDir->direction) <= 0.578)
			summVec += iDir->direction;
	}

	return summVec;
}


void Picture::calcDescriptor(KeyPoint3D &_kp, const LevelData &_data, const Vector2DF &_prevDir) const {
	assert(_kp.gradientDirection.empty());
	if (!_prevDir.isNotValid()) {
		const Vector2D summ = calcGradient(_kp.screenPoint, _kp.nLevelId, m_integral, g_nWidth, g_nHeight, _prevDir);
		if (summ.empty())
			return;

		_kp.gradientDirection.push_back(Gradient());
		if (!fillGradient(_kp, _kp.gradientDirection.back(), summ, _data))
			_kp.gradientDirection.pop_back();

		return;
	}

	std::vector<Vector2D> bestSumms;
	calcGradients(_kp.screenPoint, _kp.nLevelId, m_integral, g_nWidth, g_nHeight, bestSumms);

	if (bestSumms.empty())
		return;

	_kp.gradientDirection.push_back(Gradient());

	for (size_t c = 0, cCount = bestSumms.size(); c != cCount; ++c) {
		const bool bLast = c == cCount - 1;
		if (fillGradient(_kp, _kp.gradientDirection.back(), bestSumms[c], _data)) {
			if (!bLast)
				_kp.gradientDirection.push_back(Gradient());
		} else if (bLast)
			_kp.gradientDirection.pop_back();
	}
}

bool Picture::fillGradient(const KeyPoint3D &_kp, Gradient &_grad, const Vector2D &_direction, const LevelData &_data) const {
	const Vector2DF gradF = _kp.getGradVecF(_direction);
	//Rotate 90 deg because true invariant direction is direction that is orthogonal to gradient.
	const Vector2DF ortF(gradF.y, -gradF.x);
	const Vector2DF recalculatedGradF = m_frustum.getOrtDirrection(_kp.screenPoint, ortF, m_earthPlane);

	GradientArrayMinMax minMax;
	if (!recalculatedGradF.empty()) {
		_grad.fOrtZoom = _kp.getGradVecSize()/recalculatedGradF.length();
		minMax.ort = ortF.toInt();
		_grad.ortF = ortF;
		_grad.directionF = _kp.getGradVecF(recalculatedGradF);
		minMax.direction = _grad.directionF.toInt();
	} else {
		_grad.directionF = gradF;
		minMax.direction = gradF.toInt();
		_grad.fOrtZoom = 0;
		return false;
	}

	calculateArrayAlongGrad(_kp.nSize, _kp.screenPoint, minMax, m_integral);
	if (minMax.cGradCount == 0)
		return false;

	_grad.update(_kp, minMax, _data, m_integral);
	return true;
}

void Picture::fillLineGradients(const Edge &_line, float *pArray, size_t _cSize) const {
	const Vector2D vec = _line.getVector().getNormalized();
	const float fStepX = float(_line.v2.x - _line.v1.x)/(_cSize + 5);
	const float fStepY = float(_line.v2.y - _line.v1.y)/(_cSize + 5);
	float fX = _line.v1.x + 0.5 + 3*fStepX;
	float fY = _line.v1.y + 0.5 + 3*fStepY;

	for (const float *pEnd = pArray + _cSize; pArray != pEnd; ++pArray) {
		Vector2D grad = m_integral.calcGradVector(Vector2D(fast_floor(fX), fast_floor(fY)), 8);
		grad = rotateByNormalizedVector(grad, vec);
		*pArray = grad.x;
		fX += fStepX;
		fY += fStepY;
	}
}
