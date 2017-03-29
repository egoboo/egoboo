#pragma once

#include "egolib/platform.h"

enum class RefKind
{
    Billboard,       ///< @see BillboardRef
	Particle,        ///< @see ParticleRef
    ParticleProfile, ///< @see ParticleProfileRef
    Enchant,         ///< @see EnchantRef
    EnchantProfile,  ///< @see EnchantProfileRef
    Object,          ///< @see ObjectRef
    ObjectProfile,   ///< @see ObjectProfileRef
    Module,          ///< @see ModuleRef
    Player,          ///< @see PlayerRef
    Texture,         ///< @see TextureRef
};

namespace Internal {

/// @brief A struct derived from @a std::true_type if @a TypeArg fulfils the
/// requirements of the <em>reference value</em> concet and derived from
/// @a std::false_type otherwise.
template <typename RefValueArg>
struct IsRefValue : public std::conditional
    <
        // bool, char, char16_t, char32_t, wchar_t, short, int, long, long long or any implementation-defined
        // extended integer types, including any signed, unsigned, and cv-qualified variants.
        (std::is_integral<RefValueArg>::value &&
        // Without signed variants.
        std::is_unsigned<RefValueArg>::value &&
        // Without cv-qualified variants.
        !std::is_const<RefValueArg>::value),
        std::true_type,
        std::false_type
    >::type
{};

} // namespace Internal

/// @brief Type-safe references. Specialization for all types which fulfil the <c>reference value</c> concept are provied.
template <typename ValueTypeArg, ValueTypeArg MinimumValueArg, ValueTypeArg MaximumValueArg, RefKind KindArg, typename EnabledArg = void>
struct Ref;

template <typename ValueTypeArg, ValueTypeArg MinimumValueArg, ValueTypeArg MaximumValueArg, RefKind KindArg>
struct Ref < ValueTypeArg, MinimumValueArg, MaximumValueArg, KindArg,
             std::enable_if_t<::Internal::IsRefValue<ValueTypeArg>::value> >
           : public id::equal_to_expr<Ref<ValueTypeArg, MinimumValueArg, MaximumValueArg, KindArg>>,
             public id::lower_than_expr<Ref<ValueTypeArg, MinimumValueArg, MaximumValueArg, KindArg>>,
             public id::increment_expr<Ref<ValueTypeArg, MinimumValueArg, MaximumValueArg, KindArg>>,
             public id::decrement_expr<Ref<ValueTypeArg, MinimumValueArg, MaximumValueArg, KindArg>>
{
public:
    using ValueType = ValueTypeArg;
    using MyType = Ref<ValueTypeArg, MinimumValueArg, MaximumValueArg, KindArg>;
    static constexpr ValueType MinimumValue = MinimumValueArg;
    static constexpr ValueType MaximumValue = MaximumValueArg;
    static constexpr ValueType InvalidValue = MaximumValueArg;
    static const MyType Invalid;
    static const MyType Min;
    static const MyType Max;

private:
    ValueType m_value;

public:
    // Instantiation time static assertions.

    // bool, char, char16_t, char32_t, wchar_t, short, int, long, long long
    // or any implementation-defined extended integer types, including any signed, unsigned, and cv-qualified variants.
    static_assert(std::is_integral<ValueType>::value,
                  "the underlaying type must be an unsigned, arithmetic, integral type");
    // Same as above but without signed variants.
    static_assert(std::is_unsigned<ValueType>::value,
                  "the value type must be an unsigned, arithmetic, integral type");
    // Same as above but without cv-qualified variants.
    static_assert(!std::is_const<ValueType>::value,
                  "the value type must be an unsigned, arithmetic, integral type");

    // Instantiation time static assertions.
    static_assert(std::integral_constant<ValueType, MinimumValue>::value <= 
                  std::integral_constant<ValueType, MaximumValue>::value,
                  "the minimum value must be smaller than or equal to the maximum value");
    static_assert(MaximumValue == InvalidValue,
                  "the invalid value must be equal to the maximum value");
public:
    /// @brief Construct an invalid reference.
    Ref() noexcept :
        m_value(InvalidValue)
    {}

    /// @brief Construct a reference with the specified reference value.
    /// @param value the reference value
    /// @throw std::domain_error the reference value is out of bounds
    explicit Ref(const ValueType& value) :
        m_value(value)
    {
        if (m_value < MinimumValue || m_value > MaximumValue)
        {
            throw std::domain_error("out of range");
        }
    }

    Ref(const MyType& other) noexcept :
        m_value(other.m_value)
    {}

public:
    template <typename T>
    explicit Ref(std::enable_if_t<std::is_integral<T>::value &&
                 std::numeric_limits<T>::max() != std::numeric_limits<ValueTypeArg>::max(), T> value) :
        Ref()
    {
        if (value < MinimumValue || value > MaximumValue)
        {
            throw std::domain_error("out of range");
        }
        m_value = static_cast<ValueType>(value);
    }

public:
    // Logical operator overloads.

    bool operator!() const noexcept
    {
        return InvalidValue == m_value;
    }

    // Assignment operator overloads.
    MyType& operator=(const MyType& other) noexcept
    {
        m_value = other.m_value;
        return *this;
    }

#define REF_WITH_IMPLICITASSIGNMENT 1
#if defined(REF_WITH_IMPLICITASSIGNMENT) && 1 == REF_WITH_IMPLICITASSIGNMENT
    MyType& operator=(unsigned int value)
    {
        MyType temporary(value);
        (*this) = temporary;
        return *this;
    }

    MyType& operator=(signed int value)
    {
        MyType temporary(value);
        (*this) = temporary;
        return *this;
    }

    MyType& operator=(unsigned long value)
    {
        MyType temporary(value);
        (*this) = temporary;
        return *this;
    }

    MyType& operator=(signed long value)
    {
        MyType temporary(value);
        (*this) = temporary;
        return *this;
    }
#endif

    // Cast operator overloads.
    explicit operator bool() const noexcept
    {
        return InvalidValue != m_value;
    }

    // Accessors.

    ValueType get() const noexcept
    {
        return m_value;
    }

public:
    // CRTP
    bool equal_to(const MyType& other) const noexcept
    {
        return m_value == other.m_value;
    }

    // CRTP
    bool lower_than(const MyType& other) const noexcept
    {
        return m_value < other.m_value;
    }

    // CRTP
    void increment()
    {
        if (MaximumValue == m_value)
        {
            std::ostringstream msg;
            msg << __FILE__ << ":" << __LINE__ << ": " << "reference overflow";
            std::overflow_error(msg.str());
        }
        ++m_value;
    }

    // CRTP
    void decrement()
    {
        if (MinimumValue == m_value)
        {
            std::ostringstream msg;
            msg << __FILE__ << ":" << __LINE__ << ": " << "reference underflow";
            std::underflow_error(msg.str());
        }
        --m_value;
    }
};

