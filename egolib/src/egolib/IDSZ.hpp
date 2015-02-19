#pragma once

#include "egolib/platform.h"

struct IDSZ2
{

private:

	Uint32 _value;

public:

	static const IDSZ2 None;

	IDSZ2(const IDSZ2& idsz):
		_value(idsz._value)
	{
	}

	IDSZ2(char C0, char C1, char C2, char C3)
	{
		_value = static_cast<Uint32>
			(
				(((C0-'A') & 0x1F) << 15) |
				(((C1-'A') & 0x1F) << 10) |
				(((C2-'A') & 0x1F) <<  5) |
				(((C3-'A') & 0x1F) <<  0)
			);
	}
	
	IDSZ2& operator=(const IDSZ2& other)
	{
		_value = other._value;
	}
	
	bool operator==(const IDSZ2& other)
	{
		return _value == other._value;
	}
	
	bool operator!=(const IDSZ2& other)
	{
		return _value != other._value;
	}
	
	Uint32 get() const
	{
		return _value;
	}
	
};