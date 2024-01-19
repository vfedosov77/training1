#include "BaseElements/Correlation.h"
#include "Draw/Dumper.h"
#include "KeyPointsImagesComparer.h"

#ifdef NOT_ANDROID
#include <opencv2/opencv.hpp>
#include <opencv2/legacy/legacy.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "Draw/cvplot.h"
#endif //NDEBUG

namespace {
static const size_t g_cBufWith = 30;
static const size_t g_cBufSize = g_cBufWith*g_cBufWith;
}

class Solver {
public:
	Solver() {
		clear();
	}

	template <typename T, size_t _cSize>
	Solver(const SolverData<T, _cSize> &_data, const T *_pBegin1, const T *_pEnd1, const T *_pBegin2, float _fZoom) {
		assert(!_data.disabled());
		m_independent[0] = 0;
		m_independent[1] = 0;
		m_matrix[0][0] = _data.m_matrix[0][0];
		m_matrix[1][0] = _data.m_matrix[1][0];
		m_matrix[0][1] = _data.m_matrix[0][1];
		m_matrix[1][1] = _data.m_matrix[1][1];

		const T *pDistCoef = _data.m_pDistanceCoefficients;
		const T *pDerivatives = _data.m_pDerivatives;

		for (; _pBegin1 != _pEnd1; ++_pBegin1, ++_pBegin2, ++pDistCoef, ++pDerivatives) {
			const float fK0 = *_pBegin1 - (*_pBegin2*_fZoom);
			m_independent[0] -= (*pDerivatives)*fK0;
			m_independent[1] -= (*pDistCoef)*fK0;
		}
	}

	bool get(float &_fDx, float &_fDz) const {
		const float dDet = m_matrix[0][0]*m_matrix[1][1] - m_matrix[1][0]*m_matrix[0][1];

		if (dDet == 0)
			return false;

		//TODO: use m_matrix
		float inverse[2][2];
		inverse[0][0] = m_matrix[1][1]/dDet;
		inverse[1][0] = -m_matrix[1][0]/dDet;
		inverse[0][1] = -m_matrix[0][1]/dDet;
		inverse[1][1] = m_matrix[0][0]/dDet;

		_fDx = m_independent[0]*inverse[0][0] + m_independent[1]*inverse[1][0];
		_fDz = m_independent[0]*inverse[0][1] + m_independent[1]*inverse[1][1];
		return true;
	}

	void addPoint(float _fDistanceFromMid, float _fValue1, float _fValue2, float _fDirivative) {
		const float fDistCoef = _fDirivative*_fDistanceFromMid;
		const float fK0 = _fValue1 - _fValue2;
		const float fDiagonal = fDistCoef*_fDirivative;
		m_matrix[0][0] += _fDirivative*_fDirivative;
		m_matrix[1][1] += fDistCoef*fDistCoef;
		m_matrix[0][1] += fDiagonal;
		m_matrix[1][0] += fDiagonal;
		m_independent[0] += -_fDirivative*fK0;
		m_independent[1] += -fDistCoef*fK0;
	}

	void clear() {
		m_matrix[0][0] = 0;
		m_matrix[1][0] = 0;
		m_matrix[0][1] = 0;
		m_matrix[1][1] = 0;
		m_independent[0] = 0;
		m_independent[1] = 0;
	}

private:
	float m_matrix[2][2];
	float m_independent[2];
};

void _fillBufFromKP(const byte *_pBuf, const Vector2D &_screenPoint, const Vector2DF &_direction, const Vector2DF &_ort, float _fZoom,
	size_t _cWidth, float *_pResBuf, int _nSecondXOffset = 0
) {
	Vector2DF grad = _direction*_fZoom;
	Vector2DF ort = _ort*_fZoom;
	Vector2DF bottomLeft = Vector2DF(_screenPoint) - grad - ort;
	Vector2DF gradStep = grad;
	gradStep *= 2.0f/(_cWidth - 1);
	Vector2DF ortStep = ort;
	ortStep *= 2.0f/(_cWidth - 1);

	if (_nSecondXOffset != 0)
		bottomLeft -= gradStep*_nSecondXOffset;

	for (size_t j = 0; j != _cWidth; ++j) {
		Vector2DF curPos = bottomLeft + ortStep*j;
		for (size_t i = 0; i != _cWidth; ++i) {
			const float fVal = getInterpolatedValueAt(_pBuf, curPos.x, curPos.y);
			*_pResBuf = fVal;
			++_pResBuf;
			curPos += gradStep;
		}
	}
}

