#include <math.h>
#include <cstdlib>
#include <algorithm>
#include <set>
#include <iostream>
#include <map>
#include <fstream>
#include <vector>
#include <list>

#include "Sequences.h"
#include "BaseElements/BresenhamIterator.h"
#include "BaseElements/Primitives2D.h"
#include "Draw/Dumper.h"
#include "Application.h"

namespace {
enum Directions {
	None = -1,
	Top = 0,
	TopRight = 1,
	Right = 2,
	BottomRight = 3,
	Bottom = 4,
	BottomLeft = 5,
	Left = 6,
	TopLeft = 7
};

constexpr float c_PI = 3.14159265358979f;
const float g_fTan22deg = ::tan(c_PI/8);
constexpr int g_nThreshold = 5;
constexpr size_t g_cLineMinPoints = 30;
constexpr size_t g_cLineMinPoints2 = g_cLineMinPoints*g_cLineMinPoints;
constexpr int g_nGenerationPoints = 6;
constexpr int g_nSequenceMinPoints = 10;
const Rect2D g_processedRect(1, 1, g_nWidth - 2, g_nHeight - 2);

constexpr int g_nDirectionsCount = 8;
Directions *g_pDirections = new Directions[g_nWidth*g_nHeight];
int *g_pDiffrences = new int[g_nWidth*g_nHeight];
constexpr int g_nNumOfPassed = g_nWidth*g_nHeight/32;
int *g_pPassed = new int[g_nNumOfPassed];

const int shifts[] = {-g_nWidth, 1 - g_nWidth, 1, 1 + g_nWidth, g_nWidth, g_nWidth - 1, -1, -1 - g_nWidth};

std::vector<Edge> g_edges;

struct Sequence {
	std::list<Vector2D> points;
	IntColor fromColor;//	for (Edge &e : edgesList)
	//		cv::line(image1, cv::Point2f(e.v1.x, e.v1.y), cv::Point2f(e.v2.x, e.v2.y), cv::Scalar(255,255,255));

	IntColor toColor;
	int fromColorsCount;
	int toColorsCount;

	mutable Vector3D mainPoint;
	mutable float fProjValue;

	void clear() {
		points.clear();
		fromColor.reset();
		toColor.reset();
		fromColorsCount = 0;
		toColorsCount = 0;
		mainPoint = Vector3D::ms_emptyVector;
	}

	void addFromColor(Color _c) {
		fromColor += _c;
		++fromColorsCount;
	}

	void addToColor(Color _c) {
		toColor += _c;
		++toColorsCount;
	}

	void calcAverageColors() {
		fromColor /= fromColorsCount;
		toColor /= toColorsCount;
		fromColorsCount = 0;
		toColorsCount = 0;
	}
};

std::list<Sequence> g_sequences;
unsigned char *g_pCurrentData;

const Rect2D g_screenRect(1, 1, 639, 479);
}

struct Generation {
	Vector2D points[g_nGenerationPoints];
	int nPoints;

	Generation() : nPoints(0) { }

	void push_back(const Vector2D &_point) {
		if (nPoints == g_nGenerationPoints)
			return;

		points[nPoints++] = _point;
	}

	const Vector2D *begin() const {
		return points;
	}

	Vector2D *begin() {
		return points;
	}

	const Vector2D *end() const {
		return points + nPoints;
	}

	Vector2D *end() {
		return points + nPoints;
	}
};
struct Generations {
	Generation generations[4*g_nWidth];
	int nMin;
	int nMax;

	static const int ms_nMiddle = 2*g_nWidth;

	Generations() : nMin(ms_nMiddle), nMax(ms_nMiddle) {
		init(nMax);
	}

	Generation &getCurrentForward() {
		assert(nMax >= ms_nMiddle);
		return generations[nMax];
	}

	Generation &getCurrentBackward() {
		assert(nMin < ms_nMiddle);
		return generations[nMin];
	}

	void init(int _n) {
		assert(_n >= 0 && _n < 4*g_nWidth);
		generations[_n].nPoints = 0;
	}

	int addForward() {
		assert(nMax < 4*g_nWidth - 1);
		++nMax;
		init(nMax);
		return nMax;
	}

	int addBackward() {
		assert(nMin > 0);
		--nMin;
		init(nMin);
		return nMin;
	}

