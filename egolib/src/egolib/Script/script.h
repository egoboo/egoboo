//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file egolib/Script/script.h

#pragma once

#include "egolib/typedef.h"
#include "egolib/Core/Singleton.hpp"
#include "egolib/Logic/Damage.hpp"
#include "egolib/IDSZ.hpp"
#include "egolib/Clock.hpp"
#include "egolib/AI/WaypointList.h"
#include "egolib/_math.h"
#include "egolib/Script/ConstantPool.hpp"
#include "egolib/Script/Interpreter/TaggedValue.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

class Object;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MSGDISTANCE         2000                    ///< Range for SendMessageNear
#define PITNOSOUND          -256                    ///< Stop sound at bottom of pits...

/// These are for FindPath function only
#define   MOVE_MELEE  300
#define   MOVE_RANGED  600
#define   MOVE_DISTANCE 175
#define   MOVE_RETREAT  900
#define   MOVE_CHARGE  111
#define   MOVE_FOLLOW  0

/// Camera bounds/edge of the map
#define EDGE                128

/// AI targeting
#define NEARBY      3*Info<float>::Grid::Size()    ///< 3 tiles away
#define WIDE        6*Info<float>::Grid::Size()    ///< 6 tiles away
#define NEAREST     0                              ///< unlimited range

//Max size of an compiled AI script
#define MAXAICOMPILESIZE    2048

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Character AI alerts
enum chr_alert_bits
{
    ALERT_NONE                           = 0,
    ALERTIF_SPAWNED                      = 1 <<  0,
    ALERTIF_HITVULNERABLE                = 1 <<  1,
    ALERTIF_ATWAYPOINT                   = 1 <<  2,
    ALERTIF_ATLASTWAYPOINT               = 1 <<  3,
    ALERTIF_ATTACKED                     = 1 <<  4,
    ALERTIF_BUMPED                       = 1 <<  5,
    ALERTIF_ORDERED                      = 1 <<  6,
    ALERTIF_CALLEDFORHELP                = 1 <<  7,
    ALERTIF_KILLED                       = 1 <<  8,
    ALERTIF_TARGETKILLED                 = 1 <<  9,
    ALERTIF_DROPPED                      = 1 << 10,
    ALERTIF_GRABBED                      = 1 << 11,
    ALERTIF_REAFFIRMED                   = 1 << 12,
    ALERTIF_LEADERKILLED                 = 1 << 13,
    ALERTIF_USED                         = 1 << 14,
    ALERTIF_CLEANEDUP                    = 1 << 15,
    ALERTIF_SCOREDAHIT                   = 1 << 16,
    ALERTIF_HEALED                       = 1 << 17,
    ALERTIF_DISAFFIRMED                  = 1 << 18,
    ALERTIF_CHANGED                      = 1 << 19,
    ALERTIF_INWATER                      = 1 << 20,
    ALERTIF_BORED                        = 1 << 21,
    ALERTIF_TOOMUCHBAGGAGE               = 1 << 22,
    ALERTIF_LEVELUP                      = 1 << 23,
    ALERTIF_CONFUSED                     = 1 << 24,
    ALERTIF_HITGROUND                    = 1 << 25,
    ALERTIF_NOTDROPPED                   = 1 << 26,
    ALERTIF_BLOCKED                      = 1 << 27,
    ALERTIF_THROWN                       = 1 << 28,
    ALERTIF_CRUSHED                      = 1 << 29,
    ALERTIF_NOTPUTAWAY                   = 1 << 30,
    ALERTIF_TAKENOUT                     = 1 << 31,

    // add in some aliases
    ALERTIF_PUTAWAY     = ALERTIF_ATLASTWAYPOINT,
    ALERTIF_NOTTAKENOUT = ALERTIF_NOTPUTAWAY
};

// swig chokes on the definition below
#define STOR_BITS            4
#define STOR_COUNT          (1 << STOR_BITS)        ///< Storage data (Used in SetXY)
#define STOR_AND            (STOR_COUNT - 1)        ///< Storage data bitmask