void _fillBufFromKPWithPerspective(byte *_pBuf, const Vector2D &_screenPoint, const Vector2DF &_direction,
	float _fZoom, size_t _cWidth, const Picture &_picture, float *_pResBuf
) {
	const Frustum & frustum = _picture.getFrustum();
	const Plane &earth = _picture.getEarthPlane();
	const Vector2DF screenPointF(_screenPoint);
	const Vector2DF ort2D = frustum.getOrtDirrection(_screenPoint, _direction, earth);
	const Vector3D centre = frustum.toPlane(screenPointF, earth);
	const Vector3D dirEnd = frustum.toPlane(screenPointF + _direction*_fZoom, earth);
	const Vector3D ortEnd = frustum.toPlane(screenPointF + ort2D*_fZoom, earth);
	const Vector3D dir = dirEnd - centre;
	const Vector3D ort = ortEnd - centre;
	const Vector3D topLeft = centre - ort - dir;
	Vector3D gradStep = dir;
	gradStep *= 2.0f/(_cWidth - 1);
	Vector3D ortStep = ort;
	ortStep *= 2.0f/(_cWidth - 1);

	for (size_t j = 0; j != _cWidth; ++j) {
		Vector3D curPos = topLeft + ortStep*j;
		for (size_t i = 0; i != _cWidth; ++i) {
			const Vector2DF pos = frustum.toScreenF(curPos);
			const float fVal = getInterpolatedValueAt(_pBuf, pos.x, pos.y);
			*_pResBuf = fVal;
			++_pResBuf;
			curPos += gradStep;
		}
	}
}

