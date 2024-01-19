#ifndef _DYNAMIC_CONTEXT_H_
#define _DYNAMIC_CONTEXT_H_

#include "BaseElements/Primitives3D.h"
#include "BaseElements/Frustum.h"
#include "Pictures/Picture.h"
#include "FloorDetector.h"

class TracingPointData : public Counted{
	TracingPointData &operator=(const TracingPointData &_other);
	TracingPointData(const TracingPointData &_other);
public:
	TracingPointData() : pBasePoint(nullptr), prevPictureDirection(Vector2DF::ms_notValidVector) { }

	const KeyPoint3D *pBasePoint;
	Vector2D prevPicturePoint;
	Vector2DF prevPictureDirection;
	FloorDetector::KeyPairData detected;
	std::list<KeyPair> candidates;
	std::set<Vector2D> calculatedGrads;

	void updatePrevPictureData(const KeyPoint3D *_pPoint) {
		prevPicturePoint = _pPoint->screenPoint;
		if (_pPoint->gradientDirection.size() != 1)
			prevPictureDirection = Vector2DF::ms_notValidVector;
		else {
			const Vector2DF &ort = _pPoint->gradientDirection.front().ortF;
			prevPictureDirection = Vector2DF(-ort.y, ort.x);
		}
	}
};

struct ComparedPair {
	Vector2D p1, p2;

	ComparedPair(const Vector2D &_point1, const Vector2D &_point2) {
		if (_point1 < _point2) {
			p1 = _point1;
			p2 = _point2;
		} else {
			p2 = _point1;
			p1 = _point2;
		}
	}

	bool operator <(const ComparedPair &_other) const {
		return p1 == _other.p1 ? p2 < _other.p2 : p1 < _other.p1;
	}
};

struct DynamicContext {
	OrientedFrustum frustum;
	Vector3D positionMoving;
	Vector3D directionMoving;

	std::vector<Holder<TracingPointData> > tracingPoints;
	//std::shared_ptr<Picture> lastPicture;

	std::list<KeyPoint3D> pointsBuffer;
	std::set<ComparedPair> comparedPairs;
	size_t cNotDetectedCount;

	struct TemporaryDataLock {
		DynamicContext &context;
		TemporaryDataLock(DynamicContext &_context) : context(_context) { }
		~TemporaryDataLock() {
			context.clearTemporaryData();
		}
	};

	void testConsistency() {
		for (auto iData1 = tracingPoints.begin(); iData1 != tracingPoints.end(); ++iData1)
			for (auto iData2 = tracingPoints.begin(); iData2 != tracingPoints.end(); ++iData2)
				if (iData1 != iData2)
					assert(iData1->get().pBasePoint->screenPoint != iData2->get().pBasePoint->screenPoint);
	}

	DynamicContext() : cNotDetectedCount(0) {

	}

	void update(const Picture &_currentPicture) {
		const OrientedFrustum &newFrustum = _currentPicture.getOrientedFrustum();
		frustum = newFrustum;
//		if (!frustum.getPosition().isNotValid()) {
//			positionMoving = positionMoving/2 + newFrustum.getPosition() - frustum.getPosition();
//		}
	}

	void clearTemporaryData() {
		for (auto iPoint = tracingPoints.begin(); iPoint != tracingPoints.end(); ++iPoint) {
			iPoint->get().detected.invalidate();
			iPoint->get().candidates.clear();
			iPoint->get().calculatedGrads.clear();
		}

		comparedPairs.clear();
		pointsBuffer.clear();
	}
};

#endif //_DYNAMIC_CONTEXT_H_