template <typename ValueTypeArg, ValueTypeArg MinimumValueArg, ValueTypeArg MaximumValueArg, RefKind KindArg>
constexpr typename Ref<ValueTypeArg, MinimumValueArg, MaximumValueArg, KindArg, std::enable_if_t<::Internal::IsRefValue<ValueTypeArg>::value>>::ValueType
Ref<ValueTypeArg, MinimumValueArg, MaximumValueArg, KindArg, std::enable_if_t<::Internal::IsRefValue<ValueTypeArg>::value>>::MinimumValue;

template <typename ValueTypeArg, ValueTypeArg MinimumValueArg, ValueTypeArg MaximumValueArg, RefKind KindArg>
constexpr typename Ref<ValueTypeArg, MinimumValueArg, MaximumValueArg, KindArg, std::enable_if_t<::Internal::IsRefValue<ValueTypeArg>::value>>::ValueType
Ref<ValueTypeArg, MinimumValueArg, MaximumValueArg, KindArg, std::enable_if_t<::Internal::IsRefValue<ValueTypeArg>::value>>::MaximumValue;

template <typename ValueTypeArg, ValueTypeArg MinimumValueArg, ValueTypeArg MaximumValueArg, RefKind KindArg>
constexpr typename Ref<ValueTypeArg, MinimumValueArg, MaximumValueArg, KindArg, std::enable_if_t<::Internal::IsRefValue<ValueTypeArg>::value>>::ValueType
Ref<ValueTypeArg, MinimumValueArg, MaximumValueArg, KindArg, std::enable_if_t<::Internal::IsRefValue<ValueTypeArg>::value>>::InvalidValue;

template <typename ValueTypeArg, ValueTypeArg MinimumValueArg, ValueTypeArg MaximumValueArg, RefKind KindArg>
const typename Ref<ValueTypeArg, MinimumValueArg, MaximumValueArg, KindArg, std::enable_if_t<::Internal::IsRefValue<ValueTypeArg>::value>>::MyType
Ref<ValueTypeArg, MinimumValueArg, MaximumValueArg, KindArg, std::enable_if_t<::Internal::IsRefValue<ValueTypeArg>::value>>::Invalid(MaximumValueArg);

template <typename ValueTypeArg, ValueTypeArg MinimumValueArg, ValueTypeArg MaximumValueArg, RefKind KindArg>
const typename Ref<ValueTypeArg, MinimumValueArg, MaximumValueArg, KindArg, std::enable_if_t<::Internal::IsRefValue<ValueTypeArg>::value>>::MyType
Ref<ValueTypeArg, MinimumValueArg, MaximumValueArg, KindArg, std::enable_if_t<::Internal::IsRefValue<ValueTypeArg>::value>>::Min(MinimumValueArg);

template <typename ValueTypeArg, ValueTypeArg MinimumValueArg, ValueTypeArg MaximumValueArg, RefKind KindArg>
const typename Ref<ValueTypeArg, MinimumValueArg, MaximumValueArg, KindArg, std::enable_if_t<::Internal::IsRefValue<ValueTypeArg>::value>>::MyType
Ref<ValueTypeArg, MinimumValueArg, MaximumValueArg, KindArg, std::enable_if_t<::Internal::IsRefValue<ValueTypeArg>::value>>::Max(MaximumValueArg);
