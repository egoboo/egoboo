#pragma once

enum class RefKind
{
    Animation,       ///< @todo Formerly known as MAD_REF.
    Billboard,       ///< @see BillboardRef
    Particle,        ///< @see ParticleRef
    ParticleProfile, ///< @see ParticleProfileRef
    Enchant,         ///< @see EnchantRef.
    EnchantProfile,  ///< @see EnchantProfileRef
    Object,          ///< @see ObjectRef
    ObjectProfile,   ///< @see ObjectProfileRef
    Module,          ///< @see ModuleRef
    Player,           /// @todo Formerly known as PLA_REF.
    Texture,         ///< @see TextureRef
};

/**
 * @brief
 *  Type-safe references.
 * @author
 *  Michael Heilmann
 */
template <typename TYPE, TYPE MIN, TYPE MAX, TYPE INVALID, RefKind KIND>
struct Ref
{

private:

    TYPE _ref;

public:


    // Instantiation time static assertions.

    // bool, char, char16_t, char32_t, wchar_t, short, int, long, long long
    // or any implementation-defined extended integer types, including any signed, unsigned, and cv-qualified variants.
    static_assert(std::is_integral<TYPE>::value,
                  "the underlaying type must be an unsigned, arithmetic, integral type");
    // Same as above but without signed variants.
    static_assert(std::is_unsigned<TYPE>::value,
                  "the underlaying type must be an unsigned, arithmetic, integral type");
    // Same as above but without cv-qualified variants.
    static_assert(!std::is_const<TYPE>::value,
                  "the underlaying type must be an unsigned, arithmetic, integral type");

    // Instantiation time static assertions.
    static_assert(std::integral_constant<TYPE, MIN>::value <= 
                  std::integral_constant<TYPE, MAX>::value,
                  "the minimum ref must be smaller than or equal to the maximum ref");
    

    // Static variables.

    static const Ref<TYPE, MIN, MAX, INVALID, KIND> Invalid;
    static const Ref<TYPE, MIN, MAX, INVALID, KIND> Min;
    static const Ref<TYPE, MIN, MAX, INVALID, KIND> Max;


    // Constructs.

    Ref() EGO_NOEXCEPT :
        _ref(INVALID)
    {}

    explicit Ref(const TYPE ref) :
        _ref(ref)
    {
        if (ref < MIN || ref > MAX)
        {
            throw std::domain_error("out of range");
        }
    }

    Ref(const Ref<TYPE, MIN, MAX, INVALID, KIND>& other) EGO_NOEXCEPT :
        _ref(other._ref)
    {}


    // Logical operator overloads.

    bool operator!() const EGO_NOEXCEPT
    {
        return _ref == INVALID;
    }


    // Relational operator overloads.

    bool operator>=(const Ref<TYPE, MIN, MAX, INVALID, KIND>& other) const EGO_NOEXCEPT
    {
        return _ref >= other._ref;
    }

    bool operator<=(const Ref<TYPE, MIN, MAX, INVALID, KIND>& other) const EGO_NOEXCEPT
    {
        return _ref <= other._ref;
    }

    bool operator<(const Ref<TYPE, MIN, MAX, INVALID, KIND>& other) const EGO_NOEXCEPT
    {
        return _ref < other._ref;
    }

    bool operator>(const Ref<TYPE, MIN, MAX, INVALID, KIND>& other) const EGO_NOEXCEPT
    {
        return _ref > other._ref;
    }

    bool operator==(const Ref<TYPE, MIN, MAX, INVALID, KIND>& other) const EGO_NOEXCEPT
    {
        return _ref == other._ref;
    }

    bool operator!=(const Ref<TYPE, MIN, MAX, INVALID, KIND>& other) const EGO_NOEXCEPT
    {
        return _ref != other._ref;
    }


    // Assignment operator overloads.