bool KeyPointsImagesComparer::compareImages(const KeyPair &_pair1, const KeyPair &_pair2) const {
	Vector2DF dir1(_pair2.pFirst->screenPoint - _pair1.pFirst->screenPoint);
	const float fDir1Length = dir1.length();
	if (fDir1Length == 0)
		return false;
	dir1.normalize();

	const float fDir11Length = _pair1.pFirst->nSize/2 + 4;
	const float fDir12Length = _pair2.pFirst->nSize/2 + 4;
	const Vector2DF dir11 = dir1*fDir11Length;
	const Vector2DF dir12 = dir1*fDir12Length;
	const Vector2DF ort11 = m_firstPicture.getFrustum().getOrtDirrection(_pair1.pFirst->screenPoint, dir11, m_firstPicture.getEarthPlane());
	const Vector2DF ort12 = m_firstPicture.getFrustum().getOrtDirrection(_pair2.pFirst->screenPoint, dir12, m_firstPicture.getEarthPlane());
	Vector2DF dir2(_pair2.pSecond->screenPoint - _pair1.pSecond->screenPoint);
	const float fDir2Length = dir2.length();
	if (fDir2Length == 0)
		return false;
	dir2.normalize();
	const float fDir21Length = fDir11Length*fDir2Length/fDir1Length;
	const float fDir22Length = fDir12Length*fDir2Length/fDir1Length;
	const Vector2DF dir21 = dir2*fDir21Length;
	const Vector2DF dir22 = dir2*fDir22Length;
	const Vector2DF ort21 = m_secondPicture.getFrustum().getOrtDirrection(_pair1.pSecond->screenPoint, dir21, m_secondPicture.getEarthPlane());
	const Vector2DF ort22 = m_secondPicture.getFrustum().getOrtDirrection(_pair2.pSecond->screenPoint, dir22, m_secondPicture.getEarthPlane());

	bool bSmall1 = std::min(_pair1.pFirst->nSize, _pair1.pSecond->nSize) < 16;
	const float fAreaZoom1 = _getCompareZoom(*_pair1.pFirst, *_pair1.pSecond, fDir11Length, ort11.length(), fDir21Length, ort21.length(), bSmall1);
	bool bSmall2 = std::min(_pair2.pFirst->nSize, _pair2.pSecond->nSize) < 16;
	const float fAreaZoom2 = _getCompareZoom(*_pair2.pFirst, *_pair2.pSecond, fDir12Length, ort12.length(), fDir22Length, ort22.length(), bSmall2);

	float buf1[g_cBufSize], buf2[g_cBufSize];
	_fillBufFromKP(m_pFirstBuf, _pair1.pFirst->screenPoint, dir11, ort11, fAreaZoom1, g_cBufWith, buf1);
	Vector2D point = _pair1.pSecond->screenPoint;

	float fMaxCor = -1;
	for (int x = point.x - 2; x <= point.x + 2; ++x)
		for (int y = point.y - 2; y <= point.y + 2; ++y) {
			_fillBufFromKP(m_pSecondBuf, Vector2D(x, y), dir21, ort21, fAreaZoom1, g_cBufWith, buf2);
			float fRes = correlation(buf1, buf2, g_cBufSize);
			if (fRes > fMaxCor)
				fMaxCor = fRes;
		}

//	_fillBufFromKP(m_pSecondBuf, _pair1.pSecond->screenPoint, dir21, ort21, fAreaZoom1, g_cBufWith, buf2);
//	float fRes = correlation(buf1, buf2, g_cBufSize);

//	_fillBufFromKPWithPerspective(m_pFirstBuf, _pair1.pFirst->screenPoint, dir11, fAreaZoom1, g_cBufWith, m_firstHessian, buf1);
//	_fillBufFromKPWithPerspective(m_pSecondBuf, _pair1.pSecond->screenPoint, dir21, fAreaZoom1, g_cBufWith, m_secondHessian, buf2);
//	float fRes2 = correlation(buf1, buf2, g_cBufSize);

//	cv::Size sz(30, 30);
//	cv::Mat image1(sz, CV_32F, buf1);
//	cv::imwrite("/home/vfedosov/temp/KeyPoints/_kp1.jpg", image1);
//	cv::Mat image2(sz, CV_32F, buf2);
//	cv::imwrite("/home/vfedosov/temp/KeyPoints/_kp2.jpg", image2);
//	_fillBufFromKP(m_pFirstBuf, _pair2.pFirst->screenPoint, dir12, ort12, fAreaZoom2, g_cBufWith, buf1);
//	_fillBufFromKP(m_pSecondBuf, _pair2.pSecond->screenPoint, dir22, ort22, fAreaZoom2, g_cBufWith, buf2);
//	cv::Mat image3(sz, CV_32F, buf1);
//	cv::imwrite("/home/vfedosov/temp/KeyPoints/_kp3.jpg", image3);
//	cv::Mat image4(sz, CV_32F, buf2);
//	cv::imwrite("/home/vfedosov/temp/KeyPoints/_kp4.jpg", image4);
//	{
//		ImageDump dump1(m_firstHessian.getBuf(), "/home/vfedosov/temp/KeyPoints/out1.jpg");
//		ImageDump dump2(m_secondHessian.getBuf(), "/home/vfedosov/temp/KeyPoints/out2.jpg");
//		dump1.dumpKeypoint(*_pair1.pFirst);
//		dump2.dumpKeypoint(*_pair1.pSecond);
//		dump1.dumpKeypoint(*_pair2.pFirst);
//		dump2.dumpKeypoint(*_pair2.pSecond);
//	}

	if (fMaxCor < (bSmall1 ? 0.9 : 0.85))
		return false;

	_fillBufFromKP(m_pFirstBuf, _pair2.pFirst->screenPoint, dir12, ort12, fAreaZoom2, g_cBufWith, buf1);
	fMaxCor = -1;
	point = _pair2.pSecond->screenPoint;
	for (int x = point.x - 2; x <= point.x + 2; ++x)
		for (int y = point.y - 2; y <= point.y + 2; ++y) {
			_fillBufFromKP(m_pSecondBuf, Vector2D(x, y), dir22, ort22, fAreaZoom2, g_cBufWith, buf2);
			float fRes = correlation(buf1, buf2, g_cBufSize);
			if (fRes > fMaxCor)
				fMaxCor = fRes;
		}

	if (fMaxCor < (bSmall2 ? 0.9 : 0.85))
		return false;

	return true;
}

//void createImagePartMat(byte *_pBuf, const Rect2D &_rect, cv::Mat &_mat) {
//	cv::Size sz(_rect.width(), _rect.height());
//	byte *pNewBuf = new byte[_rect.width()*_rect.height()];

//	for (int i = _rect.nY0; i < _rect.nY1; ++i)
//		memcpy(pNewBuf + (i - _rect.nY0)*_rect.width(), _pBuf + i*g_nWidth + _rect.nX0, _rect.width());