//--------------------------------------------------------------------------------------------
// struct script_info_t
//--------------------------------------------------------------------------------------------

struct Instruction
{
public:
    // 1000 0000.0000 0000.0000 0000.0000 0000
    static const uint32_t FUNCTIONBITS = 0x80000000;
    // 0000 0111 1111 1111 1111 1111 1111 1111
    static const uint32_t VALUEBITS = 0x07FFFFFF;
    // 0111 1000 0000 0000 0000 0000 0000 0000
    // for function/assignment, it's the indentation of the line i.e. there are 2^4 = 16 possible indention levels.
    // for operands, it's the operator
    static const uint32_t DATABITS = 0x78000000;

private:
    uint32_t bits;

public:
    Instruction()
        : bits()
    {}

    Instruction(const Instruction& other)
        : bits(other.bits)
    {}

    Instruction(Instruction&& other)
        : bits(std::move(other.bits))
    {}

    Instruction& operator=(Instruction other)
    {
        swap(*this, other);
        return *this;
    }

    friend void swap(Instruction& x, Instruction& y)
    {
        using std::swap;

        swap(x.bits, y.bits);
    }

public:
    Instruction(uint32_t bits)
        : bits(bits)
    {}

    uint32_t operator&(uint32_t bitmask)
    {
        return bits & bitmask;
    }

    size_t getIndex() const
    {
        static_assert(std::numeric_limits<size_t>::max() >= std::numeric_limits<uint32_t>::max(),
                      "maximum value of size_t is smaller than maximum value of uint32_t");
        return (size_t)bits;
    }

    /// @brief Get if this instruction is an "inv" (~"invoke") instruction.
    /// @return @a true if this instruction is an "inv" instruction, @a false otherwise
    /// @todo
    /// EgoScript has some decision logic built-in if an instruction is an
    /// "inv" or "ldc" instruction. Clean up this mess.
    bool isInv() const
    {
        return hasSomeBits(FUNCTIONBITS);
    }

    /// @brief Get if this instruction is a "ldc" (~"load constant") instruction.
    /// @return @a true if this instruction is a "ldc" instruction, @a false otherwise
    /// @todo
    /// EgoScript has some decision logic built-in if an instruction is an
    /// "inv" or "ldc" instruction. Clean up this mess.
    bool isLdc() const
    {
        return hasSomeBits(FUNCTIONBITS);
    }

    uint32_t getBits() const
    {
        return bits;
    }

    void setBits(uint32_t bits)
    {
        this->bits = bits;
    }

    /// @brief Get the data bits.
    /// @return the data bits
    /// @remark The data bits are the upper 5 Bits of the 32 value bits
    uint8_t getDataBits() const
    {
        return (getBits() >> 27) & 0x0f;
    }

    /// @brief Get the value bits.
    /// @return the value bits
    uint32_t getValueBits() const
    {
        return getBits() & Instruction::VALUEBITS;
    }

    /// @brief Get if this instruction has none of the bits in the specified bitmask set.
    /// @param bitmask the bitmask
    /// @return @a true if this instruction has none of the bits in the bitmask set, @a false otherwise
    bool hasNoBits(uint32_t bitmask) const
    {
        return 0 == (getBits() & bitmask);
    }

    /// @brief Get if this instruction has some of the bits set in the specified bitmask set.
    /// @param bitmask the bitmask
    /// @return @a true if this instruction has any of the bits in the bitmask set, @a false otherwise
    bool hasSomeBits(uint32_t bitmask) const
    {
        return 0 != (getBits() & bitmask);
    }

    /// @brief Get if this instruction has all of the bits in the specified bitmask set.
    /// @param bitmask the bitmask
    /// @return @a true if this instruction has all of the bits in the bitmask set, @a false otherwise
    bool hasAllBitsSet(uint32_t bitmask) const
    {
        return bitmask == (getBits() & bitmask);
    }
};

