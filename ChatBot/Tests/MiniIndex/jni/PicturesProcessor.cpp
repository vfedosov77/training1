#include <fstream>
#include <iostream>

#include "Dynamic/DynamicContext.h"
#include "Pictures/KeyPointsImagesComparer.h"
#include "Pictures/PicturesStorage.h"
#include "Pictures/PointsIterators.h"

#include "Draw/Dumper.h"

#include "PicturesProcessor.h"

#ifdef NOT_ANDROID
#include <opencv2/opencv.hpp>
#include <opencv2/legacy/legacy.hpp>
#include <opencv2/features2d/features2d.hpp>
#endif

#define IMAGE_SIZE 460808

namespace {
static const size_t g_cTracingCount = 5;
static const Vector2D g_cellsArray[] = {{5, 5}, {4, 5}, {3, 5}, {6, 5}, {7, 5}, {2, 6}, {7, 6}, {5, 4}, {3, 4}, {6, 4}, {4, 6}, {5, 3}};
}

int nReceived = 0;


//static
//void saveJPG(byte *_pArray, std::string _stdName) {
//	cv::Size sz(640, 480);
//	cv::Mat image(sz, CV_8U, _pArray);
//	cv::imwrite(_stdName, image);
//}

Picture *createPicture(const std::string &_strName) {
	return new Picture(std::string("/home/vfedosov/temp/") + _strName);
}

Picture *createPicture(byte *_pBuf) {
	Picture *pPicture = new Picture(_pBuf);
	return pPicture;
}



class CellsArrayIterator {
	Grid &m_grid;
	const Vector2D *m_pEnd;
	const Vector2D *m_pCurrent;

public:
	CellsArrayIterator(Grid &_grid) : m_grid(_grid), m_pEnd(g_cellsArray + sizeof(g_cellsArray)/sizeof(Vector2D)),
		m_pCurrent(g_cellsArray)
	{

	}

	bool moveNext() {
		assert(m_pCurrent < m_pEnd);
		++m_pCurrent;
		return m_pCurrent != m_pEnd;
	}

	Grid::GridCell &current() {
		assert(m_pCurrent < m_pEnd);
		return m_grid.getCell(m_pCurrent->x, m_pCurrent->y);
	}
};

class KeyPointsIterator {
	static const int ms_nMaxCellsDistance = 1;

	DynamicContext &m_context;

	CellsArrayIterator m_iterator;
	bool m_bInitialized;
	std::list<const KeyPoint3D *>::iterator m_iBestPoint;
	bool m_bHasPoints;
	bool _initialize() {
		static const Vector2D s_startPoint(320, 380);
		PicturesStorage::instance().fillGrid(m_context);
		PicturesStorage::instance().getGrid().sortCellsPoints();
		//m_iterator = PicturesStorage::instance().getGrid().getNearestCells(s_startPoint, ms_nMaxCellsDistance);

		while (m_iterator.current().points.empty() && (m_bHasPoints = m_iterator.moveNext()))
			;

		m_bInitialized = true;
		return m_bHasPoints;
	}

	bool _moveNextBestPoint() {
		const size_t cMinDetected = 10;
		std::list<const KeyPoint3D *> &bestPoints = PicturesStorage::instance().m_bestPoints;
		if (bestPoints.front()->statistic.cSuccessDetections < cMinDetected) {
			m_iBestPoint = bestPoints.end();
			return _initialize();
		}

		while (m_iBestPoint != bestPoints.end()) {
			if (m_context.frustum.isOnScreen((*m_iBestPoint)->point)) {
				(*m_iBestPoint)->tagPoint = m_context.frustum.toScreen((*m_iBestPoint)->point);
				return true;
			}

			++m_iBestPoint;
		}

		return _initialize();
	}

public:
	KeyPointsIterator(DynamicContext &_context) : m_context(_context), m_iterator(PicturesStorage::instance().getGrid()),
		m_bInitialized(false), m_iBestPoint(PicturesStorage::instance().m_bestPoints.begin()), m_bHasPoints(true)
	{
//		if (m_iBestPoint != PicturesStorage::instance().m_bestPoints.end())
//			++m_iBestPoint;

		_moveNextBestPoint();
	}

	bool eof() const {
		return !m_bHasPoints;
	}