	void clear() {
		nMin = nMax = ms_nMiddle;
		init(nMax);
	}

	void removeEmpty() {
		if (nMax != nMin && getCurrentForward().nPoints == 0)
			--nMax;

		if (nMax != nMin && getCurrentBackward().nPoints == 0)
			++nMin;
	}

	size_t size() const {
		return nMax - nMin + 1;
	}
} g_generations;

static
void _clearPassed() {
	const int *pEnd = g_pPassed + g_nNumOfPassed;
	for (int *p = g_pPassed; p != pEnd; ++p)
		*p = 0;
}

static
void _setPassed(int _idx) {
	const int nBitIdx = _idx&31;
	int &nWord = g_pPassed[_idx>>5];
	nWord |= (1<<nBitIdx);
}

static
void _resetPassed(int _idx) {
	const int nBitIdx = _idx&31;
	int &nWord = g_pPassed[_idx>>5];
	nWord &= ~(1<<nBitIdx);
}

static
bool _isPassed(int _idx) {
	const int nBitIdx = _idx&31;
	const int nWord = g_pPassed[_idx>>5];
	return (nWord&(1<<nBitIdx)) != 0;
}

static
Directions _getDirection(int _nX, int _nY) {
	if (_nX == 0 && _nY == 0)
		return None;

	int nDx = std::abs(_nX);
	int nDy = std::abs(_nY);

	bool bIsDiagonal = false;
	bool bIsVert = true;

	if (nDy < /*g_fTan22deg**/(nDx>>1))
		bIsVert = false;
	else if (nDx >= /*g_fTan22deg**/(nDy>>1)) {
		bIsVert = false;
		bIsDiagonal = true;
	}

	if (_nX >= 0 && _nY >= 0)
		return bIsDiagonal ? BottomRight : (bIsVert ? Bottom : Right);

	if (_nX < 0 && _nY >= 0)
		return bIsDiagonal ? BottomLeft : (bIsVert ? Bottom : Left);

	if (_nX >= 0 && _nY < 0)
		return bIsDiagonal ? TopRight : (bIsVert ? Top : Right);

	return bIsDiagonal ? TopLeft : (bIsVert ? Top : Left);
}

static inline
void _getDelta(int _nShift, int &_nDx, int&_nDy) {
	const int xShift = _nShift&0x03;
	_nDx = xShift == 0 ? 0 : (xShift == 1 ? 1 : -1);
	const int yShift = _nShift - _nDx;
	_nDy = yShift == 0 ? 0 : (yShift == g_nWidth ? 1 : -1);
}

static
Directions _getDirectionAndDiff(int _nIndex, unsigned char *_pData, int &_nDiff) {
	const int nCurrent = _pData[_nIndex];

	const int nx = nCurrent - _pData[_nIndex - 1];
	const int ny = nCurrent - _pData[_nIndex - g_nWidth];
	_nDiff = std::abs(nx) + std::abs(ny);

	if (_nDiff < g_nThreshold)
		return None;

	return _getDirection(nx, ny);
}

static
void _fillDirs(unsigned char *_pData) {
	const int nBottomIdx = (g_nHeight - 1)*g_nWidth;
	for (int i = 0; i < g_nWidth; ++i) {
		g_pDirections[i] = None;
		g_pDirections[nBottomIdx + i] = None;
		g_pDiffrences[i] = 0;
		g_pDiffrences[nBottomIdx + i] = 0;
	}

	for (int j = 0; j < g_nHeight; ++j) {
		const int nIdx = j*g_nWidth;
		g_pDirections[nIdx] = None;
		g_pDirections[nIdx + g_nWidth - 1] = None;
		g_pDiffrences[nIdx] = 0;
		g_pDiffrences[nIdx + g_nWidth - 1] = 0;
	}

	int nLeftIdx = g_nWidth;
	for (int j = 1; j < g_nHeight - 1; ++j) {
		 for (int i = 1; i < g_nWidth - 1; ++i) {
			int nDiff;
			const int nCurIdx = nLeftIdx + i;
			const Directions d = _getDirectionAndDiff(nCurIdx, _pData, nDiff);
			g_pDirections[nCurIdx] = d;
			g_pDiffrences[nCurIdx] = nDiff;
		}

		nLeftIdx += g_nWidth;
	}
}

