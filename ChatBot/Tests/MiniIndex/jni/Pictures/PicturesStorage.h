#ifndef _PICTURES_STORAGE_H_
#define _PICTURES_STORAGE_H_

#include <memory>

#include "Pictures/Picture.h"
#include "Pictures/Hessian.h"
#include "Pictures/Grid.h"
#include "Pictures/KeyPointsImagesComparer.h"
#include "Dynamic/DynamicContext.h"

class PicturesStorage {
	static const size_t ms_cBestPointsCount = 16;

	std::list<Holder<Picture> > m_pictures;
	std::map<size_t, std::list<KeyPoint3D> > m_picturesPoints;
	size_t m_cCurPictureId;
	PlaneWith2DCoordinates m_floor;
	Picture *pCurrentPicture;
	Grid m_grid;

	std::list<const KeyPoint3D *> m_bestPoints;
	bool m_bLockStatistic;

	PicturesStorage() : m_cCurPictureId(size_t(-1)), pCurrentPicture(nullptr), m_bLockStatistic(false) { }

	const Picture &getPictureById(size_t _cPictureId) const {
		for (auto iPic = m_pictures.begin(); iPic != m_pictures.end(); ++ iPic)
			if (iPic->get().getId() == _cPictureId)
				return iPic->get();

		assert(false);
		return m_pictures.front().get();
	}

	friend class KeyPointsIterator;
public:
	class StatisticLock {
	public:
		StatisticLock() {
			instance().m_bLockStatistic = true;
		}

		~StatisticLock() {
			instance().m_bLockStatistic = false;
		}
	};

	static
	PicturesStorage &instance() {
		static PicturesStorage instance;
		return instance;
	}

	//_picture will be destroyed.
	void addPicture(Picture &_picture, DynamicContext *_pContext = nullptr);
	void addPicture(byte *_pBuf, DynamicContext &_context);

	void updateStatistic();

	const PlaneWith2DCoordinates &getFloorPlane() const {
		assert(!m_pictures.empty());
		return m_floor;
	}

	void fillGrid(DynamicContext &_context);

	const Grid &getGrid() const {
		return m_grid;
	}

	Grid &getGrid() {
		return m_grid;
	}

	Picture &getCurrentBasePicture() {
		assert(pCurrentPicture != nullptr);
		return *pCurrentPicture;
	}

	const Picture &getCurrentBasePicture() const {
		assert(pCurrentPicture != nullptr);
		return *pCurrentPicture;
	}

	const std::list<KeyPoint3D> &getCurrentPoints() const {
		assert(m_cCurPictureId != size_t(-1));
		auto iPoints = m_picturesPoints.find(m_cCurPictureId);
		assert(iPoints != m_picturesPoints.end());
		return iPoints->second;
	}

	void markPointAsSuccess(const KeyPoint3D &_point, const KeyPoint3D &_matched);

	Vector2DF recalculateDirection(const Vector2D &_screenPointBase, const Vector2DF &_directionBase, DynamicContext &_context) const {
		const Picture &basePic = getCurrentBasePicture();
		const Frustum &baseFrustum = basePic.getFrustum();
		const Vector3D getDirEnd = baseFrustum.toPlane(Vector2DF(_screenPointBase) + _directionBase, basePic.getEarthPlane());

		const Vector2DF newScreenPoint = _context.frustum.toScreenF(baseFrustum.toPlane(_screenPointBase, basePic.getEarthPlane()));
		const Vector2DF newDirEnd = _context.frustum.toScreenF(getDirEnd);
		return newDirEnd - newScreenPoint;
	}
};

#endif //_PICTURES_STORAGE_H_