	const KeyPoint3D *current() {
		if (m_iBestPoint != PicturesStorage::instance().m_bestPoints.end())
			return *m_iBestPoint;

		assert(!m_iterator.current().points.empty());
		return m_iterator.current().points.front();
	}

	bool moveNext() {
		if (!m_bInitialized) {
			++m_iBestPoint;
			return _moveNextBestPoint();
		}

		while ((m_bHasPoints = m_iterator.moveNext()) && m_iterator.current().points.empty())
			;

		return m_bHasPoints;
	}
};

int nAllKey = 0;
int nFoundKeys = 0;
int nStaticCompares = 0;

static
void _findTracingPoints(DynamicContext &_context, Picture &_current, KeyPointsImagesComparer &_comparer, bool *approved, HessianTransform &_hessian,
	FloorDetector &_floorDetector, int &_nFound
) {
	_nFound = 0;
	nAllKey += _context.tracingPoints.size();
	std::vector<Holder<TracingPointsIterator>> iterators;
	iterators.reserve(_context.tracingPoints.size());

	for (auto iTracing = _context.tracingPoints.begin(); iTracing != _context.tracingPoints.end(); ++iTracing)
		iterators.push_back(Holder<TracingPointsIterator>(new TracingPointsIterator(iTracing->get(), _hessian, true, _comparer, _current, _floorDetector, _context, true)));

	bool bHasPoint = false;
	size_t cApprovedCount = 0;
	do {
		bHasPoint = false;
		for (size_t c = 0; c != iterators.size(); ++c) {
			TracingPointsIterator &it = iterators[c].get();
			if (approved[c])
				continue;

			if (it.isApproved()) {
				approved[c] = true;
				++cApprovedCount;
			}

			if (!it)
				continue;

			bHasPoint = true;

			if (it.isDetected()) {
				for (size_t k = 0; k != iterators.size(); ++k)
					if (k != c && !iterators[k]->isApproved()) {
						iterators[k]->rememberAndReset();
						if (!iterators[k]->isApproved())
							iterators[k]->moveToRemembered();
					}
			}

			if (!it.isApproved())
				++it;
		}
	} while(bHasPoint);

	_nFound = cApprovedCount;
	nFoundKeys += cApprovedCount;
}


static
void _tryAddTracingPoints(DynamicContext &_context, Picture &_current, KeyPointsImagesComparer &_comparer, HessianTransform &_hessian,
	FloorDetector &_floorDetector, bool _bFrustumDetected
) {
	size_t cTraceSize = _context.tracingPoints.size();
	if (cTraceSize < g_cTracingCount) {
		KeyPointsIterator iKeyPoints(_context);
		do {
			if (iKeyPoints.eof())
				return;
			_context.tracingPoints.push_back(Holder<TracingPointData>(new TracingPointData()));

			do {
				const KeyPoint3D &curPoint = *iKeyPoints.current();
				bool bAppropriate = true;
				for (size_t c = 0; c != _context.tracingPoints.size() - 1; ++c)
					if ((_context.tracingPoints[c]->pBasePoint->screenPoint - curPoint.screenPoint).length2() <= 16) {
						bAppropriate = false;
						break;
					}

				if (!bAppropriate)
					continue;

				TracingPointData &tracing = _context.tracingPoints.back().get();
				assert(!tracing.detected.isValid());
				tracing.pBasePoint = &curPoint;
				tracing.candidates.clear();
				tracing.prevPicturePoint = curPoint.tagPoint;

				tracing.prevPictureDirection = Vector2DF::ms_notValidVector;
				if (curPoint.gradientDirection.size() == 1) {
					Vector2DF newOrt = PicturesStorage::instance().recalculateDirection(curPoint.screenPoint, curPoint.gradientDirection.front().ortF, _context);
					tracing.prevPictureDirection.x = -newOrt.y;
					tracing.prevPictureDirection.y = newOrt.x;
				}

				TracingPointsIterator iTracing(tracing, _hessian, false, _comparer, _current, _floorDetector, _context, !_bFrustumDetected);

				while (iTracing && !iTracing.isApproved())
					++iTracing;

				if (iTracing.isApproved())
					_context.tracingPoints.push_back(Holder<TracingPointData>(new TracingPointData()));

			} while(iKeyPoints.moveNext() && _context.tracingPoints.size() <= g_cTracingCount);

			_context.tracingPoints.pop_back();

			if (_context.tracingPoints.size() < 2)
				break;

			std::list<FloorDetector::KeyPairData *> planesKeys;

			for (auto iTracing = _context.tracingPoints.begin(); iTracing != _context.tracingPoints.end(); ++iTracing)
				planesKeys.push_back(&iTracing->get().detected);

			OrientedFrustum frustum;
			const int nRemoved = _floorDetector.detectPlanesRelationsWithFilter(planesKeys, frustum);

			if (nRemoved == -1)
				break;

			assert(nRemoved >= 0 && nRemoved < int(_context.tracingPoints.size()));

//			{
//			TracingPointData &data = _context.tracingPoints[nRemoved];
//			ImageDump dump1(*data.pBasePoint, "/home/vfedosov/temp/out1.jpg");
//			ImageDump dump2(*data.detected.pPair->pSecond, "/home/vfedosov/temp/out2.jpg");

//			planesKeys.clear();
//			for (TracingPointData &data : _context.tracingPoints)
//				//if (&data != &_context.tracingPoints.back())
//					planesKeys.push_back(&data.detected);

//			_floorDetector.detectPlanesRelations(planesKeys);
//			_floorDetector.calculateFrustum(frustum);

//			for (size_t c = 0; c != _context.tracingPoints.size(); ++c) {
//				if (c == nRemoved)
//					continue;
//				FloorDetector::KeyPairData &data = _context.tracingPoints[c].detected;
//				dump1.dumpKeypoint(*data.pPair->pFirst, false);
//				dump2.dumpKeypoint(*data.pPair->pSecond, false);
//				//dump2.dumpKeypoint(c >= cTraceSize ? data.pPair->pFirst->tagPoint : _context.tracingPoints[c].prevPicturePoint, c);
//				dump2.dumpKeypoint(frustum.toScreen(data.pPair->pFirst->point), c);
//			}

//			//dump2.dumpKeypoint(curPoint.tagPoint);
//			dump2.dumpKeypoint(frustum.toScreen(data.pBasePoint->point));
//			}
			_context.tracingPoints.erase(_context.tracingPoints.begin() + nRemoved);
			cTraceSize = _context.tracingPoints.size();
		} while(true);
	}
}

