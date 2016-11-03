#include "egolib/Mesh/TileFX.hpp"

namespace Ego
{


const TileFX TileFX::None(std::numeric_limits<uint8_t>::min());

const TileFX TileFX::All(std::numeric_limits<uint8_t>::max());

const TileFX TileFX::Water(1);

const TileFX TileFX::Damaging(2);

const TileFX TileFX::Slippery(4);

const TileFX TileFX::Animated(8);

const TileFX TileFX::Reflective(16);

const TileFX TileFX::Reflected(32);

const TileFX TileFX::Wall(64);

const TileFX TileFX::Impassable(128);


TileFX::TileFX(uint8_t fx) : fx(fx) {}

TileFX::TileFX() : fx(0) {}

TileFX::TileFX(TileFX&& rhs) : fx(std::move(rhs.fx)) {}

TileFX::TileFX(const TileFX& rhs) : fx(rhs.fx) {}


TileFX& TileFX::operator=(const TileFX& rhs)
{
	fx = rhs.fx;
	return *this;
}

TileFX& TileFX::operator=(TileFX&& rhs)
{
	fx = rhs.fx;
	return *this;
}


bool TileFX::equalTo(const TileFX& other) const noexcept
{
    return fx == other.fx;
}

TileFX TileFX::operator~() const
{
	return TileFX(~fx);
}

TileFX operator|(TileFX lhs, const TileFX& rhs)
{
	lhs |= rhs;
	return lhs;
}

TileFX operator&(TileFX lhs, const TileFX& rhs)
{
	lhs &= rhs;
	return lhs;
}

TileFX& TileFX::operator&=(const TileFX& rhs)
{
	fx &= rhs.fx;
	return (*this);
}

TileFX& TileFX::operator|=(const TileFX& other)
{
	fx |= other.fx;
	return *this;
}


bool TileFX::isAnimated() const
{
	return Animated == ((*this) & Animated);
}

bool TileFX::isWater() const
{ 
	return Water == ((*this) & Water); 
}

bool TileFX::isSlippery() const
{
	return Slippery == ((*this) & Slippery);  
}

bool TileFX::isReflective() const
{ 
	return Reflective == ((*this) & Reflective); 
}

bool TileFX::isImpassable() const
{ 
	return Impassable == ((*this) & Impassable); 
}
    
bool TileFX::isWall() const
{ 
	return Wall == ((*this) & Wall); 
}
    
bool TileFX::isDamaging() const 
{ 
	return Damaging == ((*this) & Damaging);  
}

bool TileFX::isReflected() const 
{ 
	return Reflected == ((*this) & Reflected);  
}


} // namespace Ego