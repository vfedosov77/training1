#ifndef _DYNAMIC_GRID_H_
#define _DYNAMIC_GRID_H_

#include "Application.h"
#include "Pictures/LevelData.h"

template<size_t _nThreshold>
class DynamicGrid {
public:
	static constexpr int ms_nStepShift = 1;
	static constexpr int ms_nStep = 1<<ms_nStepShift;
	static constexpr int ms_nWidthOfCell = 16;
	static constexpr int ms_nCellRowSize = ms_nWidthOfCell/ms_nStep;
	static constexpr int ms_nHeightOfCell = 16;
	static constexpr int ms_nWidthMask = ms_nWidthOfCell - 1;
	static constexpr int ms_nHeightMask = ms_nHeightOfCell - 1;
	static constexpr size_t ms_cCellSize = ms_nWidthOfCell*ms_nHeightOfCell/(ms_nStep*ms_nStep);
	static constexpr size_t ms_cBufSize = g_cLevelsCount*g_nWidth*g_nHeight/(ms_nStep*ms_nStep);
	static constexpr int ms_nWidth = (g_nWidth - 1)/ms_nWidthOfCell + 1;
	static constexpr int ms_nHeight = (g_nHeight - 1)/ms_nHeightOfCell + 1;
	static constexpr size_t ms_cCellsCountInLevel = ms_nWidth*ms_nHeight;

	struct GridCell {
		int *m_pBuf;

		GridCell() : m_pBuf(nullptr) { }

		inline
		int getValue(int _nCellX, int _nCellY) const {
			assert(isInit());
			assert(_nCellX >= 0 && _nCellY >= 0 && _nCellX < ms_nWidthOfCell && _nCellY < ms_nHeightOfCell);
			int *pRes = m_pBuf + _nCellX/ms_nStep + (_nCellY/ms_nStep)*ms_nCellRowSize;
			assert(pRes >= m_pBuf && pRes < m_pBuf + ms_cCellSize);
			return *pRes;
		}

		inline
		bool isInit() const {
			return m_pBuf != nullptr;
		}


		inline
		void calculate(int _nLevelId, int _nXCellIdx, int _nYCellIdx, DynamicGrid &_grid) {
			assert(m_pBuf != nullptr);
			const LevelConstants &constants = _grid.m_levels[_nLevelId].constants;
			const IntegralTransform &_integral = _grid.m_integral;

			int nStartX = 0;
			int nStartY = 0;
			int nEndDeltaX = 0;
			int nEndDeltaY = 0;

			const int nScreenEdgeMinDistance = constants.nScreenEdgeMinDistance;//((constants.nScreenEdgeMinDistance - 1)/ms_nStep + 1)*ms_nStep;
			int nX = _nXCellIdx*ms_nWidthOfCell;
			if (nX <= nScreenEdgeMinDistance) {
				nStartX = nScreenEdgeMinDistance - nX;
				nX = nScreenEdgeMinDistance&1;
			} else {
				if (nX + ms_nWidthOfCell >= g_nWidth - nScreenEdgeMinDistance)
					nEndDeltaX = nX + ms_nWidthOfCell - g_nWidth + nScreenEdgeMinDistance + 1;

				nX -= nScreenEdgeMinDistance;
			}

			int nY = _nYCellIdx*ms_nHeightOfCell;
			if (nY <= nScreenEdgeMinDistance) {
				nStartY = nScreenEdgeMinDistance - nY;
				nY = nScreenEdgeMinDistance&1;
			} else {
				if (nY + ms_nHeightOfCell >= g_nHeight - nScreenEdgeMinDistance)
					nEndDeltaY = nY + ms_nHeightOfCell - g_nHeight + nScreenEdgeMinDistance + 1;

				nY -= nScreenEdgeMinDistance;
			}

			if (nStartX > 0 || nStartY > 0 || nEndDeltaX > 0 || nEndDeltaY > 0) {
				::memset(m_pBuf, 0, ms_cCellSize*sizeof(int));
				if (nStartX >= ms_nWidthOfCell || nStartY >= ms_nHeightOfCell)
					return;

				if (nEndDeltaX >= ms_nWidthOfCell || nEndDeltaY >= ms_nHeightOfCell)
					return;
			}

			const int *pInt = _integral.getPointerTo(nX, nY);
			int *pResult = m_pBuf + (nStartY/ms_nStep)*ms_nCellRowSize;

			const int nIntegralRowSize = ms_nStep*_integral.getWidth();
			const int nLastRowIdx = ms_nHeightOfCell - nEndDeltaY;
			const int nEndXStep = nEndDeltaX/ms_nStep;
			const int nResultStartStep = nStartX == 0 ? 0 : (nStartX - 1)/ms_nStep + 1;
			const int nRowSize = ms_nWidthOfCell - nResultStartStep*ms_nStep - nEndDeltaX;
			for (int nRow = nStartY; nRow < nLastRowIdx; nRow += ms_nStep) {
				pResult += nResultStartStep;
				const int *pNext = pInt + nIntegralRowSize;
				const int *pRowEnd = pInt + nRowSize;

				for (; pInt < pRowEnd; pInt += ms_nStep) {
					assert(constants.testBounds(pInt, _integral.begin(), _integral.end()));
					*pResult = constants.calculate(pInt, _nThreshold);
					assert(pInt >= _integral.begin() && pInt < _integral.end());
					assert(pResult < _grid.m_pEnd);
					++pResult;

				}

				pResult += nEndXStep;

				pInt = pNext;
				assert((pResult - m_pBuf)%(ms_nWidthOfCell/ms_nStep) == 0);
			}

			assert(pResult == m_pBuf + ms_cCellSize || nEndDeltaY != 0);
		}

