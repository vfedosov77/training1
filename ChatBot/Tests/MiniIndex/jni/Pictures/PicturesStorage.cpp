#include "PicturesStorage.h"

void PicturesStorage::addPicture(Picture &_picture, DynamicContext *_pContext) {
	m_pictures.push_back(new Picture());
	Picture &newPicture = m_pictures.back().get();
	newPicture.swap(_picture);
	HessianTransform hessian(newPicture);
	hessian.findKeyPoints();
	std::list<KeyPoint3D> &points = m_picturesPoints[newPicture.getId()];
	hessian.calculateKeypoints3D(points, newPicture);
	const Frustum &localFrustum = newPicture.getFrustum();

	if (m_pictures.size() == 1) {
		m_cCurPictureId = newPicture.getId();
		pCurrentPicture = &m_pictures.back().get();
		const PicturePosition &defaultPos = PicturePosition::defaultPosition;
		m_floor = PlaneWith2DCoordinates::createDefault(newPicture.getEarthPlane());
		newPicture.setOrientedFrustum(OrientedFrustum(localFrustum.getFocalLength(), localFrustum.getWidth(), localFrustum.getHeight(),
			defaultPos.position, defaultPos.direction, defaultPos.topDirection)
		);

		if (_pContext != nullptr)
			_pContext->update(newPicture);
	} else
		assert(false);
}

void PicturesStorage::addPicture(byte *_pBuf, DynamicContext &_context) {
	Picture newPicture(_pBuf);
	addPicture(newPicture, &_context);
}

void PicturesStorage::fillGrid(DynamicContext &_context) {
	m_grid.clear();
	const OrientedFrustum &lastFrustum = _context.frustum;
	const std::list<KeyPoint3D> &points = getCurrentPoints();
	const Rect2D &screenRect = Application::getScreenRect();
	for (auto iPoint = points.begin(); iPoint != points.end(); ++iPoint) {
		static float fMaxDistance2 = 16.0f*g_nOneMeter*g_nOneMeter;
		const float fDistance2 = (lastFrustum.getPosition() - iPoint->point).length2();
		if (!lastFrustum.isOnScreen(iPoint->point) || fDistance2 > fMaxDistance2)
			continue;

		const Vector2D point = lastFrustum.toScreen(iPoint->point);
		iPoint->tagPoint = point;
		if (screenRect.containsInside(point))
			m_grid.add(&*iPoint);
	}
}

void PicturesStorage::markPointAsSuccess(const KeyPoint3D &_point, const KeyPoint3D &_matched) {
	if (m_bLockStatistic)
		return;

	++_point.statistic.cSuccessDetections;

	_point.nLastMatchLevelId = _matched.nLevelId;
	assert(_point.nLastMatchLevelId >= 0 &&_point.nLastMatchLevelId < int(g_cLevelsCount));
	const size_t cBackDetections = m_bestPoints.empty() ? std::numeric_limits<size_t>::max() : m_bestPoints.back()->statistic.cSuccessDetections;
	if (cBackDetections >= _point.statistic.cSuccessDetections) {
		if (m_bestPoints.size() < ms_cBestPointsCount)
			m_bestPoints.push_back(&_point);

		return;
	}

	bool bInserted = false;
	for (auto iPoint = m_bestPoints.begin(); iPoint != m_bestPoints.end(); ++iPoint) {
		if (!bInserted && (*iPoint)->statistic.cSuccessDetections > _point.statistic.cSuccessDetections) {
			m_bestPoints.insert(iPoint, &_point);
			bInserted = true;
		}

		if (*iPoint == &_point) {
			if (bInserted)
				m_bestPoints.erase(iPoint);

			return;
		}
	}

	if (m_bestPoints.size() > ms_cBestPointsCount)
		m_bestPoints.pop_back();
}

void PicturesStorage::updateStatistic() {
	static size_t s_cUpdateNum = 0;
	if (((s_cUpdateNum++) & 0xF) != 0)
		return;

	const std::list<KeyPoint3D> &points = getCurrentPoints();
	for (auto iPoint = points.begin(); iPoint != points.end(); ++iPoint)
		iPoint->statistic.cSuccessDetections /= 2;
}