//	cv::Mat image(sz, CV_8U, pNewBuf);
//	cv::cvtColor(image, _mat, CV_GRAY2BGR);
//	delete pNewBuf;
//}

//int createKeyPointMat(byte *_pBuf, const KeyPoint3D &_point, cv::Mat &_mat) {
//	const int nSize = _point.nSize > 35 ? _point.nSize : 35;
//	const int nDelta = std::min(std::min(std::min(std::min(nSize, _point.screenPoint.x),  _point.screenPoint.y), g_nHeight - 1 - _point.screenPoint.y), g_nWidth - 1 - _point.screenPoint.x);
//	Rect2D rect(_point.screenPoint.x - nDelta, _point.screenPoint.y - nDelta, _point.screenPoint.x + nDelta, _point.screenPoint.y + nDelta);
//	assert(rect.nX0 >= 0 && rect.nX0 < g_nWidth && rect.nY0 >= 0 && rect.nY1 < g_nHeight);
//	createImagePartMat(_pBuf, rect, _mat);

//	return nDelta;
//}

//void saveImagePart(byte *_pBuf, const Rect2D &_rect, const std::string &_name) {
//	cv::Mat image;
//	createImagePartMat(_pBuf, _rect, image);
//	cv::imwrite(_name, image);
//}

static
float _calcCorrelationWithZoom(const int *pArray1, size_t _cArray1Size, const int *pArray2, size_t _cArray2Size,
	float _fHalfFirst, float _fHalfSecond, bool _bByFirst, float _fZoom, Solver *_pSolver = nullptr, float _fCoef = 1
) {
	static float buf[Gradient::ms_cArraySize];
	float fCorX;

	if (_bByFirst) {
		const float fStartIndex = _fHalfSecond - _fHalfFirst*_fZoom;
		const float fStep = _fZoom;
		InterpolatedArrayIterator<int> iteratorX(pArray2, _cArray2Size, fStartIndex, fStep);

		float *pValX = buf;
		float *pEnd = buf + _cArray1Size;
		float fPrevX = *iteratorX;
		for (; pValX !=  pEnd; ++pValX) {
			const float fCurX = *iteratorX;
			*pValX = fCurX;
			if (_pSolver != nullptr) {
				_pSolver->addPoint(iteratorX.getCurrentIdx() - _fHalfSecond, fCurX, pArray1[pValX - buf]*_fCoef, (iteratorX.next() - fPrevX)/2);
				fPrevX = fCurX;
			}
			++iteratorX;
		}

		fCorX = correlation(buf, pArray1, _cArray1Size);
	} else {
		const float fStartIndex = _fHalfFirst - _fHalfSecond/_fZoom;
		const float fStep = 1/_fZoom;
		InterpolatedArrayIterator<int> iteratorX(pArray1, _cArray1Size, fStartIndex, fStep);

		float *pValX = buf;
		float *pEnd = buf + _cArray2Size;
		float fPrevX = 0;
		for (; pValX !=  pEnd; ++pValX) {
			const float fCurX = *iteratorX;
			*pValX = fCurX;
			if (_pSolver != nullptr) {
				_pSolver->addPoint(iteratorX.getCurrentIdx() - _fHalfFirst, fCurX, pArray2[pValX - buf]/_fCoef, (iteratorX.next() - fPrevX)/2);
				fPrevX = fCurX;
			}
			++iteratorX;
		}

		fCorX = correlation(buf, pArray2, _cArray2Size);
	}

	return fCorX;
}

static
float _calcCorrelationWithZoom(const int *pArray1, size_t _cArray1Size, const int *pArray2, size_t _cArray2Size,
	float _fZoom, Solver *_pSolver = nullptr, float _fCoef = 1
) {
	const float fHalfFirst = float(_cArray1Size - 1)/2;
	const float fHalfSecond = float(_cArray2Size - 1)/2;
	const bool bByFirst = fHalfFirst*_fZoom < fHalfSecond;
	const float fCorX = _calcCorrelationWithZoom(pArray1, _cArray1Size, pArray2, _cArray2Size,
		fHalfFirst, fHalfSecond, bByFirst, _fZoom, _pSolver, _fCoef
	);
	return fCorX;
}

static int nCompareNum = 0;

