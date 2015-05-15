#pragma once

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

};
