#pragma once

#include "egolib/platform.h"

struct LocalParticleProfileRef : public idlib::equal_to_expr<LocalParticleProfileRef>,
                                 public idlib::lower_than_expr<LocalParticleProfileRef>,
                                 public idlib::increment_expr<LocalParticleProfileRef> {
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

	// CRTP
    bool lower_than(const LocalParticleProfileRef& other) const {
        return _value < other._value;
    }

	// CRTP
    bool equal_to(const LocalParticleProfileRef& other) const
    {
        return _value == other._value;
    }

	// CRTP
    void increment()
    {
        if (_value == std::numeric_limits<int>::max())
        {
            std::ostringstream msg;
            msg << __FILE__ << ":" << __LINE__ << ": " << "reference overflow";
            std::overflow_error(msg.str());
        }
        ++_value;
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