		inline
		void init(int _nLevelId, int _nXCellIdx, int _nYCellIdx, DynamicGrid &_grid) {
			assert(m_pBuf == nullptr);
			m_pBuf = _grid.m_pBuf + (_nXCellIdx + _nYCellIdx*ms_nWidth + ms_cCellsCountInLevel*_nLevelId)*ms_cCellSize;
			assert(m_pBuf < _grid.m_pEnd);
			calculate(_nLevelId, _nXCellIdx, _nYCellIdx, _grid);
		}
	};

	DynamicGrid(const std::vector<LevelData> &_levels, const IntegralTransform &_integral) :
		m_levels(_levels), m_integral(_integral), m_pBuf(nullptr), m_pEnd(nullptr)
	{

	}

	~DynamicGrid() {
		delete[] m_pBuf;
	}

	bool isInitialized() const {
		return m_pBuf != nullptr;
	}

	void initialize() {
		assert(m_pBuf == nullptr);
		m_pBuf = new int[ms_cBufSize];
		m_pEnd = m_pBuf + ms_cBufSize;
		m_levelsCells.resize(g_cLevelsCount);

		for (auto iCell = m_levelsCells.begin(); iCell != m_levelsCells.end(); ++iCell)
			iCell->resize(ms_cCellsCountInLevel);
	}

	inline
	GridCell &getCell(std::vector<GridCell> &_levelCells, int _nXCellIdx, int _nYCellIdx) {
		assert(_nXCellIdx >= 0 && _nXCellIdx < ms_nWidth && _nYCellIdx >= 0 && _nYCellIdx < ms_nHeight);
		assert(!m_levelsCells.empty());
		const size_t cCellIdx = _nXCellIdx + getYOffset(_nYCellIdx);
		assert(cCellIdx < ms_cCellsCountInLevel);
		GridCell &cell = _levelCells[cCellIdx];
		if (!cell.isInit()) {
			const int nLevelId = &_levelCells - &*m_levelsCells.begin();
			assert(nLevelId >= 0 && nLevelId < g_cLevelsCount);
			cell.init(nLevelId, _nXCellIdx, _nYCellIdx, *this);
		}

		return cell;
	}

	inline
	GridCell &getCell(int _nLevelId, int _nXCellIdx, int _nYCellIdx) {
		return getCell(m_levelsCells[_nLevelId], _nXCellIdx, _nYCellIdx);
	}

	inline
	int getValue(std::vector<GridCell> &_levelCells, int _nX, int _nY) {
		return getCell(_levelCells, _nX/ms_nWidthOfCell, _nY/ms_nHeightOfCell).getValue(_nX&ms_nWidthMask, _nY&ms_nHeightMask);
	}

	inline
	int getValue(int _nLevelId, int _nX, int _nY) {
		return getCell(_nLevelId, _nX/ms_nWidthOfCell, _nY/ms_nHeightOfCell).getValue(_nX&ms_nWidthMask, _nY&ms_nHeightMask);
	}

	std::vector<GridCell> &getLevelCells(int _nLevelId) {
		return m_levelsCells[_nLevelId];
	}

private:
	//returns _nY*(640/16)
	static inline
	int getYOffset(int _nY) {
		assert(ms_nWidthOfCell == 16 && g_nWidth == 640);
		return (_nY<<5) + (_nY<<3);
	}

	const std::vector<LevelData> &m_levels;
	const IntegralTransform &m_integral;

	std::vector<std::vector<GridCell>> m_levelsCells;
	int *m_pBuf;
	int *m_pEnd;
};

#endif //_DYNAMIC_GRID_H_