    Ref<TYPE, MIN, MAX, INVALID, KIND>& operator=(const Ref<TYPE, MIN, MAX, INVALID, KIND>& other) EGO_NOEXCEPT
    {
        _ref = other._ref;
        return *this;
    }

#define REF_WITH_IMPLICITASSIGNMENT 1
#if defined(REF_WITH_IMPLICITASSIGNMENT) && 1 == REF_WITH_IMPLICITASSIGNMENT
    Ref<TYPE, MIN, MAX, INVALID, KIND>& operator=(const unsigned int& other)
    {
        if (other < MIN || other > MAX)
        {
            throw std::domain_error("out of range");
        }
        _ref = other;
        return *this;
    }

    Ref<TYPE, MIN, MAX, INVALID, KIND>& operator=(const int& other)
    {
        if (other < MIN || other > MAX)
        {
            throw std::domain_error("out of range");
        }
        _ref = other;
        return *this;
    }

    Ref<TYPE, MIN, MAX, INVALID, KIND>& operator=(const unsigned long& other)
    {
        if (other < MIN || other > MAX)
        {
            throw std::domain_error("out of range");
        }
        _ref = other;
        return *this;
    }

    Ref<TYPE, MIN, MAX, INVALID, KIND>& operator=(const long& other)
    {
        if (other < MIN || other > MAX)
        {
            throw std::domain_error("out of range");
        }
        _ref = other;
        return *this;
    }
#endif

    // Cast operator overloads.
    explicit operator bool() const EGO_NOEXCEPT
    {
        return INVALID != _ref;
    }

    // Accessors.

    TYPE get() const EGO_NOEXCEPT
    {
        return _ref;
    }


    // Post- and pre-increment operator overloads.

    Ref<TYPE, MIN, MAX, INVALID, KIND> operator++(int)
    {
        if (_ref == MAX)
        {
            std::ostringstream msg;
            msg << __FILE__ << ":" << __LINE__ << ": " << "reference overflow";
            std::overflow_error(msg.str());
        }
        _ref++;
        return *this;
    }

    Ref<TYPE, MIN, MAX, INVALID, KIND>& operator++()
    {
        if (MAX == _ref)
        {
            std::ostringstream msg;
            msg << __FILE__ << ":" << __LINE__ << ": " << "reference overflow";
            std::overflow_error(msg.str());
        }
        ++_ref;
        return *this;
    }


    // Post- and pre-decrement operator overloads.

    Ref<TYPE, MIN, MAX, INVALID, KIND> operator--(int)
    {
        if (0 == _ref)
        {
            std::ostringstream msg;
            msg << __FILE__ << ":" << __LINE__ << ": " << "reference underflow";
            std::underflow_error(msg.str());
        }
        size_t ref = _ref;
        _ref--;
        return Ref<TYPE, MIN, MAX, INVALID, KIND>(ref);
    }

    Ref<TYPE, MIN, MAX, INVALID, KIND>& operator--()
    {
        if (0 == _ref)
        {
            std::ostringstream msg;
            msg << __FILE__ << ":" << __LINE__ << ": " << "reference underflow";
            std::underflow_error(msg.str());
        }
        --_ref;
        return *this;
    }

};

template <typename TYPE, TYPE MIN, TYPE MAX, TYPE INVALID, RefKind KIND>
const Ref<TYPE, MIN, MAX, INVALID, KIND> Ref<TYPE, MIN, MAX, INVALID, KIND>::Invalid(INVALID);

template <typename TYPE, TYPE MIN, TYPE MAX, TYPE INVALID, RefKind KIND>
const Ref<TYPE, MIN, MAX, INVALID, KIND> Ref<TYPE, MIN, MAX, INVALID, KIND>::Min(MIN);

template <typename TYPE, TYPE MIN, TYPE MAX, TYPE INVALID, RefKind KIND>
const Ref<TYPE, MIN, MAX, INVALID, KIND> Ref<TYPE, MIN, MAX, INVALID, KIND>::Max(MAX);

namespace std
{
    template <typename TYPE, TYPE MIN, TYPE MAX, TYPE INVALID, RefKind KIND>
    struct hash<Ref<TYPE, MIN, MAX, INVALID, KIND>>
    {
        size_t operator()(const Ref<TYPE, MIN, MAX, INVALID, KIND>& ref) const
        {
            return hash<TYPE>()(ref.get());
        }
    };
}
