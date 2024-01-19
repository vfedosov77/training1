#ifndef _ACCELEROMETER_
#define _ACCELEROMETER_

#include "Primitives3D.h"

class Accelerometer {
	static const int g_nAccelerometerDataIndex = 640*480*3/2;

	float fValues[3];
	Vector3D m_vertDirection;
	Plane m_earth;

public:
	Accelerometer(unsigned char *_data) {
		update(_data);
	}

	void update(unsigned char *_data) {
		fValues[0] = ((unsigned int)_data[g_nAccelerometerDataIndex + 0]) + ((unsigned int)_data[g_nAccelerometerDataIndex + 1])*256;
		fValues[1] = ((unsigned int)_data[g_nAccelerometerDataIndex + 2]) + ((unsigned int)_data[g_nAccelerometerDataIndex + 3])*256;
		fValues[2] = ((unsigned int)_data[g_nAccelerometerDataIndex + 4]) + ((unsigned int)_data[g_nAccelerometerDataIndex + 5])*256;

		for (int i = 0; i < 3; ++i) {
			if (fValues[i] > 0x8FFF)
				fValues[i] = fValues[i] - 0x10000;

			fValues[i] /= 10000;
		}

		m_vertDirection = Vector3D(-fValues[1], -fValues[0], -fValues[2]);
		m_vertDirection.normalize();
		m_earth = Plane::createEarthPlane(m_vertDirection);
	}

	float getX() {
		return fValues[0];
	}

	float getY() {
		return fValues[1];
	}

	float getZ() {
		return fValues[2];
	}

	const Vector3D &verticalDirection() const {
		return m_vertDirection;
	}

	const Plane &getEarth() const {
		return m_earth;
	}
};

#endif //_ACCELEROMETER_
