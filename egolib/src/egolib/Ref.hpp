#pragma once

enum class RefKind
{
    Particle, /// @todo Formerly known as PRT_REF.
    ParticleProfile, /// @todo Formerly known as PIP_REF.
    Enchant, /// @todo Formerly known as ENC_REF.
    EnchantProfile, // @todo Formerly known as EVE_REF.
    Player, /// @todo Formerly known as PLA_REF.
    Object, /// @todo Formerly known as CHR_REF.
    Module, /// @todo Formerly knownas MOD_REF.
    ObjectProfile, /// @todo Formerly known as PRO_REF.
};

/// @todo Assert that MIN <= INVALID <= MAX upon template instantiation. 
template <typename TYPE, TYPE MIN, TYPE MAX, TYPE INVALID, RefKind KIND>
struct Ref
{

private:

    size_t _ref;

public:


    // Instantiation time static assertions.

    static_assert(std::is_integral<TYPE>::value, "TYPE must be an integral type");

    static_assert(std::integral_constant<uint16_t, MIN>::value <= 
                  std::integral_constant<uint16_t, MAX>::value,
                  "minimum ref must be smaller than or equal to maximum ref");

    static_assert(std::integral_constant<uint16_t, INVALID>::value >=
                  std::integral_constant<uint16_t, MIN>::value,
                  "invalid ref must be greater than or equal to minimum ref");

    static_assert(std::integral_constant<uint16_t, INVALID>::value <=
                  std::integral_constant<uint16_t, MAX>::value,
                  "invalid ref must be smaller than or equal to maximum ref");
    

    // Static variables.

    static const Ref<TYPE, MIN, MAX, INVALID, KIND> Invalid;


    // Constructs.

    Ref() :
		_ref(INVALID)
	{
	}

    Ref(const TYPE ref) :
        _ref(ref)
    {
        if (ref < MIN || ref > MAX)
        {
            throw std::domain_error("out of range");
        }
    }

    Ref(const Ref<TYPE, MIN, MAX, INVALID, KIND>& other) :
        _ref(other._ref)
    {
    }


    // Logical operator overloads.
		
	bool operator!() const
	{
		return _ref == INVALID;
	}


	// Relational operator overloads.
	
    bool operator>=(const Ref<TYPE, MIN, MAX, INVALID, KIND>& other) const
    {
        return _ref >= other._ref;
    }

    bool operator<=(const Ref<TYPE, MIN, MAX, INVALID, KIND>& other) const
    {
        return _ref <= other._ref;
    }

    bool operator<(const Ref<TYPE, MIN, MAX, INVALID, KIND>& other) const
    {
        return _ref < other._ref;
    }

    bool operator>(const Ref<TYPE, MIN, MAX, INVALID, KIND>& other) const
    {
        return _ref > other._ref;
    }

    bool operator==(const Ref<TYPE, MIN, MAX, INVALID, KIND>& other) const
    {
        return _ref == other._ref;
    }

    bool operator!=(const Ref<TYPE, MIN, MAX, INVALID, KIND>& other) const
    {
        return _ref != other._ref;
    }
	

	// Assignment operator overloads.

    Ref<TYPE, MIN, MAX, INVALID, KIND>& operator=(const Ref<TYPE, MIN, MAX, INVALID, KIND>& other)
    {
        _ref(other._ref);
        return *this;
    }

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


    // Cast operator overloads.

	operator TYPE() const
	{
		return _ref;
	}


    // Accessors.

    TYPE get() const
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