static
Directions _prevDir(Directions dir) {
   assert (dir != None);

   if (dir == Top)
	   return TopLeft;

   return Directions(dir - 1);
}

static
Directions _nextDir(Directions dir) {
   assert (dir != None);

   if (dir == TopLeft)
	   return Top;

   return Directions(dir + 1);
}

static
int _getDirX(Directions dir) {
	assert (dir != None);

	if (dir >= TopRight && dir <= BottomRight)
		return 1;

	if (dir >= BottomLeft)
		return -1;

	return 0;
}

static
int _getDirY(Directions dir) {
	assert (dir != None);

	if (dir >= BottomRight && dir <= BottomLeft)
		return 1;

	if (dir <= TopRight || dir == TopLeft)
		return -1;

	return 0;
}

static
Directions _getEdgeByGradient(Directions _grad) {
	assert (_grad != None);

	if (_grad > 1)
		return Directions(_grad - 2);
	else
		return Directions(_grad + 6);
}

static
Directions _getReverseEdgeByGradient(Directions _grad) {
	assert (_grad != None);

	if (_grad > 5)
		return Directions(_grad - 6);
	else
		return Directions(_grad + 2);
}

static inline
bool _dirsContains(Directions *_pUsedDirs, int _nDirsCount,  Directions _dir) {
	assert(_nDirsCount <= 3 && _nDirsCount > 0);
	if (_pUsedDirs[0] == _dir)
		return true;

	if (_nDirsCount == 1)
		return false;

	if (_pUsedDirs[1] == _dir)
		return true;

	if (_nDirsCount == 2)
		return false;

	return _pUsedDirs[2] == _dir;
}

static
bool _isGood(Directions *_pAllowedDirs, int _nDirsCount, Directions _dir) {
	return _nDirsCount < 3 || _dirsContains(_pAllowedDirs, _nDirsCount, _dir);
}

static
bool _isGood(Directions *_pAllowedDirs, int _nDirsCount, Directions _dir, int &_nMaxCount) {
	bool bContains = _dirsContains(_pAllowedDirs, _nDirsCount, _dir);

	if (!bContains && _nDirsCount >= _nMaxCount)
		return false;

	if (!bContains)
		--_nMaxCount;

	return true;
}

void _addToAllowed(Directions *_pAllowedDirs, int &_nDirsCount, Directions _dir) {
	if (_dirsContains(_pAllowedDirs, _nDirsCount, _dir))
		return;

	assert(_nDirsCount < 3);
	_pAllowedDirs[_nDirsCount] = _dir;
	++_nDirsCount;
}

static inline
bool _isAllowed(Directions _dir1, Directions _dir2) {
	return _dir1 == _dir2 || _nextDir(_dir1) == _dir2 || _prevDir(_dir1) == _dir2;
}

Sequence g_currentSequence;

static
void _processFirstGenerationPoint(Vector2D _curPoint, Directions _dir) {
	Color c;
	_curPoint.x -= _getDirX(_dir);
	_curPoint.y -= _getDirY(_dir);
	getYUVColor(g_pCurrentData, _curPoint, c);
	g_currentSequence.addFromColor(c);
}

static
void _processLastGenerationPoint(Vector2D _curPoint, Directions _dir) {
	Color c;
	_curPoint.x += _getDirX(_dir);
	_curPoint.y += _getDirY(_dir);
	getYUVColor(g_pCurrentData, _curPoint, c);
	g_currentSequence.addToColor(c);
}

static
void _fillPointers(Directions *_pAllowedDirs, int &_nDirsCount, const Vector2D &_curPoint, Directions _dir,
	Generation &_seq, int _nDepth = 0
) {
	const int nCurIdx = _curPoint.x + _curPoint.y*g_nWidth;
	_setPassed(nCurIdx);
	_seq.push_back(_curPoint);
	bool bFound = false;
	//Find pointers.
	const Directions near[] = {_dir, _nextDir(_dir), _prevDir(_dir)};
	for (Directions d : near) {
		if (!_isGood(_pAllowedDirs, _nDirsCount, d))
			continue;

		int nId = nCurIdx - shifts[d];

		if (g_pDirections[nId] != d || _isPassed(nId))
			continue;

		Vector2D pointer = _curPoint;
		pointer.x -= _getDirX(d);
		pointer.y -= _getDirY(d);

		_addToAllowed(_pAllowedDirs, _nDirsCount, d);
		assert(g_pDirections[pointer.x + pointer.y*g_nWidth] != None);
		_seq.push_back(pointer);

		_setPassed(nId);
		if (_nDepth < 3) {
			bFound = true;
			_fillPointers(_pAllowedDirs, _nDirsCount, pointer, d, _seq, _nDepth + 1);
		}
	}

	if (!bFound)
		_processFirstGenerationPoint(_curPoint, _dir);
}

