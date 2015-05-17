#pragma once

#include "egolib/platform.h"

struct LocalParticleProfileRef {
private:
    int _value;
public:
    static const int InvalidValue = -1;
    static const LocalParticleProfileRef Invalid;
    explicit LocalParticleProfileRef(int value)
        : _value(value) {}


    LocalParticleProfileRef(const LocalParticleProfileRef& other)
        : _value(other._value) {}

    LocalParticleProfileRef()
        : _value(-1) {}

    int get() const {
        return _value;
    }

    LocalParticleProfileRef& operator=(const LocalParticleProfileRef& other) {
        _value = other._value;
        return *this;
    }

    bool operator<(const LocalParticleProfileRef& other) const {
        return _value < other._value;
    }

    bool operator<=(const LocalParticleProfileRef& other) const {
        return _value <= other._value;
    }

    bool operator>(const LocalParticleProfileRef& other) const {
        return _value > other._value;
    }

    bool operator>=(const LocalParticleProfileRef& other) const {
        return _value >= other._value;
    }

    bool operator==(const LocalParticleProfileRef& other) const {
        return _value == other._value;
    }

    bool operator!=(const LocalParticleProfileRef& other) const {
        return _value != other._value;
    }

    // Post-increment.
    LocalParticleProfileRef operator++(int) {
        if (_value == std::numeric_limits<int>::max()) {
            std::ostringstream msg;
            msg << __FILE__ << ":" << __LINE__ << ": " << "reference overflow";
            std::overflow_error(msg.str());
        }
        int old = _value;
        ++_value;
        return LocalParticleProfileRef(old);
    }

    LocalParticleProfileRef& operator++() {
        if (_value == std::numeric_limits<int>::max()) {
            std::ostringstream msg;
            msg << __FILE__ << ":" << __LINE__ << ": " << "reference overflow";
            std::overflow_error(msg.str());
        }
        ++_value;
        return *this;
    }

};

namespace std {
    template <>
    struct hash<LocalParticleProfileRef> {
        size_t operator()(const LocalParticleProfileRef& lppref) const {
            return hash<int>()(lppref.get());
        }
    };
    template <>
    struct equal_to<LocalParticleProfileRef> {
        bool operator()(const LocalParticleProfileRef& lhs, const LocalParticleProfileRef& rhs) const {
            return lhs == rhs;
        }
    };
} // namespace std
