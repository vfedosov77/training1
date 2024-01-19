#include "Grid.h"

const Rect2D Grid::gridRect(0, 0, Grid::ms_nWidth, Grid::ms_nHeight);

Grid::RectCellsIterator::RectCellsIterator(const Grid &_grid, const Rect2D &_rect) : m_grid(_grid), m_rect(_rect), m_pRowBegin(nullptr), m_pCurrent(nullptr) {
	int nCurXCell = _rect.nX0/ms_nWidthOfCell;
	if (nCurXCell >= ms_nWidth)
		return;

	if (nCurXCell < 0) {
		if (_rect.nX1 < 0)
			return;

		nCurXCell = 0;
	}

	int nCurIdx = nCurXCell + (_rect.nY0/ms_nHeightOfCell)*ms_nWidth;

	if (nCurIdx >= ms_nSize)
		return;

	if (nCurIdx < 0) {
		if (_rect.nY1 < 0)
			return;

		nCurIdx = nCurXCell;
	}

	m_pRowBegin = m_pCurrent = m_grid.m_cells + nCurIdx;
}

bool Grid::RectCellsIterator::moveNext() {
	if (m_pCurrent == nullptr)
		return false;

	++m_pCurrent;
	if (m_pCurrent == m_grid.m_pEnd) {
		m_pCurrent = nullptr;
		return false;
	}

	if (!m_pCurrent->rect.intersects(m_rect)) {
		m_pRowBegin += ms_nWidth;
		if (m_pRowBegin >= m_grid.m_pEnd || !m_pRowBegin->rect.intersects(m_rect)) {
			m_pCurrent = nullptr;
			return false;
		}

		m_pCurrent = m_pRowBegin;
	}

	return true;
}