static
bool _tryDetectFrustum(DynamicContext &_context, FloorDetector &_floorDetector, std::list<KeyPair> &_pairs, Picture &_current) {
	if (_context.tracingPoints.size() >= 2) {
		std::list<FloorDetector::KeyPairData *> planesKeys;

		for (auto iTracing = _context.tracingPoints.begin(); iTracing != _context.tracingPoints.end(); ++iTracing)
			planesKeys.push_back(&iTracing->get().detected);

		_floorDetector.detectPlanesRelations(planesKeys);
		//_floorDetector.dump(planesKeys, _pairs);
		if (_floorDetector.detected()) {
			_floorDetector.calculateFrustum(_context.frustum);
			_current.setOrientedFrustum(_context.frustum);
			return true;
		}
	}
	return false;
}

static
bool _updateContext(DynamicContext &_context, FloorDetector &_floorDetector, std::list<KeyPair> &_pairs, Picture &_current) {
	if (_tryDetectFrustum(_context, _floorDetector, _pairs, _current)) {
		PicturesStorage::instance().updateStatistic();

		for (auto iTracing = _context.tracingPoints.begin(); iTracing != _context.tracingPoints.end(); ++iTracing) {
			assert(iTracing->detected.pPair != nullptr);
			PicturesStorage::instance().markPointAsSuccess(*iTracing->get().pBasePoint, *iTracing->get().detected.pPair->pSecond);
		}

		return true;
	}

	return false;
}

bool updateTracingPoints(DynamicContext &_context, Picture &_current, KeyPointsImagesComparer &_comparer) {
	assert(_context.tracingPoints.size() <= g_cTracingCount);
	bool approved[g_cTracingCount] = {false};

	DynamicContext::TemporaryDataLock lock(_context);
	Rect2D emptyRect;
	HessianTransform hessian(_current, emptyRect);
	std::list<KeyPair> pairs;
	FloorDetector floorDetector(PicturesStorage::instance().getCurrentBasePicture(), _current, _comparer, pairs);

	int nFoundNew;
	_findTracingPoints(_context, _current, _comparer, approved, hessian, floorDetector, nFoundNew);

	for (int n = int(_context.tracingPoints.size()) - 1; n >= 0; --n)
		if (!approved[n])
			_context.tracingPoints.erase(_context.tracingPoints.begin() + n);

	if (_context.tracingPoints.empty())
		return false;

	const bool bFrustumDetected = _context.tracingPoints.size() >= 2 ? _tryDetectFrustum(_context, floorDetector, pairs, _current) : false;
	_tryAddTracingPoints(_context, _current, _comparer, hessian, floorDetector, bFrustumDetected);
	return _updateContext(_context, floorDetector, pairs, _current);
}

