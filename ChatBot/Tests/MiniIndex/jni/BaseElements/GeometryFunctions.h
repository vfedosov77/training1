#ifndef _GEOMETRY_FUNCTIONS_H_
#define _GEOMETRY_FUNCTIONS_H_

#include <vector>

#include "Primitives2D.h"

//Test if the convex polygon contains point.
template<class Vector>
inline
bool isConvexPolygonContainsPoint(const std::vector<Vector> &_polygon, const Vector &_point) {
	if (_polygon.empty())
		return false;

	static const double dEpsilon = 1.0d/std::numeric_limits<int32_t>::max();
	int nSign = 0;

	for (size_t i = 0, j = _polygon.size() - 1; i < _polygon.size(); j = i++) {
		const double dVal = (_polygon[i] - _polygon[j]).crossProduct(_point - _polygon[j]);

		if (nSign == 0 && (dVal < -dEpsilon || dVal > dEpsilon))
			nSign = dVal > 0 ? 1 : -1;
		else if ((nSign == 1 && dVal < -dEpsilon) || (nSign == -1 && dVal > dEpsilon))
			return false;
	}

	return nSign != 0;
}

#ifdef NOT_ANDROID

template<class Vector, typename Iterator>
std::vector<Vector> makeConvexPolygon(Iterator _begin, Iterator _end) {
	std::vector<cv::Point> points;

	for (Iterator iPoints = _begin; iPoints != _end; ++iPoints)
		points.push_back(cvPoint(iPoints->x, iPoints->y));

	std::vector<cv::Point> hull;
	cv::Mat point_mat(points);
	cv::Mat hull_mat(hull);
	cv::convexHull(point_mat, hull_mat, false);

	std::vector<Vector> result;
	for (size_t c = 0; c != hull_mat.rows; ++c) {
		const cv::Point point = hull_mat.at<cv::Point>(c);
		result.emplace_back(point.x, point.y);
	}
	return result;
}

#endif //NDEBUG

//Works up to 90 degs.
inline
float getTanAngle(const Vector2DF &_v1, const Vector2DF &_v2) {
	Vector2DF rotatedV1 = rotateByVector(_v1, _v2);
	return std::abs(rotatedV1.y/rotatedV1.x);
}

inline
float getTanAngle(const Vector2D &_v1, const Vector2D &_v2) {
	Vector2D rotatedV1 = rotateByVector(_v1, _v2);
	return std::abs(float(rotatedV1.y)/rotatedV1.x);
}

#endif //_GEOMETRY_FUNCTIONS_H_
