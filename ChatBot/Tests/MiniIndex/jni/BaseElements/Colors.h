#ifndef _COLORS_H_
#define _COLORS_H_

#include "Primitives2D.h"
#include "Application.h"

enum Colors {
	clRed,
	clGreen,
	clBlue
};

struct Color {
	unsigned char btBtness;
	unsigned char btCol1;
	unsigned char btCol2;

	Color() : btBtness(0), btCol1(0), btCol2(0) { }
	Color(unsigned char _btBtness, unsigned char _btCol1, unsigned char _btCol2) : btBtness(_btBtness), btCol1(_btCol1), btCol2(_btCol2) {

	}

	int operator -(const Color &_other) const {
		return std::abs(int(btBtness) - _other.btBtness) + std::abs(int(btCol1) - _other.btCol1) + std::abs(int(btCol2) - _other.btCol2);
	}

	bool isSet() const {
		return btBtness != 0 || btCol1 != 0 || btCol2 != 0;
	}

	void reset() {
		btBtness = 0;
		btCol1 = 0;
		btCol2 = 0;
	}

	unsigned char getRGBItem(Colors _color) {
		switch(_color) {
		case clRed:
			return getR();
		case clGreen:
			return getG();
		case clBlue:
			return getB();
		default:
			assert(false);
		}
	}

	unsigned char getR() {
		int nCr = btCol2;
		nCr -= 128;
		int nR = int(btBtness) + nCr + (nCr >> 2) + (nCr >> 3) + (nCr >> 5);
		if (nR < 0)
			nR = 0;
		else if (nR > 255)
			nR = 255;
		return nR;
	}

	unsigned char getG() {
		int nCb = btCol1;
		nCb -= 128;
		int nCr = btCol2;
		nCr -= 128;
		int nG = int(btBtness) - (nCb >> 2) + (nCb >> 4) + (nCb >> 5) - (nCr >> 1) + (nCr >> 3) + (nCr >> 4) + (nCr >> 5);
		if (nG < 0)
			nG = 0;
		else if (nG > 255)
			nG = 255;
		return nG;
	}

	unsigned char getB() {
		int nCb = btCol1;
		nCb -= 128;
		int nCr = btCol2;
		nCr -= 128;
		int nB = int(btBtness) + nCb + (nCb >> 1) + (nCb >> 2) + (nCb >> 6);
		if (nB < 0)
			nB = 0;
		else if (nB > 255)
			nB = 255;
		return nB;
	}
};

struct IntColor {
	int nBtness;
	int nCol1;
	int nCol2;

	IntColor() : nBtness(0), nCol1(0), nCol2(0) { }
	IntColor(unsigned char _btBtness, unsigned char _btCol1, unsigned char _btCol2) : nBtness(_btBtness), nCol1(_btCol1), nCol2(_btCol2) {

	}

	void operator +=(const Color &_other)  {
		 nBtness += _other.btBtness;
		 nCol1 += _other.btCol1;
		 nCol2 += _other.btCol2;
	}

	void operator/= (int _nVal)  {
		 nBtness /= _nVal;
		 nCol1 /= _nVal;
		 nCol2 /= _nVal;
	}

	bool isSet() const {
		return nBtness != 0 || nCol1 != 0 || nCol2 != 0;
	}

	void reset() {
		nBtness = 0;
		nCol1 = 0;
		nCol2 = 0;
	}

	Color toColor() const {
		return Color(nBtness, nCol1, nCol2);
	}
};

struct ColorsRange {
	Color min;
	Color max;

	ColorsRange() : min(255, 255, 255) {

	}

	bool contains(const Color &_color) const {
		return _color.btBtness <= max.btBtness && _color.btBtness >= min.btBtness &&
			_color.btCol1 <= max.btCol1 && _color.btCol1 >= min.btCol1 &&
			_color.btCol2 <= max.btCol2 && _color.btCol2 >= min.btCol2
		;
	}

	bool contains(ColorsRange &_other) const {
		return contains(_other.min) && contains(_other.max);
	}

	void extend(const Color &_color) {
		if (_color.btBtness < min.btBtness)
			min.btBtness = _color.btBtness;

		if (_color.btCol1 < min.btCol1)
			min.btCol1 = _color.btCol1;

		if (_color.btCol2 < min.btCol2)
			min.btCol2 = _color.btCol2;

		if (_color.btBtness > max.btBtness)
			max.btBtness = _color.btBtness;

		if (_color.btCol1 > max.btCol1)
			max.btCol1 = _color.btCol1;

		if (_color.btCol2 > max.btCol2)
			max.btCol2 = _color.btCol2;
	}

	void extendIntervals(unsigned char _btBtness, unsigned char _btCol1, unsigned char _btCol2) {
		if (min.btBtness > _btBtness)
			min.btBtness = min.btBtness - _btBtness;
		else
			min.btBtness = 0;

		if (min.btCol1 > _btCol1)
			min.btCol1 = min.btCol1 - _btCol1;
		else
			min.btCol1 = 0;

		if (min.btCol2 > _btCol2)
			min.btCol2 = min.btCol2 - _btCol2;
		else
			min.btCol2 = 0;

		if (max.btBtness < 255 - _btBtness)
			max.btBtness = max.btBtness + _btBtness;
		else
			max.btBtness = 255;

		if (max.btCol1 < 255 - _btCol1)
			max.btCol1 = max.btCol1 + _btCol1;
		else
			max.btCol1 = 255;

		if (max.btCol2 < 255 - _btCol2)
			max.btCol2 = max.btCol2 + _btCol2;
		else
			max.btCol2 = 255;
	}
};

inline
int getYUVBrightnessIndex(const Vector2D &_p) {
	return _p.x + _p.y*g_nWidth;
}

inline
int getYUVCol1Index(const Vector2D &_p) {
	constexpr int nBeginColIdx = g_nWidth*g_nHeight;
	return nBeginColIdx + (_p.x&(~1)) + (_p.y>>1)*g_nWidth;
}

inline
void getYUVColor(unsigned char *_pBuf, const Vector2D &_p, Color &_color) {
	const int nIdx = getYUVBrightnessIndex(_p);
	_color.btBtness = _pBuf[nIdx];
	const int nColIdx = getYUVCol1Index(_p);
	_color.btCol1 = _pBuf[nColIdx];
	_color.btCol2 = _pBuf[nColIdx + 1];
}

#endif //_COLORS_H_