FloorDetector *staticCompare(Picture &_picture, KeyPointsImagesComparer &_imagesComparer) {
	std::list<KeyPoint3D> points;
	std::list<KeyPair> pairs;

	HessianTransform hessian(_picture);
	hessian.findKeyPoints();
	hessian.calculateKeypoints3D(points, _picture);

	const std::list<KeyPoint3D> &basePoints = PicturesStorage::instance().getCurrentPoints();

	for (auto iKp1 = basePoints.begin(); iKp1 != basePoints.end(); ++iKp1) {
		for (auto iKp2 = points.begin(); iKp2 != points.end(); ++iKp2) {
			KeyPair pair;
			if (_imagesComparer.compare(*iKp1, *iKp2, &pair)) {
				pairs.push_back(pair);
				break;
			}
		}
	}

	FloorDetector *pDetector = new FloorDetector(PicturesStorage::instance().getCurrentBasePicture(), _picture, _imagesComparer, pairs);
	pDetector->getPointsBuffer().swap(points);
	return pDetector;
}

FloorDetector *staticCompare(Picture &_picture1, Picture &_picture2) {
	PicturesStorage::instance().addPicture(_picture1);
	KeyPointsImagesComparer comparer(PicturesStorage::instance().getCurrentBasePicture(), _picture2);
	return staticCompare(_picture2, comparer);
}

std::vector<Vector2DF> g_resultPoints;

void fillCalculatedData(byte *_pBuf, const OrientedFrustum &_frustum, bool _bDetected) {
	if (_bDetected) {
		static const Vector2DF points[] = {{g_nOneMeter - g_nOneMeter/5, g_nOneMeter/5}, {g_nOneMeter - g_nOneMeter/5, -g_nOneMeter/5},
			{g_nOneMeter + g_nOneMeter/5, -g_nOneMeter/5}, {g_nOneMeter + g_nOneMeter/5, g_nOneMeter/5}
		};

		g_resultPoints.resize(4);
		PlaneWith2DCoordinates plane = PlaneWith2DCoordinates::createDefault(PicturesStorage::instance().getCurrentBasePicture().getEarthPlane());
		//ImageDump dump(_pBuf);
		for (size_t c = 0; c != sizeof(points)/sizeof(Vector2DF); ++c)
			g_resultPoints[c] = Vector2DF(_frustum.toScreen(plane.get3DCoord(points[c])));
	} else
		g_resultPoints.clear();
}

