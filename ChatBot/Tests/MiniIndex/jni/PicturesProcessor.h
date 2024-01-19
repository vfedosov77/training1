#ifndef _PICTURES_PROCESSOR_H_
#define _PICTURES_PROCESSOR_H_

#include <memory>

#include "BaseElements/Types.h"
#include "Pictures/Hessian.h"
#include "FloorDetector.h"

void testMatch(byte *_pBuf, int _nSize);
FloorDetector *staticCompare(Picture &_picture1, Picture &_picture2);
Picture *createPicture(const std::string &_strName);
Picture *createPicture(byte *_pBuf);
#endif