bool KeyPointsImagesComparer::compareAlongGrad(const Gradient &_firstGrad, const Gradient &_secondGrad, int &_nSecondXOffset, bool *_pbNear) {
	if (std::abs(_firstGrad.fSinCorrelation - _secondGrad.fSinCorrelation) > 0.1)
		return false;

	_nSecondXOffset = 0;
	const float fMinCorX = 0.7;
	const float fAppropriateCorX = 0.9;
	const float fNearCorX = 0.8;
	const float fAppropriateCorY = 0.6;
	const float fAppropriateHessianCor = 0.9;
	const float fMinDx = 0.4;
	const float fMinDz = 0.07;

	const bool bLongCompare = isLongCompare(_firstGrad, _secondGrad);
	const auto &gradArrayX1 = bLongCompare ? _firstGrad.gradArrayXLong : _firstGrad.gradArrayX;
	const auto &gradArrayX2 = bLongCompare ? _secondGrad.gradArrayXLong : _secondGrad.gradArrayX;

//	if (ImageDump::isActive()) {
//		std::stringstream stream1;
//		stream1 << "/home/vfedosov/temp/KeyPoints/point_graph1.jpg";
//		std::stringstream stream2;
//		stream2 << "/home/vfedosov/temp/KeyPoints/pointd_graph2.jpg";
//		saveDiagramm(stream1.str().data(), const_cast<int *>(gradArrayX1.begin()), Gradient::ms_cArraySize, 10, 20);
//		saveDiagramm(stream2.str().data(), const_cast<int *>(gradArrayX2.begin()), Gradient::ms_cArraySize,10 ,20);
//	}

	float fCor = gradArrayX1.calculate(gradArrayX2);
	if (fCor < fMinCorX)
		return false;

	const auto &gradArrayY1 = bLongCompare ? _firstGrad.gradArrayYLong : _firstGrad.gradArrayY;
	const auto &gradArrayY2 = bLongCompare ? _secondGrad.gradArrayYLong : _secondGrad.gradArrayY;
	const auto &hessian1 = bLongCompare ? _firstGrad.hessianLong : _firstGrad.hessian;
	const auto &hessian2 = bLongCompare ? _secondGrad.hessianLong : _secondGrad.hessian;

	if (fCor > fAppropriateCorX) {
		const float fCorY = gradArrayY1.calculate(gradArrayY2);
		if (fCorY >= fAppropriateCorY)
			return true;

		const float fHessianCor = hessian1.calculate(hessian2);
		if (fHessianCor >= fAppropriateHessianCor)
			return true;

		if (fHessianCor < 0.7 && fCorY < 0.5) {
			if (_pbNear && (fHessianCor > 0.5 || fCorY > 0.35))
				*_pbNear = true;

			return false;
		}
	}

	if (_pbNear && fCor > fNearCorX) {
		const float fCorY = gradArrayY1.calculate(gradArrayY2);
		if (fCorY >= fAppropriateCorY)
			*_pbNear = true;

		const float fHessianCor = hessian1.calculate(hessian2);
		if (fHessianCor >= fAppropriateHessianCor)
			*_pbNear = true;
	}

	const float fZoom = gradArrayX1.getStandartDeviation()/gradArrayX2.getStandartDeviation();
	Solver solver(gradArrayX1.getSolverData(), gradArrayX1.begin(), gradArrayX1.end(), gradArrayX2.begin(), fZoom);
	float fDx, fDz;
	if (!solver.get(fDx, fDz))
		return false;

	if (fDx > fMinDx) {
		_nSecondXOffset = 1;
		const float fCorX = gradArrayX1.calculateWithShift1(gradArrayX2);
		if (fCorX < fAppropriateCorX)
			return false;

		const float fCorY = gradArrayY1.calculateWithShift1(gradArrayY2);
		if (fCorY >= fAppropriateCorY)
			return true;

		const float fHessianCor = hessian1.calculateWithShift1(hessian2);

//		if (ImageDump::isActive()) {
//			std::stringstream stream1;
//			stream1 << "/home/vfedosov/temp/KeyPoints/point_hessian_graph1.jpg";
//			std::stringstream stream2;
//			stream2 << "/home/vfedosov/temp/KeyPoints/point_hessian_graph2.jpg";
//			saveDiagramm(stream1.str().data(), const_cast<int *>(hessian1.begin()), Gradient::ms_cArraySize, 10, 20);
//			saveDiagramm(stream2.str().data(), const_cast<int *>(hessian2.begin()), Gradient::ms_cArraySize,10 ,20);
//		}

//		if (ImageDump::isActive()) {
//			std::stringstream stream1;
//			stream1 << "/home/vfedosov/temp/KeyPoints/point_y_graph1.jpg";
//			std::stringstream stream2;
//			stream2 << "/home/vfedosov/temp/KeyPoints/point_y_graph2.jpg";
//			saveDiagramm(stream1.str().data(), const_cast<int *>(gradArrayY1.begin()), Gradient::ms_cArraySize, 10, 20);
//			saveDiagramm(stream2.str().data(), const_cast<int *>(gradArrayY2.begin()), Gradient::ms_cArraySize,10 ,20);
//		}

		return fHessianCor >= fAppropriateHessianCor;
	} else if (fDx < -fMinDx) {
		_nSecondXOffset = -1;
		const float fCorX = gradArrayX2.calculateWithShift1(gradArrayX1);
		if (fCorX < fAppropriateCorX)
			return false;

		const float fCorY = gradArrayY2.calculateWithShift1(gradArrayY1);
		if (fCorY >= fAppropriateCorY)
			return true;

		const float fHessianCor = hessian2.calculateWithShift1(hessian1);
		return fHessianCor >= fAppropriateHessianCor;
	} else if (fDz > fMinDz) {
		const float fCorX = gradArrayX1.calculateWithZoom1(gradArrayX2);
		if (fCorX < fAppropriateCorX)
			return false;

		const float fCorY = gradArrayY1.calculateWithZoom1(gradArrayY2);
		if (fCorY >= fAppropriateCorY)
			return true;

		const float fHessianCor = hessian1.calculateWithZoom1(hessian2);
		return fHessianCor >= fAppropriateHessianCor;
	} else if (fDz < -fMinDz) {
		const float fCorX = gradArrayX2.calculateWithZoom1(gradArrayX1);
		if (fCorX < fAppropriateCorX)
			return false;

		const float fCorY = (gradArrayY2).calculateWithZoom1(gradArrayY1);
		if (fCorY >= fAppropriateCorY)
			return true;

		const float fHessianCor = hessian2.calculateWithZoom1(hessian1);
		return fHessianCor >= fAppropriateHessianCor;
	}

	return false;
}