struct InstructionList
{
public:
    /// @brief A size of an instruction list.
    using Size = uint32_t;
    /// @brief An index in an instruction list.
    using Index = uint32_t;

private:
    /// @brief The number of instructions in this instruction list.
    /// @remark The first @a numberOfInstructions entries in the instruction array are used.
    Size numberOfInstructions;

    /// @brief The instructions.
    std::array<Instruction, MAXAICOMPILESIZE> instructions;

    /// @brief The constant pool.
    Ego::Script::ConstantPool constantPool;

public:
    /// @brief Construct an empty instruction list.
    /// @post The instruction list has an empty constant pool and zero instructions.
    InstructionList()
        : constantPool(), instructions(), numberOfInstructions(0)
    {}

    /**@{*/

    /// @brief Construct an instruction list with the values of another instruction list.
    /// @param other the other instruction list
    InstructionList(const InstructionList& other)
        : constantPool(other.constantPool), instructions(other.instructions), numberOfInstructions(other.numberOfInstructions)
    {}

    InstructionList(InstructionList&& other)
        : constantPool(std::move(other.constantPool)), instructions(std::move(other.instructions)), numberOfInstructions(std::move(other.numberOfInstructions))
    {}

    /**@}*/

    /// @brief Assign this instruction list with the values of another instruction list.
    /// @param other the other instruction list
    /// @return this instruction list
    InstructionList& operator=(InstructionList other)
    {
        swap(*this, other);
        return *this;
    }

    friend void swap(InstructionList& x, InstructionList& y)
    {
        using std::swap;

        swap(x.instructions, y.instructions);
        swap(x.constantPool, y.constantPool);
        swap(x.numberOfInstructions, y.numberOfInstructions);
    }
    
    /// @brief Get the number of instructions in this instruction list.
    /// @return the number of instructions in this instruction list
    Size getNumberOfInstructions() const
    {
        return numberOfInstructions;
    }

    /// @brief Get if this instruction list is full.
    /// @return @a true if this instruction list is full, @a false otherwise
    bool isFull() const
    {
        return MAXAICOMPILESIZE == getNumberOfInstructions();
    }

    /// @brief Get if this instruction list is empty.
    /// @return @a true if this instruction list is empty, @a false otherwise
    bool isEmpty() const
    {
        return 0 == getNumberOfInstructions();
    }

    void append(const Instruction& instruction)
    {
        if (isFull())
        {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "instruction list overflow");
        }
        instructions[numberOfInstructions++] = instruction;
    }

    /// @brief Get the instruction at the specified index.
    /// @param index the index
    /// @return a constant reference to the instruction
    /// @throw Id::RuntimeErrorException @a index is out of bounds
	const Instruction& operator[](Index index) const 
    {
        if (index >= getNumberOfInstructions())
        {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "instruction index out of bounds");
        }
		return instructions[index];
	}

    /// @brief Get the instruction at the specified index.
    /// @param index the index
    /// @return a reference to the instruction
    /// @throw Id::RuntimeErrorException @a index is out of bounds
    Instruction& operator[](Index index)
    {
        if (index >= getNumberOfInstructions())
        {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "instruction index out of bounds");
        }
		return instructions[index];
	}

    const Ego::Script::ConstantPool& getConstantPool() const
    {
        return constantPool;
    }

    Ego::Script::ConstantPool& getConstantPool()
    {
        return constantPool;
    }

    void clear()
    {
        constantPool.clear();
        numberOfInstructions = 0;
    }
};

struct script_info_t
{
public:
    script_info_t() :
        _name(),
        indent(0),
        indent_last(0),
        _position(0),
        _instructions()
    {
        //ctor
    }

public:
	/**
	 * @brief
	 *	The name of the script file.
	 */
	std::string _name;