void testMatch(byte *_pBuf, int _nSize) {
	if (_nSize != IMAGE_SIZE && _nSize != -1)
		return;

	static DynamicContext context;
	static DynamicContext lastSuccess;
//	nReceived ++;
//	std::stringstream stream;
//	stream << "/home/vfedosov/temp/test/test" << nReceived;
//	std::ofstream stream0(stream.str().data(), std::ios_base::out | std::ios_base::binary);
//	stream0.write((char*)_pBuf, IMAGE_SIZE);
//	stream0.close();
//	return;

	std::cout << "Image " << nReceived << std::endl;
	if (nReceived ++ == 0) {
		static std::vector<byte> copy(IMAGE_SIZE);
		::memcpy(&*copy.begin(), _pBuf, IMAGE_SIZE);
		PicturesStorage::instance().addPicture(&*copy.begin(), context);
		fillCalculatedData(_pBuf, context.frustum, false);
		return;
	}

	std::cout << "begin" << std::endl;
	Picture picture(_pBuf);

	KeyPointsImagesComparer imagesComparer(PicturesStorage::instance().getCurrentBasePicture(), picture);
	const size_t cCount = context.tracingPoints.size();
	const size_t cNeed = 2;

	if (cCount >= cNeed) {
		if (updateTracingPoints(context, picture, imagesComparer)) {
			context.cNotDetectedCount = 0;
			lastSuccess = context;
			fillCalculatedData(_pBuf, context.frustum, true);
//			PicturesStorage::StatisticLock lock;
//			for (size_t c = 0; c < 1; ++c) {
//				DynamicContext context2 = context;
//				updateTracingPoints(context2, picture, imagesComparer);
//			}

			return;
		}
	}

	if (context.cNotDetectedCount == 0 && !lastSuccess.tracingPoints.empty()) {
		context = lastSuccess;
		context.cNotDetectedCount = 1;
		fillCalculatedData(_pBuf, context.frustum, false);
		return;
	}

	lastSuccess = DynamicContext();

//	if (cCount >= cNeed) {
//		PicturesStorage::StatisticLock lock;
//		for (size_t c = 0; c < 9; ++c) {
//			DynamicContext context2 = context;
//			updateTracingPoints(context2, picture, imagesComparer);
//		}
//	}

	imagesComparer.clearStatistic();

	FloorDetector *pDetector = staticCompare(picture, imagesComparer);
	//pDetector->dump(pDetector->getPlanesKeys(), pDetector->getPairs()/*, &PicturesStorage::instance().getCurrentPoints(), &points2*/);
	fillCalculatedData(_pBuf, context.frustum, pDetector->detected());
	if (pDetector->detected()) {
		OrientedFrustum frustum;
		pDetector->calculateFrustum(frustum);
		picture.setOrientedFrustum(frustum);
		context.update(picture);
		context.tracingPoints.clear();

		std::list<FloorDetector::KeyPairData *> &keysList = pDetector->getPlanesKeys();
		std::vector<FloorDetector::KeyPairData *> keys;
		keys.reserve(keysList.size());
		keys.insert(keys.end(), keysList.begin(), keysList.end());

		for (auto iKey = keys.begin(); iKey != keys.end(); ++iKey)
			(*iKey)->nDistance2 = frustum.toScreen((*iKey)->pPair->pFirst->point).distance2((*iKey)->pPair->pSecond->screenPoint);

		std::sort(keys.begin(), keys.end(), [] (const FloorDetector::KeyPairData *const p1, const FloorDetector::KeyPairData *const p2)
			{
				static constexpr int nMiddleY = g_nHeight/2;
				const bool bFirstAtBottom = p1->pPair->pSecond->screenPoint.y > nMiddleY;
				const bool bSecondAtBottom = p2->pPair->pSecond->screenPoint.y > nMiddleY;
				return  bFirstAtBottom != bSecondAtBottom ? bFirstAtBottom : p1->nDistance2 < p2->nDistance2;
			}
		);

		for (auto iKey = keys.begin(); iKey != keys.end(); ++iKey) {
			const FloorDetector::KeyPairData *pData = *iKey;
//			for(TracingPointData &t : context.tracingPoints)
//				assert(t.pBasePoint->screenPoint != pData->pPair->pFirst->screenPoint);

			if (pData->pPair == nullptr)
				continue;

			context.tracingPoints.push_back(Holder<TracingPointData>(new TracingPointData()));
			TracingPointData &tracing = context.tracingPoints.back().get();
			tracing.pBasePoint = pData->pPair->pFirst;
			tracing.updatePrevPictureData(pData->pPair->pSecond);
			tracing.detected.invalidate();

			if (context.tracingPoints.size() == g_cTracingCount)
				break;
		}
	}

	nStaticCompares ++;
	std::cout << "Static compare processed: " << nStaticCompares << std::endl;
	delete pDetector;
	//context.lastPicture = pPicture;
}

#ifndef NOT_ANDROID

int main() {
	std::cout <<"start" << std::endl;
	for (size_t c = 0; c < 1; ++c)
	for (int nTest = 1; ; ) {
		std::stringstream strstream;
		strstream << "/sdcard/test" << nTest;
		std::ifstream stream(strstream.str().data(), std::ios_base::out | std::ios_base::binary);
		stream.seekg(0, std::ios_base::end);
		const int nSize = stream.tellg();

		if (nSize == -1)
			break;

		stream.seekg(0, std::ios_base::beg);
		unsigned char *pData = new unsigned char[nSize];
		stream.read((char *)pData, nSize);
		stream.close();

		testMatch(pData, nSize);

//		if (nTest == 1)
//			nTest = 1;
//		else
			++nTest;

		delete[] pData;
	}
	return 0;
}
#endif
