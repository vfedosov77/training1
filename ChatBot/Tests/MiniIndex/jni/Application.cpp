#include "Application.h"

Rect2D Application::ms_screenRect(0, 0, g_nWidth, g_nHeight);

int Application::getFocalLength(byte *_pData) {
	static const int g_nFocalLengthDataIndex = 640*480*3/2 + 6;
	static const float fCoef = 0.15;
	const int nFocalLength = fast_floor((((int)_pData[g_nFocalLengthDataIndex]) + ((int)_pData[g_nFocalLengthDataIndex + 1])*256)*fCoef);
	return nFocalLength;
}
