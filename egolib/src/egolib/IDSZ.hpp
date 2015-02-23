#pragma once

#include "egolib/platform.h"

struct IDSZ2
{

private:

	uint32_t _value;

public:

	static const IDSZ2 None;

	IDSZ2(const IDSZ2& idsz):
		_value(idsz._value)
	{
	}

	IDSZ2(char C0, char C1, char C2, char C3) :
		_value(static_cast<uint32_t>
			(
				(((C0-'A') & 0x1F) << 15) |
				(((C1-'A') & 0x1F) << 10) |
				(((C2-'A') & 0x1F) <<  5) |
				(((C3-'A') & 0x1F) <<  0)
			))
	{
		//ctor
	}
	
	IDSZ2& operator=(const IDSZ2& other)
	{
		_value = other._value;
        return *this;
	}
	
	bool operator==(const IDSZ2& other)
	{
		return _value == other._value;
	}
	
	bool operator!=(const IDSZ2& other)
	{
		return _value != other._value;
	}
	
	uint32_t get() const
	{
		return _value;
	}
	
};