	/**
	 * @brief
	 *	Get the name of this script file.
	 * @return
	 *	the name of this script file
	 */
	const std::string& getName() const {
		return _name;
	}

    uint32_t        indent;
    uint32_t        indent_last;


	/**
	 * @brief
	 *	The instruction index.
	 */
    size_t _position;

	/**
	 * @brief
	 *	The instruction list.
	 */
	InstructionList _instructions;

	bool increment_pos();
	size_t get_pos() const;
	bool set_pos(size_t position);

};

//--------------------------------------------------------------------------------------------
// struct ai_state_t
//--------------------------------------------------------------------------------------------

#include "egolib/AI/State.hpp"

/// The state of an A.I. controlling a single object.
struct ai_state_t : public AI::State<ObjectRef>
{
public:

    // some script states
    Sint32         poof_time;
    bool           changed;
    bool           terminate;

    // who are we related to?
    ObjectRef      owner;         ///< The character's owner
    ObjectRef      child;         ///< The character's child

    // some local storage
    BIT_FIELD      alert;         ///< Alerts for AI script
    int            state;         ///< Short term memory for AI
    int            content;       ///< More short term memory
    int            passage;       ///< The passage associated with this character
    Uint32         timer;         ///< AI Timer
    int            x[STOR_COUNT]; ///< Temporary values...  SetXY
    int            y[STOR_COUNT];
    float          maxSpeed;      ///< Artificial movement speed limit for AI

    // ai memory from the last event
    int            bumplast_time;   ///< The last time that a ALERTIF_BUMPED was sent

    ObjectRef      hitlast;         ///< Last character it hit
    Facing         directionlast;   ///< Direction of last attack/healing
    DamageType     damagetypelast;  ///< Last damage type
    ObjectRef      lastitemused;    ///< The last item the character used

    // message handling
    Uint32         order_value;           ///< The last order given the character
    Uint16         order_counter;         ///< The rank of the character on the order chain

    // waypoints
    bool            wp_valid;            ///< is the current waypoint valid?
    waypoint_t      wp;                  ///< current waypoint
    waypoint_list_t wp_lst;              ///< Stored waypoints
    Uint32          astar_timer;         ///< Throttle on astar pathfinding

    // performance monitoring
	std::shared_ptr<Ego::Time::Clock<Ego::Time::ClockPolicy::NonRecursive>> _clock;

	ai_state_t();
	~ai_state_t();

	static void reset(ai_state_t& self);
	static bool set_bumplast(ai_state_t& self, const ObjectRef  ichr);
	static bool get_wp(ai_state_t& self);
	static bool ensure_wp(ai_state_t& self);
	static bool add_order(ai_state_t& self, Uint32 value, Uint16 counter);
	static bool set_changed(ai_state_t& self);
	static void spawn(ai_state_t& self, const ObjectRef index, const PRO_REF iobj, Uint16 rank);

};

//--------------------------------------------------------------------------------------------
// struct script_state_t
//--------------------------------------------------------------------------------------------

/// The state of the scripting system
/// @details It is not persistent between one evaluation of a script and another
struct script_state_t : Id::NonCopyable
{
    int x;
    int y;
    int turn;
    int distance;
    int argument;
    using TaggedValue = Ego::Script::Interpreter::TaggedValue;
    TaggedValue operationsum; /// The result of an arithmetic operation

	// public
	script_state_t();

    /// @brief Error handler for the error "variable not defined".
    /// Writes a warning log messages and raises an Id::RuntimeErrorException.
    /// @param variableIndex the variable index
    /// @throw Id::RuntimeErrorException
    void onVariableNotDefinedError(uint8_t variableIndex);
	// protected
	uint8_t run_function(ai_state_t& aiState, script_info_t& script);
    int32_t loadVariable(uint8_t variableIndex, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);
	void storeVariable(uint8_t variableIndex);
	static void run_operand(script_state_t& self, ai_state_t& aiState, script_info_t& script);
	static bool run_operation(script_state_t& self, ai_state_t& aiState, script_info_t& script);
	static bool run_function_call(script_state_t& self, ai_state_t& aiState, script_info_t& script);
};

