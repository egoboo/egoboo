#include "egolib/IDSZ.hpp"

const IDSZ2 IDSZ2::None('N','O','N','E');

IDSZ2::IDSZ2() : IDSZ2(IDSZ2::None)
{
	//default ctor
}

IDSZ2::IDSZ2(const IDSZ2& idsz) :
	_value(idsz._value)
{
	//copy ctor
}

IDSZ2::IDSZ2(uint32_t value) :
	_value(value)
{
	//ctor
}


IDSZ2::IDSZ2(char C0, char C1, char C2, char C3) :
	_value(caseLabel(C0, C1, C2, C3))
{
	//ctor	
}

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

uint32_t IDSZ2::toUint32() const {
	return _value;
}

std::string IDSZ2::toString() const {
	if (*this == IDSZ2::None) {
		return "NONE";
	}
	else {
		std::array<char, 4> result;
		result[0] = ((_value >> 15) & 0x1F) + 'A';
		result[1] = ((_value >> 10) & 0x1F) + 'A';
		result[2] = ((_value >> 5) & 0x1F) + 'A';
		result[3] = ((_value >> 0) & 0x1F) + 'A';
		return std::string(result.begin(), result.end());
	}
}