static
void _fillGeneration(Directions *_pAllowedDirs, int &_nDirsCount, const Vector2D &_curPoint, Directions _dir,
	Generation &_seq
) {
	Vector2D p = _curPoint;
	int nCurIdx = p.x + p.y*g_nWidth;

	//Find pointers.
	while(true) {
		//_seq.push_back(p);
		//_setPassed(nCurIdx);
		nCurIdx += shifts[_dir];

		const Directions d = g_pDirections[nCurIdx];

		if ( d == None || _isPassed(nCurIdx))
			break;

		if (!_isGood(_pAllowedDirs, _nDirsCount, d) || !_isAllowed(_dir, d))
			break;

		p.x += _getDirX(_dir);
		p.y += _getDirY(_dir);
		assert(g_pDirections[p.x + p.y*g_nWidth] != None);

		_addToAllowed(_pAllowedDirs, _nDirsCount, d);
		_dir = d;
	}

	_fillPointers(_pAllowedDirs, _nDirsCount, p, _dir, _seq);
	_processLastGenerationPoint(p, _dir);
}

static
bool _selectNextPoint(Directions *_pAllowedDirs, int &_nDirsCount, Generation &_curGeneration,
	bool _bReverse, Vector2D &_nextPoint, Directions &_nextDirection
) {
	int nMaxGrad = 0;
	Directions toNext;
	const Vector2D *pEnd = _curGeneration.end();
	for (const Vector2D *pVec = _curGeneration.begin(); pVec != pEnd; ++pVec) {
		const Vector2D &p = *pVec;
		const int nCurIdx = p.x + p.y*g_nWidth;
		if (g_pDiffrences[nCurIdx] <= nMaxGrad)
			continue;

		const Directions d = g_pDirections[nCurIdx];

		int nMaxChildGrad = 0;

		for (int i = 0; i < 3; ++i) {
			Directions curDir;

			switch(i) {
			case 0:
				curDir = d;
				break;
			case 1:
				curDir = _nextDir(d);
				break;
			case 2:
				curDir = _prevDir(d);
				break;
			}

			int nMaxDirs = 3;
			if (!_isGood(_pAllowedDirs, _nDirsCount, curDir, nMaxDirs))
				continue;

			Directions edgeDir = _bReverse ? _getReverseEdgeByGradient(curDir) : _getEdgeByGradient(curDir);
			const int nNextId = nCurIdx + shifts[edgeDir];

			const int nGradient = g_pDiffrences[nNextId];
			if (nGradient < g_nThreshold || nGradient < nMaxChildGrad || _isPassed(nNextId))
				continue;

			Directions nextDir = g_pDirections[nNextId];
			if (!_isGood(_pAllowedDirs, _nDirsCount, nextDir, nMaxDirs) || !_isAllowed(curDir, nextDir))
				continue;

			Vector2D next = p;
			next.x += _getDirX(edgeDir);
			next.y += _getDirY(edgeDir);

			toNext = curDir;
			_nextDirection = nextDir;
			nMaxGrad = g_pDiffrences[nCurIdx];
			_nextPoint = next;
			nMaxChildGrad = nGradient;
		}
	}

	if (nMaxGrad == 0)
		return false;

	_addToAllowed(_pAllowedDirs, _nDirsCount, toNext);
	_addToAllowed(_pAllowedDirs, _nDirsCount,_nextDirection);
	return true;
}

