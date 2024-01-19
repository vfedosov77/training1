#ifndef __DUMPER__
#define __DUMPER__

//#ifndef NDEBUG

#include <list>
#include <fstream>
#include <unistd.h>

#include "BaseElements/Primitives2D.h"
#include "Pictures/KeyPoint.h"
#include "Pictures/Picture.h"

#ifdef NOT_ANDROID

using namespace std;

class ImageDump {
	const byte *m_pData;
	cv::Mat image;
	std::string fName;
	CvFont font;
	bool m_bDumpInWindow;

	static bool ms_bActive;
	static ImageDump *ms_pCurrent;
public:
	class Lock {
	public:
		Lock() {
			ms_bActive = true;
		}

		~Lock() {
			ms_bActive = false;
		}
	};

	Vector2D tagPoint;

	static bool isActive() {
		return ms_bActive;
	}

	static bool isNearTag(const Vector2D &_point, int _nMaxDist2 = 49) {
		return (_point - current().tagPoint).length2() <= _nMaxDist2;
	}

	static
	ImageDump &current() {
		assert(ms_pCurrent != nullptr);
		return *ms_pCurrent;
	}

	ImageDump(const byte *_pData, const std::string &_fName, bool _bDumpInWindow = false) : m_pData(_pData), fName(_fName), m_bDumpInWindow(_bDumpInWindow) {
		clear();
		cvInitFont(&font,CV_FONT_HERSHEY_PLAIN,0.9,1.1, 0,1,CV_AA);

		ms_pCurrent = this;
	}

	ImageDump(const Picture &_picture, const std::string &_fName = "") : m_pData(_picture.getBuf()), fName(_fName), m_bDumpInWindow(_fName.empty()) {
		clear();
		cvInitFont(&font,CV_FONT_HERSHEY_PLAIN,0.9,1.1, 0,1,CV_AA);

		ms_pCurrent = this;
	}

	ImageDump(const KeyPoint3D &_kp, const std::string &_fName = "") : m_pData(_kp.pPicture->getBuf()), fName(_fName), m_bDumpInWindow(_fName.empty()) {
		clear();
		cvInitFont(&font,CV_FONT_HERSHEY_PLAIN,0.9,1.1, 0,1,CV_AA);

		ms_pCurrent = this;

		dumpKeypoint(_kp, false);
	}

	void clear() {
		cv::Size sz(g_nWidth, g_nHeight);
		cv::Mat image0(sz, CV_8U, const_cast<byte *>(m_pData));
		cv::cvtColor(image0, image, CV_GRAY2BGR);
	}

	void dumpKeypoint(const KeyPoint &_kp) {
		std::vector<cv::KeyPoint> keypoints1;
		circle(_kp.point, _kp.nSize/2, cv::Scalar(255, 0, 0));
	}

	void dumpKeypoint(const Vector2D &_point, int _nNumber = -1) {
		circle(_point, 5);
		if (_nNumber != -1)
			number(_point + Vector2D(5, -2), _nNumber);
	}

	void dumpKeypoint(const KeyPoint3D &_kp, bool _bShowId = true);