bool KeyPointsImagesComparer::compareAlongOrt(const Gradient &_firstGrad, const Gradient &_secondGrad, bool *_pbNear) {
	const float fAppropriateCorX = 0.8;
	const float fNearCorX = 0.7;
	const float fMinDx = 0.4;
	const float fMinDz = 0.07;

	const bool bLongCompare = isLongCompare(_firstGrad, _secondGrad);
	const auto &arrayX1 = bLongCompare ? _firstGrad.ortArrayXLong : _firstGrad.ortArrayX;
	const auto &arrayX2 = bLongCompare ? _secondGrad.ortArrayXLong : _secondGrad.ortArrayX;

//	std::stringstream stream1;
//	stream1 << "/home/vfedosov/temp/KeyPoints/point" << nCompareNum << "_ort1.jpg";
//	std::stringstream stream2;
//	stream2 << "/home/vfedosov/temp/KeyPoints/point" << nCompareNum << "_ort2.jpg";
//	saveDiagramm(stream1.str().data(), const_cast<int *>(arrayX1.begin()), Gradient::ms_cArraySize, 10, 20);
//	saveDiagramm(stream2.str().data(), const_cast<int *>(arrayX2.begin()), Gradient::ms_cArraySize,10 ,20);

	float fCor = arrayX1.calculate(arrayX2);
	if (fCor < 0.6)
		return false;

	if (_pbNear != nullptr && fCor >= fNearCorX)
		*_pbNear = true;

	if (fCor < fAppropriateCorX) {
		const float fZoom = arrayX1.getStandartDeviation()/arrayX2.getStandartDeviation();
		Solver solver(arrayX1.getSolverData(), arrayX1.begin(), arrayX1.end(), arrayX2.begin(), fZoom);
		float fDx, fDz;
		if (!solver.get(fDx, fDz))
			return false;

		if (fDx > fMinDx)
			fCor = arrayX1.calculateWithShift1(arrayX2);
		else if (fDx < -fMinDx)
			fCor = arrayX2.calculateWithShift1(arrayX1);
		else if (fDz > fMinDz)
			fCor = arrayX1.calculateWithZoom1(arrayX2);
		else if (fDz < -fMinDz)
			fCor = arrayX2.calculateWithZoom1(arrayX1);
	}

	return fCor >= fAppropriateCorX;
}
int nCounter = 0;
int nAllCounter = 0;

