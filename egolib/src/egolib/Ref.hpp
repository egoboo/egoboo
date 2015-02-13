#pragma once

/// @todo Assert that MIN <= INVALID <= MAX upon template instantiation. 
template <size_t MIN,size_t MAX,size_t INVALID>
struct Ref
{
private:
    size_t _ref;
public:

	Ref() :
		_ref(INVALID)
	{
	}

    Ref(const Ref<MIN, MAX, INVALID>& other) :
        _ref(other._ref)
    {
    }
		
	bool operator!() const
	{
		return _ref == INVALID;
	}

	// Relational operator overloads.
	
    bool operator>=(const Ref<MIN, MAX, INVALID>& other) const
    {
        return _ref >= other._ref;
    }

    bool operator<=(const Ref<MIN, MAX, INVALID>& other) const
    {
        return _ref <= other._ref;
    }

    bool operator<(const Ref<MIN, MAX, INVALID>& other) const
    {
        return _ref < other._ref;
    }

    bool operator>(const Ref<MIN, MAX, INVALID>& other) const
    {
        return _ref > other._ref;
    }

    bool operator==(const Ref<MIN, MAX, INVALID>& other) const
    {
        return _ref == other._ref;
    }

    bool operator!=(const Ref<MIN, MAX, INVALID>& other) const
    {
        return _ref != other._ref;
    }
	
	// Assignment operator overloads.

    Ref<MIN, MAX, INVALID>& operator=(const Ref<MIN, MAX, INVALID>& other)
    {
        _ref(other._ref);
        return *this;
    }

    Ref<MIN, MAX, INVALID>& operator=(const unsigned int& other)
    {
        if (other < MIN || other > MAX)
        {
            throw std::domain_error("out of range");
        }
        _ref = other;
        return *this;
    }

    Ref<MIN, MAX, INVALID>& operator=(const int& other)
    {
        if (other < MIN || other > MAX)
        {
            throw std::domain_error("out of range");
        }
        _ref = other;
        return *this;
    }

	Ref<MIN, MAX, INVALID>& operator=(const size_t& other)
	{
		if (other < MIN || other > MAX)
		{
			throw std::domain_error("out of range");
		}
		_ref = other;
        return *this;
	}
	
	// Cast operator overloads.
	
    operator unsigned int() const
    {
        if (_ref > std::numeric_limits<unsigned int>::max())
        {
            throw std::bad_cast();
        }
    }

    operator int() const
    {
        if (_ref > std::numeric_limits<int>::max())
        {
            throw std::bad_cast();
        }
    }

	operator size_t() const
	{
		return _ref;
	}
	
};
