#pragma once

#include "egolib/typedef.h"
#include "egolib/_math.h"

/**
 * @brief A lookup table for the trigonometric functions sine and cosine.
 */
struct TLT {
    static constexpr size_t bits = 14;
    static constexpr size_t size = 1 << bits;
    static constexpr size_t mask = size - 1;
    static constexpr size_t offset = size >> 2;
private:
    float tosin[size];           ///< Convert TURN_T to sine
    float tocos[size];           ///< Convert TURN_T to cosine

protected:
    TLT() {
        const float step = idlib::two_pi<float>() / float(size);

        for (size_t i = 0; i < size; ++i) {
            float x = i * step;
            tosin[i] = std::sin(x);
            tocos[i] = std::cos(x);
        }
    }

public:
    struct Index {
        uint16_t i;
        Index() : i(0) {}
        Index(uint16_t i) : i(i) {}
        Index(const Index& other) : i(other.i) {}
        const Index& operator=(const Index& other) {
            i = other.i;
            return *this;
        }
    };

    float sin(const Index& i) const {
        return tosin[i.i];
    }

    float cos(const Index& i) const {
        return tocos[i.i];
    }

private:
    // Maps an angle in "facings" x to a trigonometric lookup table index i.
    // The mapping is defined as
    // \f{align*}{
    // i = x >> 2 = \left\lfloor \frac{x}{2^2} \right\rfloor = \left \lfloor \frac{x}{4} \right \rfloor
    // \f}
    // 
    // Pluggin in the greatest value \f$2^16-1\f$ of \f$x\f$ into the above formula
    // gives \f$i = (2^16 - 1) / 4 = \left\lfloor 16383.75 \right\rfloor = 16383
    // = 2^14-1\f$ which is the greatest index of the trigonometric lookup table.
    Index fromFacing(const FACING_T& x) const {
        return Index((x >> 2) & mask);
    }

public:
    Index fromFacing(const Facing& x) const {
        return fromFacing(FACING_T(x));
    }

    static const TLT& get() {
        static const TLT g_instance;
        return g_instance;
    }
};

namespace std {

inline float sin(const Facing& x) {
    auto i = TLT::get().fromFacing(x);
    return TLT::get().sin(i);
}

inline float cos(const Facing& x) {
    auto i = TLT::get().fromFacing(x);
    return TLT::get().cos(i);
}

}