static
void _detectForward(Vector2D curPoint, Directions dir, Directions *_pAllowedDirs, int &_nDirsCount) {
	while (true) {
		Generation &curGeneration = g_generations.getCurrentForward();
		assert(g_pDirections[curPoint.x + curPoint.y*g_nWidth] != None);
		_fillGeneration(_pAllowedDirs, _nDirsCount, curPoint, dir, curGeneration);

		if (!_selectNextPoint(_pAllowedDirs, _nDirsCount, curGeneration, false, curPoint, dir))
			return;

		g_currentSequence.points.push_back(curPoint);
		g_generations.addForward();
	}

	g_generations.removeEmpty();
}

static
void _detectBackward(Vector2D curPoint, Directions dir, Directions *_pAllowedDirs, int &_nDirsCount) {
	_addToAllowed(_pAllowedDirs, _nDirsCount, dir);

	while (true) {
		g_currentSequence.points.push_back(curPoint);

		Generation &curGeneration = g_generations.getCurrentBackward();
		assert(g_pDirections[curPoint.x + curPoint.y*g_nWidth] != None);
		_fillGeneration(_pAllowedDirs, _nDirsCount, curPoint, dir, curGeneration);

		if (!_selectNextPoint(_pAllowedDirs, _nDirsCount, curGeneration, true, curPoint, dir))
			return;

		g_generations.addBackward();
	}

	g_generations.removeEmpty();
}

Vector2D selectPoint(const Generation &generation) {
	double dMax = 0;
	Vector2D pMax;
	const Vector2D *pEnd = generation.end();
	for (const Vector2D *pVec = generation.begin(); pVec != pEnd; ++pVec) {
		const Vector2D &p = *pVec;
		const int nDiff = g_pDiffrences[p.x + p.y*g_nWidth];
		if (nDiff > dMax) {
			dMax = nDiff;
			pMax = p;
		}
	}

	return pMax;
}

static inline
bool _verifyDirection(int _nIdx, Directions _cur, Directions _prev, Directions _next, int _nShift) {
	Directions grad = g_pDirections[_nIdx];

	if (grad == _cur || grad == _prev || grad == _next)
		return true;

	grad = g_pDirections[_nIdx + _nShift];

	if (grad == _cur || grad == _prev || grad == _next)
		return true;

//	grad = g_pDirections[_nIdx - _nShift];

//	if (grad == _cur || grad == _prev || grad == _next)
//		return true;

	return false;
}

void _tryMakeLonger(Edge &_edge) {
	Vector2D vec1 = (_edge.v2 - _edge.v1)/2;
	Vector2D vec2 = (_edge.v1 - _edge.v2)/2;
	const Directions curDir = _getReverseEdgeByGradient(_getDirection(_edge.v2.x - _edge.v1.x, _edge.v2.y - _edge.v1.y));
	const Directions prev = _prevDir(curDir);
	const Directions next = _nextDir(curDir);
	const int nShift = shifts[_getEdgeByGradient(curDir)];

	BresenhamIterator it1 = BresenhamIterator::create(_edge.v2, _edge.v2 + vec1);
	BresenhamIterator it2 = BresenhamIterator::create(_edge.v1, _edge.v1 + vec2);
	it1.moveNext();
	it2.moveNext();

	int nTrash = 0;
	int nNormal = 0;
	do {
		const Vector2D point = it1.point();
		if (!g_processedRect.contains(point))
			break;
		const int nCurIdx = point.x + point.y*g_nWidth;
		const bool bDirCorrect = _verifyDirection(nCurIdx, curDir, prev, next, nShift);
		if (!bDirCorrect) {
			++nTrash;
			nNormal = 0;
			if (nTrash > 1)
				break;
		} else {
			nNormal++;
			if (nNormal > 5) {
				nTrash = 0;
				_edge.v2 = point;
			}
		}
	} while (it1.moveNext());

	nTrash = 0;
	nNormal = 0;
	do {
		const Vector2D point = it2.point();
		if (!g_processedRect.contains(point))
			break;

		const int nCurIdx = point.x + point.y*g_nWidth;
		const bool bDirCorrect = _verifyDirection(nCurIdx, curDir, prev, next, nShift);
		if (!bDirCorrect) {
			++nTrash;
			nNormal = 0;
			if (nTrash > 1)
				break;
		} else {
			nNormal++;
			if (nNormal > 5) {
				nTrash = 0;
				_edge.v1 = point;
			}
		}
	} while (it2.moveNext());
}