float KeyPointsImagesComparer::_getCompareZoom(const KeyPoint3D &_first, const KeyPoint3D &_second, float _fGradLen1, float _fOrtLen1,
	float _fGradLen2, float _fOrtLen2, bool &_bSmall) const
{
	Rect2D allowedRect1 = Application::getScreenRect();
	Rect2D allowedRect2 = allowedRect1;
	static const float fSqrt2 = ::sqrt(2.0f);
	const float fFirstInflate = fast_floor(std::max(_fGradLen1, _fOrtLen1)*fSqrt2 + 1);
	const float fSecondInflate = fast_floor(std::max(_fGradLen2, _fOrtLen2)*fSqrt2 + 1);
	float fAreaSizeZoom = 1;
	allowedRect1.inflate(fFirstInflate);
	allowedRect2.inflate(fSecondInflate);
	if (!allowedRect1.contains(_first.screenPoint) || !allowedRect2.contains(_second.screenPoint)) {
		fAreaSizeZoom = 0.7f;
	} else if (_fGradLen1 <= 9 || _fGradLen2 <= 9) {
		allowedRect1.inflate(fFirstInflate*2);
		allowedRect2.inflate(fSecondInflate*2);
		if (allowedRect1.contains(_first.screenPoint) && allowedRect2.contains(_second.screenPoint)) {
			fAreaSizeZoom *= 3;
			_bSmall = false;
		} else
			fAreaSizeZoom *= 2;
	} else if (_fGradLen1 <= 25) {
		allowedRect1.inflate(fFirstInflate);
		allowedRect2.inflate(fSecondInflate);
		if (allowedRect1.contains(_first.screenPoint) && allowedRect2.contains(_second.screenPoint))
			fAreaSizeZoom *= 2;
	}

	return fAreaSizeZoom;
}

extern int nReceived;

bool KeyPointsImagesComparer::compareBuffs(const Gradient &_firstGrad, const Gradient &_secondGrad, const KeyPoint3D &_first, const KeyPoint3D &_second,
	int _nSecondXOffset, KeyPair *_pPair
) {
	if (!_firstGrad.pixelsBuf.initialized())
		_firstGrad.fillPixelsBuffer(m_firstPicture.getBuf(), _first.screenPoint);
	if (!_secondGrad.pixelsBuf.initialized())
		_secondGrad.fillPixelsBuffer(m_secondPicture.getBuf(), _second.screenPoint);

	const int nMinSize = std::min(_first.nSize, _second.nSize);
	bool bSmall = nMinSize < 16;
	const float fBufCor = _firstGrad.pixelsBuf.calculate(_secondGrad.pixelsBuf);
	if (fBufCor < 0.65)
		return false;
	else if (fBufCor >= (bSmall ? 0.95 : 0.9)) {
		if (_pPair != nullptr) {
			_pPair->pFirst = &_first;
			_pPair->pSecond = &_second;
			_pPair->pGrad1 = &_firstGrad;
			_pPair->pGrad2 = &_secondGrad;
			_pPair->fCorrelation = fBufCor;
			_pPair->fZoom = 1;
			_pPair->fAngle = 0;
			_pPair->bApproved = true;
		}
		++nCompareNum;
		++m_pointsIdsToMatchCount[_first.nId];
		++m_pointsIdsToMatchCount[_second.nId];
		++nCounter;
		return true;
	}

	float fAreaSizeZoom = _getCompareZoom(_first, _second, _firstGrad.fGradLen, _firstGrad.fOrtLen, _secondGrad.fGradLen, _secondGrad.fOrtLen, bSmall);

	float buf1[g_cBufSize], buf2[g_cBufSize];
	_fillBufFromKP(m_pFirstBuf, _first.screenPoint, _firstGrad.directionF, _firstGrad.ortF, fAreaSizeZoom, g_cBufWith, buf1);
	_fillBufFromKP(m_pSecondBuf, _second.screenPoint, _secondGrad.directionF, _secondGrad.ortF, fAreaSizeZoom, g_cBufWith, buf2, _nSecondXOffset);

	const float fRes = correlation(buf1, buf2, g_cBufSize);
	if (fRes > (bSmall ? 0.75 : 0.65)) {
		++nCounter;
		if (_pPair != nullptr) {
			_pPair->pFirst = &_first;
			_pPair->pSecond = &_second;
			_pPair->pGrad1 = &_firstGrad;
			_pPair->pGrad2 = &_secondGrad;
			_pPair->fCorrelation = fRes;
			_pPair->fZoom = 1;
			_pPair->fAngle = 0;
			_pPair->bApproved = fRes >= (bSmall ? 0.95 : 0.9);
		}

		++nCompareNum;
		++m_pointsIdsToMatchCount[_first.nId];
		++m_pointsIdsToMatchCount[_second.nId];
		return true;
	}

	return false;
}

