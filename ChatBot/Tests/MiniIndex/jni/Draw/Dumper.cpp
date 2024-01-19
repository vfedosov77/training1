#include "Dumper.h"

#ifdef NOT_ANDROID

std::list<std::pair<Vector2D, DumpColor> > Dumper::ms_points;
std::list<std::pair<Edge, DumpColor> > Dumper::ms_edges;
ImageDump *ImageDump::ms_pCurrent = nullptr;
bool ImageDump::ms_bActive = false;

void ImageDump::dumpKeypoint(const KeyPoint3D &_kp, bool _bShowId) {
	std::vector<cv::KeyPoint> keypoints1;
	cv::Mat outImg1;


	cv::Point2f cvPoint(_kp.screenPoint.x, _kp.screenPoint.y);
	keypoints1.push_back(cv::KeyPoint(cvPoint, _kp.nSize));

	for (const Gradient &grad : _kp.gradientDirection) {
		Vector2DF dir = grad.directionF*6;
		Vector2DF ort = grad.ortF*6;
		cv::line(image, cvPoint, cv::Point2f(_kp.screenPoint.x + dir.x, _kp.screenPoint.y + dir.y),
			cv::Scalar(0,0,255)
		);
		cv::line(image, cvPoint, cv::Point2f(_kp.screenPoint.x + ort.x, _kp.screenPoint.y + ort.y),
			cv::Scalar(0,255,255)
		);
	}

	if (_bShowId)
		number(_kp.screenPoint, _kp.nId);

	cv::drawKeypoints(image, keypoints1, outImg1, cv::Scalar(255,0,0), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
	image = outImg1;
}

void drawKeypoints(const byte *_pData, const std::string &_fName, const std::list<KeyPoint3D> &_bestPoints)
{
	cv::Size sz(640, 480);
	cv::Mat image0(sz, CV_8U, const_cast<byte *>(_pData));
	cv::Mat image1 = image0.clone();
	cv::Mat outImg1;

	std::vector<cv::KeyPoint> keypoints1;
	for (const KeyPoint3D &kp : _bestPoints) {
//		if (kp.gradientDirection.empty())
//			continue;

		cv::Point2f cvPoint(kp.screenPoint.x, kp.screenPoint.y);
		keypoints1.push_back(cv::KeyPoint(cvPoint, kp.nSize));

		for (const Gradient &grad : kp.gradientDirection) {
			Vector2DF dir = grad.directionF;
			dir *= 2;

			cv::line(image1, cvPoint, cv::Point2f(kp.screenPoint.x + dir.x, kp.screenPoint.y + dir.y),
				cv::Scalar(0,0,255)
			);
		}
	}

	cv::drawKeypoints(image1, keypoints1, outImg1, cv::Scalar(255,0,0), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
	cv::imwrite(_fName, outImg1);
}

void drawKeypoints(const byte *_pData, const std::string &_fName, const std::vector<const KeyPoint3D *> &_bestPoints)
{
	cv::Size sz(640, 480);
	cv::Mat image0(sz, CV_8U, const_cast<byte *>(_pData));
	cv::Mat image1 = image0.clone();
	cv::Mat outImg1;

	std::vector<cv::KeyPoint> keypoints1;
	for (const KeyPoint3D *pPoint : _bestPoints) {
		const KeyPoint3D &kp = *pPoint;
//		if (kp.gradientDirection.empty())
//			continue;

		cv::Point2f cvPoint(kp.screenPoint.x, kp.screenPoint.y);
		keypoints1.push_back(cv::KeyPoint(cvPoint, kp.nSize));

		for (const Gradient &grad : kp.gradientDirection) {
			Vector2D dir = grad.directionF.toInt();
			dir.normalize();
			dir *= kp.nSize/2 + 2;
			dir /= g_nNormalizedVectorLength;
			cv::line(image1, cvPoint, cv::Point2f(kp.screenPoint.x + dir.x, kp.screenPoint.y + dir.y),
				cv::Scalar(0,0,255)
			);
		}
	}

	cv::drawKeypoints(image1, keypoints1, outImg1, cv::Scalar(255,0,0), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
	cv::imwrite(_fName, outImg1);
}

#endif
