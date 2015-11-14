#include "egolib/IDSZ.hpp"

const char *undo_idsz(IDSZ idsz)
{
	static char value_string[5] = { "NONE" };

	if (idsz == IDSZ_NONE)
	{
		strncpy(value_string, "NONE", SDL_arraysize(value_string));
	}
	else
	{
		// Bad! both function return and return to global variable!
		value_string[0] = ((idsz >> 15) & 0x1F) + 'A';
		value_string[1] = ((idsz >> 10) & 0x1F) + 'A';
		value_string[2] = ((idsz >> 5) & 0x1F) + 'A';
		value_string[3] = ((idsz)& 0x1F) + 'A';
		value_string[4] = 0;
	}

	return value_string;
}

const IDSZ2 IDSZ2::None('N','O','N','E');

IDSZ2::IDSZ2(const IDSZ2& idsz) :
	_value(idsz._value)
{}

IDSZ2::IDSZ2(char C0, char C1, char C2, char C3) :
	_value(static_cast<uint32_t>
		(
			(((C0 - 'A') & 0x1F) << 15) |
			(((C1 - 'A') & 0x1F) << 10) |
			(((C2 - 'A') & 0x1F) << 5) |
			(((C3 - 'A') & 0x1F) << 0)
			))
{}

IDSZ2& IDSZ2::operator=(const IDSZ2& other) {
	_value = other._value;
	return *this;
}

bool IDSZ2::operator==(const IDSZ2& other) const {
	return _value == other._value;
}

bool IDSZ2::operator!=(const IDSZ2& other) const {
	return _value != other._value;
}

uint32_t IDSZ2::get() const {
	return _value;
}

std::string IDSZ2::toString() const {
	if (*this == IDSZ2::None) {
		return "NONE";
	}
	else {
		char string[] = { char(((_value >> 15) & 0x1F) + 'A'),
			char(((_value >> 10) & 0x1F) + 'A'),
			char(((_value >> 5) & 0x1F) + 'A'),
			char(((_value >> 0) & 0x1F) + 'A'),
			'\0' };
		return string;
	}
}