bool KeyPointsImagesComparer::compare(const KeyPoint3D &_first, const KeyPoint3D &_second, KeyPair *pPair, bool *_pbNear) {
	if (_first.gradientDirection.empty() || _second.gradientDirection.empty())
		return false;

	if (_first.bHessianSign != _second.bHessianSign)
		return false;

//	if (nReceived == 15 && _first.nId == 69) {
//		++nCompareNum;
//	}

	size_t cCompareNum1 = size_t(-1);

//	cv::Mat image1, image2;
//	const int nDelta1 = createKeyPointMat(m_pFirstBuf, _first, image1);
//	const int nDelta2 = createKeyPointMat(m_pSecondBuf, _second, image2);

	for (auto iGrad1 = _first.gradientDirection.begin(); iGrad1 != _first.gradientDirection.end(); ++iGrad1) {
		const Gradient &firstGrad = *iGrad1;
		size_t cCompareNum2 = size_t(-1);
		cCompareNum1++;
		for (auto iGrad2 = _second.gradientDirection.begin(); iGrad2 != _second.gradientDirection.end(); ++iGrad2) {
			const Gradient &secondGrad = *iGrad2;
			cCompareNum2++;

			++nAllCounter;

//			if (cCompareNum1 != 0 || cCompareNum2 != 1)
//				continue;

//			cv::Point centre1(nDelta1, nDelta1);
//			cv::Point centre2(nDelta2, nDelta2);
			int nSecondXOffset;
			if (compareAlongGrad(firstGrad, secondGrad, nSecondXOffset, _pbNear)) {
				if (_pbNear)
					*_pbNear = true;

//				cv::circle(image1, centre1, round(firstGrad.fGradLen), cv::Scalar(255, 0, 0));
//				cv::circle(image2, centre2, round(secondGrad.fGradLen), cv::Scalar(255, 0, 0));

//				cv::line(image1, centre1, centre1 + (firstGrad.directionF*2).toCV(), cv::Scalar(0,0,255));
//				cv::line(image2, centre2, centre2 + (secondGrad.directionF*2).toCV(), cv::Scalar(0,0,255));

//				cv::line(image1, centre1, centre1 + (firstGrad.ortF*2).toCV(), cv::Scalar(0, 255, 255));
//				cv::line(image2, centre2, centre2 + (secondGrad.ortF*2).toCV(), cv::Scalar(0, 255, 255));

				if (compareAlongOrt(firstGrad, secondGrad))
					if (compareBuffs(firstGrad, secondGrad, _first, _second, nSecondXOffset, pPair))
						return true;
			}
		}
	}

	//if (bMatch) {
//		if (!kp1Mat.empty()) {
//			cv::imwrite("/home/vfedosov/temp/KeyPoints/_kp1.jpg", kp1Mat);
//			cv::imwrite("/home/vfedosov/temp/KeyPoints/_kp2.jpg", kp2Mat);
//		}
//		std::stringstream stream1;
//		stream1 << "/home/vfedosov/temp/KeyPoints/point" << nCompareNum << "_1.jpg";
//		cv::imwrite(stream1.str(), image1);
//		std::stringstream stream2;
//		stream2 << "/home/vfedosov/temp/KeyPoints/point" << nCompareNum << "_2.jpg";
//		cv::imwrite(stream2.str(), image2);
	//}

	++nCompareNum;
	return false;
}


