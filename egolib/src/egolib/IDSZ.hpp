#pragma once

#include "egolib/platform.h"

class IDSZ2 {
public:
	static const IDSZ2 None;

    IDSZ2();

	IDSZ2(const IDSZ2& idsz);

	IDSZ2(char C0, char C1, char C2, char C3);

    IDSZ2(uint32_t value);
	
	IDSZ2& operator=(const IDSZ2& other);
	
	bool operator==(const IDSZ2& other) const;
	
	bool operator!=(const IDSZ2& other) const;
	
	uint32_t toUint32() const;

	std::string toString() const;

    inline bool equals(char C0, char C1, char C2, char C3) const {
        return caseLabel(C0, C1, C2, C3) == _value;
    }

    static constexpr uint32_t caseLabel(char C0, char C1, char C2, char C3)
    {
        return static_cast<uint32_t>(
            (((C0 - 'A') & 0x1F) << 15) |
            (((C1 - 'A') & 0x1F) << 10) |
            (((C2 - 'A') & 0x1F) << 5) |
            (((C3 - 'A') & 0x1F) << 0)
        );
    }

private:
    uint32_t _value;
};

/**
* @brief
*   Define hash function for the IDSZ2 class so that it can be used
*   in std. Do not use operator() since that would lead to implicit
*   conversion to uint32_t and could lead to various bugs that are
*   hard to find.
**/
namespace std {

  template <>
  struct hash<IDSZ2>
  {
    std::size_t operator()(const IDSZ2& k) const
    {
        return k.toUint32();
    }
  };

}