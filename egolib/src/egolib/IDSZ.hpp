#pragma once

#include "egolib/platform.h"

// IDSZ
typedef uint32_t IDSZ;

#if !defined(MAKE_IDSZ)
#define MAKE_IDSZ(C0,C1,C2,C3)                 \
    ((IDSZ)(                                   \
             ((((C0)-'A')&0x1F) << 15) |       \
             ((((C1)-'A')&0x1F) << 10) |       \
             ((((C2)-'A')&0x1F) <<  5) |       \
             ((((C3)-'A')&0x1F) <<  0)         \
           ))
#endif

#define IDSZ_NONE MAKE_IDSZ('N','O','N','E')       ///< [NONE]
/**
 * @brief
 *	Convert an integer IDSZ to a text IDSZ.
 * @param idsz
 *	the integer IDSZ
 * @return
 *	a pointer to a text IDSZ
 * @todo
 *	This currently uses a static bufer. Change this.
 */
const char *undo_idsz(IDSZ idsz);

struct IDSZ2 {

private:

	uint32_t _value;

public:

	static const IDSZ2 None;

	IDSZ2(const IDSZ2& idsz);

	IDSZ2(char C0, char C1, char C2, char C3);
	
	IDSZ2& operator=(const IDSZ2& other);
	
	bool operator==(const IDSZ2& other) const;
	
	bool operator!=(const IDSZ2& other) const;
	
	uint32_t get() const;

	std::string toString() const;
	
};