constexpr int g_nMaxLinePoints = g_nWidth + g_nHeight;
static Vector2D g_linePoints[g_nMaxLinePoints];

static
bool _pointToLine(int _nRange, float _fK, float _fB, int _nX, int &_nY) {
	const float fY = _fK*_nX + _fB;

	if (std::abs(fY - _nY) > 10 || fY < 0 || fY >= _nRange)
		return false;

	_nY = ::floor(fY + 0.5);
	return true;
}

static
void _ajustEdge(Edge &_edge, int _nStartId, int _nToId) {
	int nFromId = _nStartId + 6;
	_nToId -= 6;

	const bool bSwap = std::abs(g_linePoints[_nToId].x - g_linePoints[nFromId].x) < std::abs(g_linePoints[_nToId].y - g_linePoints[nFromId].y);

	int nXYSumm = 0;
	int nXSumm = 0;
	int nYSumm = 0;
	int nX2Summ = 0;
	int nCount = _nToId - nFromId;

	for (int nGeneration = 0; nGeneration < nCount; ++nGeneration) {
		const int nX = bSwap ? g_linePoints[nGeneration].y : g_linePoints[nGeneration].x;
		const int nY = bSwap ? g_linePoints[nGeneration].x : g_linePoints[nGeneration].y;
		nXYSumm += nX*nY;
		nXSumm += nX;
		nYSumm += nY;
		nX2Summ += nX*nX;
	}

	float fK = (float(nCount)*nXYSumm - float(nXSumm)*nYSumm)/
		(float(nCount)*nX2Summ - float(nXSumm)*nXSumm)
	;

	float fB = (float(nYSumm - fK*nXSumm))/nCount;

	_pointToLine(bSwap ? g_nWidth : g_nHeight, fK, fB, bSwap? _edge.v1.y : _edge.v1.x, bSwap ? _edge.v1.x : _edge.v1.y);
	_pointToLine(bSwap ? g_nWidth : g_nHeight, fK, fB, bSwap? _edge.v2.y : _edge.v2.x, bSwap ? _edge.v2.x : _edge.v2.y);
}

bool split(int _fromId, int _toId) {
	Vector2D p1 = selectPoint(g_generations.generations[_fromId]);
	Vector2D p2 = selectPoint(g_generations.generations[_toId]);
	double nMaxDistance2 = -1;
	int nMaxIdx = 0;
	Edge e(p1, p2);

	if (e.getVector().length2() < int(g_cLineMinPoints2)) {
//		for (int i = fromId; i <= toId; ++i) {
//			Generation &seq = g_generations.generations[i];
//			Vector2D *pEnd = seq.end();

//			for (auto it = seq.begin(); it != pEnd; ++it)
//				_resetPassed(it->x + it->y*g_nWidth);
//		}

		return false;
	}

	const int nStartId = _fromId + 1;
	for (int nId = nStartId; nId < _toId; ++nId) {
		Vector2D p = selectPoint(g_generations.generations[nId]);
		g_linePoints[nId - nStartId] = p;
		float nDist2 = e.distance2FromLine(p);

		if (nDist2 > nMaxDistance2) {
			nMaxDistance2 = nDist2;
			nMaxIdx = nId;
		}
	}

	if (nMaxDistance2 < 8) {
		g_edges.push_back(e);
		_tryMakeLonger(g_edges.back());

		_ajustEdge(g_edges.back(), nStartId, _toId);

		if (g_edges.back().length2() < int(g_cLineMinPoints2)) {
			g_edges.pop_back();
			return false;
		}

		_tryMakeLonger(g_edges.back());
		return true;
	} else {
		bool bAdded = split(_fromId, nMaxIdx);
		bAdded |= split(nMaxIdx, _toId);
		return bAdded;
	}
}

bool _makeLines() {
	return split(g_generations.nMin, g_generations.nMax);
}