	void dumpKeypoints(const std::list<KeyPoint3D> &_bestPoints) {
		std::vector<cv::KeyPoint> keypoints1;
		cv::Mat outImg1;
		for (const KeyPoint3D &kp : _bestPoints) {
			if (kp.gradientDirection.empty())
				continue;

			cv::Point2f cvPoint(kp.screenPoint.x, kp.screenPoint.y);
			keypoints1.push_back(cv::KeyPoint(cvPoint, kp.nSize));

			for (const Gradient &grad : kp.gradientDirection) {
				Vector2D dir = grad.directionF.toInt();
				dir.normalize();
				dir *= kp.nSize/2 + 2;
				dir /= g_nNormalizedVectorLength;
				cv::line(image, cvPoint, cv::Point2f(kp.screenPoint.x + dir.x, kp.screenPoint.y + dir.y),
					cv::Scalar(0,0,255)
				);
			}
		}

		cv::drawKeypoints(image, keypoints1, outImg1, cv::Scalar(255,0,0), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
		image = outImg1;
	}

	void line(Vector2D _from, Vector2D _to, int _nNum, cv::Scalar color = cv::Scalar(255)) {
		cv::line(image, _from.toCV(), _to.toCV(), color);
		number((_from + _to)/2, _nNum);
	}

	void line(Vector2D _from, Vector2D _to, string name = "", cv::Scalar color = cv::Scalar(255)) {
		cv::line(image, _from.toCV(), _to.toCV(), color);

		if (!name.empty())
			text((_from + _to)/2, name);
	}

	void circle(Vector2D _centre, int _nRadius, cv::Scalar color = cv::Scalar(255, 255, 255)) {
		cv::circle(image, _centre.toCV(), _nRadius, color);
	}

	void polygon(const std::vector<Vector2D> &_points) {
		if (_points.size() < 2)
			return;

		line(_points.back(), _points.front());
		for (size_t c = 1; c != _points.size(); ++c)
			line(_points[c - 1], _points[c]);
	}

	void number(Vector2D point, float _fVal) {
		std::stringstream str;
		str << _fVal;
		cv::putText(image, str.str().data(), point.toCV(), font.font_face, font.hscale, cv::Scalar(255, 255, 255));
	}

	void text(Vector2D point, std::string _str) {
		cv::putText(image, _str.data(), point.toCV(), font.font_face, font.hscale, cv::Scalar(255, 255, 255));
	}

	void dumpPlane(const Plane &_plane, const Frustum &_frustum) {
		PlaneWith2DCoordinates plane = PlaneWith2DCoordinates::createDefault(_plane);
		const Vector2DF dir(g_nOneMeter/5, 0);
		const Vector2DF ort(0, g_nOneMeter/5);
		for (int x = 10; x < g_nWidth; x += 40) {
			for (int y = 10; y < g_nHeight; y += 40) {
				Vector2D point(x, y);
				if (_frustum.canBeOnPlane(point, _plane)) {
					Vector2DF planeCoord = _frustum.toPlane(point, plane);
					Vector2DF dirEnd = _frustum.toScreenF(planeCoord + dir, plane);
					Vector2DF ortEnd = _frustum.toScreenF(planeCoord + ort, plane);
					line(point, dirEnd.toInt());
					line(point, ortEnd.toInt());
				}
			}
		}
	}

	void dumpInWindow() {
		static int nCallNum = 0;

		if (nCallNum++ == 0) {
			cv::startWindowThread();
			cv::namedWindow( "Display window", cv::WINDOW_NORMAL);
			cv::resizeWindow("Display window", 640, 480);
		}

		cv::imshow("Display window", image);
		//cv::waitKey();
	}

	void dumpInFile() {
		cv::imwrite(fName, image);
	}

	~ImageDump() {
		if (m_bDumpInWindow)
			dumpInWindow();
		else
			dumpInFile();

		if (ms_pCurrent == this)
			ms_pCurrent = nullptr;
	}

	cv::Mat &getImage() {
		return image;
	}
};

template<typename T>
inline
void saveDiagramm(const std::string &_name, T *_pArray, size_t _cSize, float _fIdx1 = -1.0f, float _fIdx2 = -1.0f) {
	T max = std::numeric_limits<T>::min(), min = std::numeric_limits<T>::max();
	for (T *pVal = _pArray, *pEnd = _pArray + _cSize; pVal != pEnd; ++pVal) {
		if (*pVal > max)
			max = *pVal;
		if (*pVal < min)
			min = *pVal;
	}


	cv::Size sz(640, 480);
	cv::Mat image(sz, CV_8U, cv::Scalar(0));
	int nCurIdx = 0;
	const int nPixInIdx = _cSize > 320 ? 1 : 640/_cSize;
	cv::Point2f prev(0, 480);
	for (T *pVal = _pArray, *pEnd = _pArray + _cSize; pVal != pEnd; ++pVal) {
		const int nCoordX = nCurIdx*nPixInIdx;
		const int nCoordY = 480 - (*pVal - min)*480/(max - min);
		cv::Point2f cur(nCoordX, nCoordY);
		cv::line(image, prev, cur, cv::Scalar(255));

		prev = cur;
		++nCurIdx;
	}

	if (_fIdx1 != -1.0f)
		cv::line(image, cv::Point2f(_fIdx1*nPixInIdx, 0), cv::Point2f(_fIdx1*nPixInIdx, 479), cv::Scalar(255));
	if (_fIdx2 != -1.0f)
		cv::line(image, cv::Point2f(_fIdx2*nPixInIdx, 0), cv::Point2f(_fIdx2*nPixInIdx, 479), cv::Scalar(255));


	cv::imwrite(_name, image);
}

void drawKeypoints(const byte *_pData, const std::string &_fName, const std::list<KeyPoint3D> &_bestPoints = std::list<KeyPoint3D>());
void drawKeypoints(const byte *_pData, const std::string &_fName, const std::vector<const KeyPoint3D *> &_bestPoints);

struct DumpColor {
	unsigned char btR, btG, btB;

