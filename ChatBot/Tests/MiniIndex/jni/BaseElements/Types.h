#ifndef _JNI_TYPES_H_
#define _JNI_TYPES_H_

#include <limits>
#include <assert.h>
#include <math.h>
#include <inttypes.h>

typedef unsigned char byte;

template<typename _CoordType>
struct CoordTraits {
	typedef _CoordType MultTypeTrait;
};
template<>
struct CoordTraits<int> {
	typedef int64_t MultTypeTrait;
};

inline
int32_t fast_floor(float _f) {
	assert(_f < std::numeric_limits<int>::max() && _f > std::numeric_limits<int>::min());
	if (sizeof(intptr_t) == 8)
		return (int32_t)::floorf(_f); // TODO: implement fast_floor for 64 bit platforms

	uint32_t dw = reinterpret_cast<uint32_t &> (_f);
	if (int32_t(dw) < 0) {
		//
		// For negative values.
		//
		dw &= 0x7FFFFFFF;
		if (dw == 0)
			return 0;

		const int32_t sh = 23 + 127 - (dw >> 23);
		if (sh >= 24)
			return -1;
		else if (sh < 0) {
			// NOTE: precision is lost.
			return -int32_t((0x00800000 | (dw & 0x007FFFFF)) << (-sh));
		} else {
			if (dw & (0x007FFFFF >> (23 - sh)))
				// NOTE: the number has fractional part.
				return -int32_t((0x00800000 | (dw & 0x007FFFFF)) >> sh) - 1;
			else
				// NOTE: the number is whole.
				return -int32_t((0x00800000 | (dw & 0x007FFFFF)) >> sh);
		}
	} else {
		//
		// For positive values.
		//
		if (dw == 0)
			return 0;

		const int32_t sh = 23 + 127 - (dw >> 23);
		if (sh >= 24)
			return 0;
		else if (sh < 0)
			// NOTE: the precision is lost.
			return (0x00800000 | (dw & 0x007FFFFF)) << (-sh);
		else
			return (0x00800000 | (dw & 0x007FFFFF)) >> sh;
	}
}

class Counted {
	mutable size_t m_cRefsCount;

	Counted(const Counted &_other);
public:
	Counted() : m_cRefsCount(0) {

	}

	void addRef() const {
		++m_cRefsCount;
	}

	bool removeRef() const {
		assert(m_cRefsCount > 0);
		--m_cRefsCount;
		return m_cRefsCount == 0;
	}

	size_t getRefsCount() const {
		return m_cRefsCount;
	}
};

template<class T>
class Holder {
	T *m_pItem;
public:
	Holder() : m_pItem(nullptr) {

	}

	Holder(T *_pItem) : m_pItem(_pItem) {
		m_pItem->addRef();
	}

	Holder(const Holder &_other) : m_pItem(_other.m_pItem) {
		if (m_pItem != nullptr)
			m_pItem->addRef();
	}

	Holder &operator=(const Holder &_other) {
		T *pPrev = m_pItem;

		m_pItem = _other.m_pItem;
		if (m_pItem != nullptr)
			m_pItem->addRef();

		if (pPrev != nullptr && pPrev->removeRef())
			delete pPrev;
	}

	~Holder() {
		if (m_pItem->removeRef())
			delete m_pItem;
	}

	T *operator->() {
		assert(m_pItem != nullptr);
		return m_pItem;
	}

	const T *operator->() const {
		assert(m_pItem != nullptr);
		return m_pItem;
	}

	T &get() {
		assert(m_pItem != nullptr);
		return *m_pItem;
	}

	const T &get() const {
		assert(m_pItem != nullptr);
		return *m_pItem;
	}

	bool empty() const {
		return m_pItem != nullptr;
	}
};

#endif //_JNI_TYPES_H_