static
void _tryDetectLine(Vector2D curPoint) {
	g_generations.clear();
	g_currentSequence.clear();
	Directions allowedDirs[3];
	int nDirsCount = 1;
	allowedDirs[0] = g_pDirections[curPoint.x + curPoint.y*g_nWidth];

	g_currentSequence.points.push_back(curPoint);
	_detectForward(curPoint, allowedDirs[0], allowedDirs, nDirsCount);
	Directions dir;
	if (_selectNextPoint(allowedDirs, nDirsCount, g_generations.generations[Generations::ms_nMiddle], true, curPoint, dir)) {
		g_generations.addBackward();
		_detectBackward(curPoint, dir, allowedDirs, nDirsCount);
	}

	if ((int)g_generations.size() >= g_nSequenceMinPoints) {
		g_currentSequence.calcAverageColors();
		g_sequences.push_back(Sequence());
		g_sequences.back().fromColor = g_currentSequence.fromColor;
		g_sequences.back().toColor = g_currentSequence.toColor;
		g_sequences.back().points.swap(g_currentSequence.points);
	}

	if (g_generations.size() >= g_cLineMinPoints)
		_makeLines();
	else if ((int)g_generations.size() < g_nSequenceMinPoints) {
		for (int i = g_generations.nMin; i <= g_generations.nMax; ++i) {
			Generation &seq = g_generations.generations[i];
			Vector2D *pEnd = seq.end();

			for (auto it = seq.begin(); it != pEnd; ++it)
				_resetPassed(it->x + it->y*g_nWidth);
		}
	}
}

const int g_nMaxDist2ToCombine = 16;

bool isPairCollinear(const Vector2D &_v1, const Vector2D &_v2) {
	float dDotProduct = _v1.dotProduct(_v2);
	return dDotProduct*dDotProduct/(_v1.length2()*_v2.length2()) > 0.96;
}

std::list<Edge>::iterator next(const std::list<Edge>::iterator &it) {
	std::list<Edge>::iterator it2 = it;
	++it2;
	return it2;
}

class ProjectionComparer {
	const Vector2D m_base;
public:
	ProjectionComparer(const Vector2D &_base) : m_base(_base) {

	}

	bool operator() (const Vector2D &_x, const Vector2D &_y) const {
		return _x*m_base < _y*m_base;
	}
};

void combineEdges(std::list<Edge> &_edges) {
	for (auto iFirst = _edges.begin(); iFirst != _edges.end(); ++iFirst) {
		for (auto iSecond = next(iFirst); iSecond != _edges.end();) {
			Edge &first = *iFirst;
			Edge &second = *iSecond;

			if (!isPairCollinear(first.getVector(), second.getVector())) {
				++iSecond;
				continue;
			}

			if (first.distance2(second) > g_nMaxDist2ToCombine) {
				++iSecond;
				continue;
			}

			if (first.distance2(second.v1) <= g_nMaxDist2ToCombine &&
				first.distance2(second.v2) <= g_nMaxDist2ToCombine) {
				iSecond = _edges.erase(iSecond);
				continue;
			}

			if (second.distance2(first.v1) <= g_nMaxDist2ToCombine &&
				second.distance2(first.v2) <= g_nMaxDist2ToCombine) {
				iFirst = _edges.erase(iFirst);
				iSecond = next(iFirst);
				continue;
			}

			bool bHasNear1 = first.distance2(second.v1) <= g_nMaxDist2ToCombine ||
				first.distance2(second.v2) <= g_nMaxDist2ToCombine;

			bool bHasNear2 = second.distance2(first.v1) <= g_nMaxDist2ToCombine ||
				second.distance2(first.v2) <= g_nMaxDist2ToCombine;

			if (bHasNear1 && bHasNear2) {
				Vector2D points[] = {first.v1, first.v2, second.v1, second.v2};
				std::sort(points, points + 4, ProjectionComparer(first.getVector()));
				first.v1 = points[0];
				first.v2 = points[3];
				iSecond = _edges.erase(iSecond);
				continue;
			}

			++iSecond;
		}
	}
}

int g_nCurIndex = 0;

void saveShort(int _nValue, unsigned char *_pData) {
	_pData[g_nCurIndex] = _nValue&0xFF;
	_pData[g_nCurIndex + 1] = (_nValue&0xFF00)>>8;
	g_nCurIndex += 2;
}

void savePoint(const Vector2D &_vec, unsigned char *_pData) {
	saveShort(_vec.x, _pData);
	saveShort(_vec.y, _pData);
}

void saveEdge(const Edge &_edge, unsigned char *_pData) {
	savePoint(_edge.v1, _pData);
	savePoint(_edge.v2, _pData);
}