//--------------------------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

void scr_run_chr_script(Object *pchr);
void scr_run_chr_script(const ObjectRef character);

void issue_order( const ObjectRef character, Uint32 order );
void issue_special_order( uint32_t order, const IDSZ2& idsz );
void set_alerts( const ObjectRef character );

namespace Ego {
namespace Script {

// Forward declaration.
template <typename FunctionType>
struct IRuntimeStatistics;

namespace NativeInterface {
	/**
	 * @brief
	 *  The type of a C/C++ native interface (NI) function.
	 */
	using Function = uint8_t(script_state_t&, ai_state_t&);
	/**
	 * @brief
	 *  Combination of a pointer to a C/C++ NI function with its name in the DSL.
	 */
	struct FunctionInfo {
		/// The name of the function in the DSL.
		std::string _name;
		/// A pointer to the C/C++ NI function.
		Function *_pointer;
	};
} // namespace NativeInterface

/// @brief A list of all possible EgoScript functions.
enum ScriptFunctions {
#define Define(name) name,
#define DefineAlias(alias, name) alias = name,
#include "egolib/Script/Functions.in"
#undef DefineAlias
#undef Define
    SCRIPT_FUNCTIONS_COUNT
};

extern std::array<std::string, ScriptFunctions::SCRIPT_FUNCTIONS_COUNT> _scriptFunctionNames;

/// @brief A list of all possible EgoScript variables.
enum ScriptVariables {
#define Define(cName, eName) cName,
#define DefineAlias(cName, eName)
#include "egolib/Script/Variables.in"
#undef DefineAlias
#undef Define
    SCRIPT_VARIABLES_COUNT
};

extern std::array<std::string, ScriptVariables::SCRIPT_VARIABLES_COUNT> _scriptVariableNames;

/// @brief A list of all possible EgoScript operators.
enum ScriptOperators {
#define Define(name) name,
#define DefineAlias(alias, name) alias = name,
#include "egolib/Script/Operators.in"
#undef DefineAlias
#undef Define
    SCRIPT_OPERATORS_COUNT
};

extern std::array<std::string, ScriptOperators::SCRIPT_OPERATORS_COUNT> _scriptOperatorNames;

/// @brief The runtime (environment) for the scripts.
struct Runtime : public Core::Singleton<Runtime> {
protected:
    friend Core::Singleton<Runtime>::CreateFunctorType;
    friend Core::Singleton<Runtime>::DestroyFunctorType;

    /// @brief Construct this runtime.
	/// @remarks Intentionally protected.
	Runtime();

    /// @brief Destruct this runtime.
	/// @remarks Intentionally protected.
	~Runtime();

public:
	/// @brief A map from function value codes to function pointers.
	std::unordered_map<uint32_t, NativeInterface::Function*> _functionValueCodeToFunctionPointer;

private:
    /// @brief A clock to measure the time from the beginning to the end of an action performed by the runtime.
    /// @remark Its window size is 1 as the duration spend in the invocation is added to an histogram.
    std::unique_ptr<Ego::Time::Clock<Ego::Time::ClockPolicy::NonRecursive>> _clock;

    /// @brief Runtime statistics (of this runtime).
    std::unique_ptr<IRuntimeStatistics<uint32_t>> _statistics;

public:
    /// @brief Get the clock.
    /// @return the clock
    /// @remark Used to measure the time from the beginning to the end of an action performed by the runtime.
    Ego::Time::Clock<Ego::Time::ClockPolicy::NonRecursive> &getClock() { return *_clock; }

    /// @brief Get the statistics.
    /// @return the statistics
    IRuntimeStatistics<uint32_t>& getStatistics() { return *_statistics; }
};

} // namespace Script
} // namespace Ego

void scripting_system_begin();
void scripting_system_end();