	DumpColor(unsigned char _btR, unsigned char _btG, unsigned char _btB) : btR(_btR), btG(_btG), btB(_btB) { }

	static DumpColor getRed() {
		static DumpColor col(255, 0, 0);
		return col;
	}

	static DumpColor getGreen() {
		static DumpColor col(0, 255, 0);
		return col;
	}

	static DumpColor getBlue() {
		static DumpColor col(0, 0, 255);
		return col;
	}

	static DumpColor getBlack() {
		static DumpColor col(0, 0, 0);
		return col;
	}

	static DumpColor getWhite() {
		static DumpColor col(255, 255, 255);
		return col;
	}
};

class Dumper {
	static std::list<std::pair<Vector2D, DumpColor> > ms_points;
	static std::list<std::pair<Edge, DumpColor> > ms_edges;

public:
	static void addPoint(const Vector2D &p, DumpColor c) {
		ms_points.push_back(std::pair<Vector2D, DumpColor>(p, c));
	}

	static void addEdge(const Edge &e, DumpColor c) {
		ms_edges.push_back(std::pair<Edge, DumpColor>(e, c));
	}

	static void addCrostInPoint(const Vector2D &point, DumpColor c) {
		Edge e1(Vector2D(point.x - 3, point.y - 3), Vector2D(point.x + 3, point.y + 3));
		Edge e2(Vector2D(point.x + 3, point.y - 3), Vector2D(point.x - 3, point.y + 3));
		addEdge(e1, c);
		addEdge(e2, c);
	}

	static void flush(/*RefPointsDetector *pDetector = nullptr*/) {
		unlink("/home/vladimir/temp/line.bin");
		std::ofstream stream("/home/vladimir/temp/line.bin", std::ios_base::out | std::ios_base::binary);

		size_t cSize = ms_edges.size();
		stream.write((char*)&cSize, 4);

		for (auto &pair : ms_edges) {
			stream.write((char*)&pair.first.v1.x, 4);
			stream.write((char*)&pair.first.v1.y, 4);
			stream.write((char*)&pair.first.v2.x, 4);
			stream.write((char*)&pair.first.v2.y, 4);
			stream.write((char*)&pair.second.btR, 1);
			stream.write((char*)&pair.second.btG, 1);
			stream.write((char*)&pair.second.btB, 1);
		}

		cSize = ms_points.size();
		stream.write((char*)&cSize, 4);

		for (auto &pair : ms_points) {
			stream.write((char*)&pair.first.x, 4);
			stream.write((char*)&pair.first.y, 4);
			stream.write((char*)&pair.second.btR, 1);
			stream.write((char*)&pair.second.btG, 1);
			stream.write((char*)&pair.second.btB, 1);
		}

//		if (pDetector != nullptr) {
//			cSize = pDetector->getConvolutions().size();
//			stream.write((char*)&cSize, 4);

//			for (const LineConvolution &l : pDetector->getConvolutions()) {
//				stream.write((char*)&l.line.v1.x, 4);
//				stream.write((char*)&l.line.v1.y, 4);
//				stream.write((char*)&l.line.v2.x, 4);
//				stream.write((char*)&l.line.v2.y, 4);

//				cSize = l.pointsHor1.size();
//				stream.write((char*)&cSize, 4);

//				for (const RefPoint &p : l.pointsHor1) {
//					assert(p.fPartOfLine >= 0 && p.fPartOfLine <= 1);
//					int shortVal = 65536.0f*p.fPartOfLine;
//					stream.write((char*)&shortVal, 4);
//				}

//				cSize = l.pointsHor2.size();
//				stream.write((char*)&cSize, 4);

//				for (const RefPoint &p : l.pointsHor2) {
//					assert(p.fPartOfLine >= 0 && p.fPartOfLine <= 1);
//					int shortVal = 65536.0f*p.fPartOfLine;
//					stream.write((char*)&shortVal, 4);
//				}

//				cSize = l.pointsVert.size();
//				stream.write((char*)&cSize, 4);

//				for (const RefPoint &p : l.pointsVert) {
//					assert(p.fPartOfLine >= 0 && p.fPartOfLine <= 1);
//					int shortVal = 65536.0f*p.fPartOfLine;
//					stream.write((char*)&shortVal, 4);
//				}
//			}
//		} else {
			cSize = 0;
			stream.write((char*)&cSize, 4);
//		}

		stream.close();
		ms_points.clear();
		ms_edges.clear();
	}
};

#endif

#endif
