#include "KeyPoint.h"

int KeyPointsGrid::getCellAndNeihbours(int _nCellX, int _nCellY, GridCell *(&_cells)[9]) {
	assert(_nCellX >= 0 && _nCellX < ms_nWidth && _nCellY >= 0 && _nCellY < ms_nHeight);
	const int nId = _nCellX + _nCellY*ms_nWidth;
	GridCell *pCurCell = m_cells + nId;
	_cells[0] = pCurCell;

	int nCurId = 1;
	const bool bLeftOut = _nCellX == 0;
	const bool bRigthOut = _nCellX == ms_nWidth - 1;
	const bool bTopOut = _nCellY == 0;
	const bool bBottomOut = _nCellY == ms_nHeight - 1;

	if (!bLeftOut) {
		_cells[nCurId++] = pCurCell - 1;
		if (!bTopOut)
			_cells[nCurId++] = pCurCell - 1 - ms_nWidth;
		if (!bBottomOut)
			_cells[nCurId++] = pCurCell - 1 + ms_nWidth;
	}

	if (!bRigthOut) {
		_cells[nCurId++] = pCurCell + 1;
		if (!bTopOut)
			_cells[nCurId++] = pCurCell + 1 - ms_nWidth;
		if (!bBottomOut)
			_cells[nCurId++] = pCurCell + 1 + ms_nWidth;
	}

	if (!bTopOut)
		_cells[nCurId++] = pCurCell - ms_nWidth;
	if (!bBottomOut)
		_cells[nCurId++] = pCurCell + ms_nWidth;

	assert(nCurId <= 9);
	if (nCurId != 9)
		_cells[nCurId] = nullptr;

	return nCurId;
}

void KeyPointsGrid::removeMinorPointsAndCollectPointsForLines(std::list<KeyPoint> &_pointsForLinesDetection) {
	for (int i = 0; i < ms_nWidth; ++i) {
		for (int j = 0; j < ms_nHeight; ++j) {
			int nId = i + j*ms_nWidth;
			GridCell &cell = m_cells[nId];
			for (auto it = cell.points.begin(); it != cell.points.end();) {
				if (std::abs(it->nValue) < cell.nThreshold)
					it = cell.points.erase(it);
				else
					++it;
			}
		}
	}

	const int nMaxDistance2 = 400;
	const int nMaxLinePointSize = 15;
	const int nLinesDetectionThreshold = 6000;

	for (int i = 0; i < ms_nWidth; ++i) {
		for (int j = 0; j < ms_nHeight; ++j) {
			const int nId = i + j*ms_nWidth;
			GridCell &cell = m_cells[nId];
			GridCell *neighbours[9];
			const int nCount = getCellAndNeihbours(i, j, neighbours);

			for (auto it1 = cell.points.begin(); it1 != cell.points.end();) {
				const Vector2D &curPoint = it1->point;
				const int nCurVal = std::abs(it1->nValue);
				const int nLowBound = nCurVal*3/2;
				bool bRemoved = false;

				for (int nCell = 0; nCell < nCount; ++nCell) {
					GridCell &neighbour = *(neighbours[nCell]);
					const Rect2D &cellRect = neighbour.rect;

					if (curPoint.distance2(cellRect.topLeft()) >= nMaxDistance2 &&
						curPoint.distance2(cellRect.bottomLeft()) >= nMaxDistance2 &&
						curPoint.distance2(cellRect.topRigth()) >= nMaxDistance2 &&
						curPoint.distance2(cellRect.bottomRigth()) >= nMaxDistance2
					)
						continue;

					for (auto it2 = neighbour.points.begin(); it2 != neighbour.points.end(); ++it2) {
						const int nDist2 = curPoint.distance2(it2->point);
						if (it1 == it2 || nDist2 >= nMaxDistance2)
							continue;


						const int nOtherVal = std::abs(it2->nValue);

						if (nOtherVal < nLowBound)
							continue;

						if (nCurVal < nOtherVal*(512 - nDist2)/512.0f) {
							it1 = cell.points.erase(it1);
							bRemoved = true;
							break;
						}
					}

					if (bRemoved)
						break;
				}

				if (!bRemoved) {
					KeyPoint &kp = *it1;

					if (kp.nSize <= nMaxLinePointSize && std::abs(kp.nValue) > nLinesDetectionThreshold)
						_pointsForLinesDetection.push_back(kp);

					++it1;
				}
			}
		}
	}
}

void KeyPointsGrid::getEdgePoints(const Edge &_edge, bool _bNearest, const Plane &_plane, std::list<KeyPoint3D> &_result, const Frustum &_frustum, const class Picture *_pPicture) const {
	Edge3D edge = _frustum.toPlane(_edge, _plane);

	if ((edge.v2.x > edge.v1.x) == _bNearest)
		std::swap(edge.v1, edge.v2);

	const Vector3D norm = (edge.v2 - edge.v1).getNormalized();

	if (edge.v2.z + norm.z*g_nOneMeter > g_nOneMeter/5)
		edge.v2 += norm*g_nOneMeter;

	if (edge.v1.z - norm.z*g_nOneMeter > g_nOneMeter/5)
		edge.v1 -= norm*g_nOneMeter;

	//const Vector3D ort = earth.normal.crossProduct (norm);
	const GridCell *pEnd = m_cells + ms_nSize;
	for (const GridCell *iCell = m_cells; iCell != pEnd; ++iCell) {
		const GridCell &cell = *iCell;
		for (auto iPoint = cell.points.begin(); iPoint != cell.points.end(); ++iPoint) {
			const KeyPoint &kp = *iPoint;
			KeyPoint3D kp3d(kp, _plane, _frustum, _pPicture);
			kp3d.nDistance2 = edge.distance2ToLine(kp3d.point);

			if (kp3d.nDistance2 > g_nOneMeter2)
				continue;

			const Vector3D cross = (kp3d.point - edge.v1).crossProduct(edge.getVector());
			if (cross*_plane.normal > 0)
				continue;

			_result.push_back(kp3d);
		}
	}
}
