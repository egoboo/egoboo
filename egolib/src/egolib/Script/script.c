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

/// @file  egolib/Script/script.c
/// @brief Implements the game's scripting language.
/// @details

#include "egolib/Script/script.h"
#include "game/script_compile.h"
#include "game/script_implementation.h"
#include "game/script_functions.h"
#include "egolib/AI/AStar.hpp"
#include "game/game.h"
#include "game/Entities/_Include.hpp"
#include "game/Core/GameEngine.hpp"
#include "game/Graphics/CameraSystem.hpp"
#include "game/Module/Module.hpp"
#include "egolib/Script/IRuntimeStatistics.hpp"

namespace Ego {
namespace Script {

/// @brief An implementation of runtime statistics.
struct RuntimeStatistics : IRuntimeStatistics<uint32_t> { 
public:
    void append(const std::string& pathname) {
        auto target = std::shared_ptr<vfs_FILE>(vfs_openAppend(pathname),
                                                [](vfs_FILE *file) { if (nullptr != file) { vfs_close(file); } });
        if (nullptr != target) {
            for (const auto& functionStatistic : _functionStatistics) {
                vfs_printf(target.get(), "function = %" PRIu32 "\t function name = \"%s\"\tnumber of calls = %ld\ttotalTime = %lf\tmaxTime = %lf\n",
                           functionStatistic.first, _scriptFunctionNames[functionStatistic.first].c_str(), functionStatistic.second.numberOfCalls,
                           functionStatistic.second.totalTime, functionStatistic.second.maxTime);

            }
        }
    }
};

std::array<std::string, ScriptVariables::SCRIPT_VARIABLES_COUNT> _scriptVariableNames = {
#define Define(cName, eName) #cName,
#define DefineAlias(cName, eName)
#include "egolib/Script/Variables.in"
#undef DefineAlias
#undef Define
};

std::array<std::string, ScriptFunctions::SCRIPT_FUNCTIONS_COUNT> _scriptFunctionNames = {
#define Define(name) #name,
#define DefineAlias(alias, name)
#include "egolib/Script/Functions.in"
#undef DefineAlias
#undef Define
};

std::array<std::string, ScriptOperators::SCRIPT_OPERATORS_COUNT> _scriptOperatorNames = {
#define Define(name) #name,
#define DefineAlias(alias, name)
#include "egolib/Script/Operators.in"
#undef DefineAlias
#undef Define
};

Runtime::Runtime()
    :_functionValueCodeToFunctionPointer{
        #define Define(name) { name, &scr_##name },
        #define DefineAlias(alias, name) { alias, &scr_##name },     
        #include "egolib/Script/Functions.in"
        #undef DefineAlias
        #undef Define
    },
    _statistics(std::make_unique<RuntimeStatistics>()),
    _clock(std::make_unique<Ego::Time::Clock<Ego::Time::ClockPolicy::NonRecursive>>("runtime clock", 1))
    {
    /* Intentionally empty. */ 
}

Runtime::~Runtime() {
    /* Intentionally empty. */
}

} // namespace Script
} // namespace Ego

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static PRO_REF script_error_model = INVALID_PRO_REF;
static const char * script_error_classname = "UNKNOWN";

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void scripting_system_begin()
{
	if (!Runtime::isInitialized()) {
		Runtime::initialize();
	}
}

void scripting_system_end()
{
    if (Runtime::isInitialized()) {
        Runtime::get().getStatistics().append("/debug/script_function_timing.txt");
		Runtime::uninitialize();
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void scr_run_chr_script(Object *pchr) {

	// Make sure that this module is initialized.
	scripting_system_begin();

	// Do not run scripts of terminated entities.
	if (pchr->isTerminated()) {
		return;
	}
	ai_state_t& aiState = pchr->ai;
	script_info_t& script = pchr->getProfile()->getAIScript();

	// Has the time for this character to die come and gone?
	if (aiState.poof_time >= 0 && aiState.poof_time <= (Sint32)update_wld) {
		return;
	}

	// Grab the "changed" value from the last time the script was run.
	if (aiState.changed) {
		SET_BIT(aiState.alert, ALERTIF_CHANGED);
		aiState.changed = false;
	}

	Ego::Time::ClockScope<Ego::Time::ClockPolicy::NonRecursive> scope(*aiState._clock);

	// debug a certain script
	// debug_scripts = ( 385 == pself->index && 76 == pchr->profile_ref );

	// target_old is set to the target every time the script is run
	aiState.setOldTarget(aiState.getTarget());

	// Make life easier
	script_error_classname = "UNKNOWN";
	script_error_model = pchr->getProfileID();
	if (script_error_model < INVALID_PRO_REF)
	{
		script_error_classname = ProfileSystem::get().getProfile(script_error_model)->getClassName().c_str();
	}

	if (debug_scripts && debug_script_file) {
		vfs_FILE * scr_file = debug_script_file;

		vfs_printf(scr_file, "\n\n--------\n%s\n", script._name.c_str());
		vfs_printf(scr_file, "%d - %s\n", REF_TO_INT(script_error_model), script_error_classname);

		// who are we related to?
		vfs_printf(scr_file, "\tself   == %" PRIuZ "\n", aiState.getSelf().get());
		vfs_printf(scr_file, "\ttarget == %" PRIuZ "\n", aiState.getTarget().get());
		vfs_printf(scr_file, "\towner  == %" PRIuZ "\n", aiState.owner.get());
		vfs_printf(scr_file, "\tchild  == %" PRIuZ "\n", aiState.child.get());

		// some local storage
		vfs_printf(scr_file, "\talert     == %x\n", aiState.alert);
		vfs_printf(scr_file, "\tstate     == %d\n", aiState.state);
		vfs_printf(scr_file, "\tcontent   == %d\n", aiState.content);
		vfs_printf(scr_file, "\ttimer     == %d\n", aiState.timer);
		vfs_printf(scr_file, "\tupdate_wld == %d\n", update_wld);

		// ai memory from the last event
		vfs_printf(scr_file, "\tdirectionlast  == %d\n", uint16_t(aiState.directionlast));
		vfs_printf(scr_file, "\tbumped         == %" PRIuZ "\n", aiState.getBumped().get());
		vfs_printf(scr_file, "\tlast attacker  == %" PRIuZ "\n", aiState.getLastAttacker().get());
		vfs_printf(scr_file, "\thitlast        == %" PRIuZ "\n", aiState.hitlast.get());
		vfs_printf(scr_file, "\tdamagetypelast == %d\n", aiState.damagetypelast);
		vfs_printf(scr_file, "\tlastitemused   == %" PRIuZ "\n", aiState.lastitemused.get());
		vfs_printf(scr_file, "\told target     == %" PRIuZ "\n", aiState.getOldTarget().get());

		// message handling
		vfs_printf(scr_file, "\torder == %d\n", aiState.order_value);
		vfs_printf(scr_file, "\tcounter == %d\n", aiState.order_counter);

		// waypoints
		vfs_printf(scr_file, "\twp_tail == %d\n", aiState.wp_lst._tail);
		vfs_printf(scr_file, "\twp_head == %d\n\n", aiState.wp_lst._head);
	}

	// Clear the button latches.
	if (!pchr->isPlayer()) {
		pchr->resetInputCommands();
	}

	// Reset the target if it can't be seen.
	if (aiState.getTarget() != aiState.getSelf()) {
		const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[aiState.getTarget()];
		if (target && !pchr->canSeeObject(target)) {
			aiState.setTarget(aiState.getSelf());
		}
	}

	// Reset the script state.
	script_state_t my_state;

	// Reset the ai.
	aiState.terminate = false;
	script.indent = 0;

	// Run the AI Script.
	script.set_pos(0);
	while (!aiState.terminate && script.get_pos() < script._instructions.getNumberOfInstructions()) {
		// This is used by the Else function
		// it only keeps track of functions.
		script.indent_last = script.indent;
		script.indent = script._instructions[script.get_pos()].getDataBits();

		// Was it a function.
		if (script._instructions[script.get_pos()].isInv()) {
			if (!script_state_t::run_function_call(my_state, aiState, script)) {
				break;
			}
		}
		else {
			if (!script_state_t::run_operation(my_state, aiState, script)) {
				break;
			}
		}
	}

	// Set movement latches
	if (!pchr->isPlayer()) {

		ai_state_t::ensure_wp(aiState);

		if (pchr->isMount() && pchr->getLeftHandItem()) {
			// Mount (rider is held in left grip)
			pchr->getObjectPhysics().setDesiredVelocity(pchr->getLeftHandItem()->getObjectPhysics().getDesiredVelocity());
		}
		else if (aiState.wp_valid) {
			// Normal AI
			pchr->getObjectPhysics().setDesiredVelocity(Vector2f(
									 (aiState.wp[kX] - pchr->getPosX()) / Info<float>::Grid::Size(),
									 (aiState.wp[kY] - pchr->getPosY()) / Info<float>::Grid::Size()));
		}
	}

	// Clear alerts for next time around
	RESET_BIT_FIELD(aiState.alert);
}
void scr_run_chr_script( const ObjectRef character )
{
    /// @author ZZ
    /// @details This function lets one character do AI stuff

    // Make sure that this module is initialized.
    scripting_system_begin();

	if (!_currentModule->getObjectHandler().exists(character))  {
		return;
	}
	Object *pchr = _currentModule->getObjectHandler().get(character);
	return scr_run_chr_script(pchr);
}

//--------------------------------------------------------------------------------------------
bool script_state_t::run_function_call( script_state_t& state, ai_state_t& aiState, script_info_t& script )
{
    Uint8  functionreturn;

    // check for valid execution pointer
    if ( script.get_pos() >= script._instructions.getNumberOfInstructions() ) return false;

    // Run the function
	functionreturn = state.run_function(aiState, script);

    // move the execution pointer to the jump code
    script.increment_pos();
    if ( functionreturn )
    {
        // move the execution pointer to the next opcode
		script.increment_pos();
    }
    else
    {
        // use the jump code to jump to the right location
        size_t new_index = script._instructions[script.get_pos()].getBits();

        // make sure the value is valid
        EGOBOO_ASSERT( new_index <= script._instructions.getNumberOfInstructions() );

        // actually do the jump
		script.set_pos(new_index);
    }

    return true;
}

//--------------------------------------------------------------------------------------------
/// @todo Merge with caller.
bool script_state_t::run_operation( script_state_t& state, ai_state_t& aiState, script_info_t& script )
{
    // check for valid execution pointer
    if ( script.get_pos() >= script._instructions.getNumberOfInstructions() ) return false;

    auto var_value = script._instructions[script.get_pos()].getValueBits();

    // debug stuff
    std::string variable = "UNKNOWN";
    if ( debug_scripts && debug_script_file )
    {

        for (auto  i = 0; i < script.indent; i++ ) { vfs_printf( debug_script_file, "  " ); }

        for (auto i = 0; i < Opcodes.size(); i++ )
        {
            if ( Token::Type::Variable == Opcodes[i]._type && var_value == Opcodes[i].iValue )
            {
                variable = Opcodes[i].cName;
                break;
            }
        }

        vfs_printf( debug_script_file, "%s = ", variable.c_str() );
    }

    // Get the number of operands
	script.increment_pos();
    auto operand_count = script._instructions[script.get_pos()].getBits();

    // Now run the operation
    state.operationsum = 0;
    for (auto i = 0; i < operand_count && script.get_pos() < script._instructions.getNumberOfInstructions(); ++i )
    {
		script.increment_pos();
		script_state_t::run_operand(state, aiState, script);
    }
    if ( debug_scripts && debug_script_file )
    {
        vfs_printf( debug_script_file, " == %d \n", (int)state.operationsum );
    }

    // Save the results in the register that called the arithmetic
    state.store( var_value );

    // go to the next opcode
	script.increment_pos();

    return true;
}

//--------------------------------------------------------------------------------------------
Uint8 script_state_t::run_function(ai_state_t& aiState, script_info_t& script)
{
    auto constantIndex = script._instructions[script.get_pos()].getValueBits();
    const auto& constant = script._instructions.getConstantPool().getConstant(constantIndex);
    uint32_t functionIndex = constant.getAsInteger();

    // Assume that the function will pass, as most do
    uint8_t returnCode = true;
    auto& runtime = Runtime::get();
    {

        Ego::Time::ClockScope<Ego::Time::ClockPolicy::NonRecursive> scope(runtime.getClock());
        const auto& result = runtime._functionValueCodeToFunctionPointer.find(functionIndex);
        if (runtime._functionValueCodeToFunctionPointer.cend() == result)
        {
            throw RuntimeErrorException(__FILE__, __LINE__, "function not found");
        }
        returnCode = result->second(*this, aiState);
    }
    runtime.getStatistics().onFunctionInvoked(functionIndex, runtime.getClock().lst());
    return returnCode;
}

//--------------------------------------------------------------------------------------------
std::string getVariableName(int variableIndex)
{
    return _scriptVariableNames[variableIndex];
}

void script_state_t::store(uint8_t variableIndex)
{
    auto variableName = getVariableName(variableIndex);
    switch (variableIndex)
    {
        case VARTMPX:
            x = operationsum;
            break;

        case VARTMPY:
            y = operationsum;
            break;

        case VARTMPDISTANCE:
            distance = operationsum;
            break;

        case VARTMPTURN:
            turn = operationsum;
            break;

        case VARTMPARGUMENT:
            argument = operationsum;
            break;

        default:
            Log::Entry e(Log::Level::Warning, __FILE__, __LINE__);
            e << "variable " << variableName << "/" << variableIndex << " not found" << Log::EndOfEntry;
            Log::get() << e;
            break;
    }
}

//--------------------------------------------------------------------------------------------


void script_state_t::run_operand( script_state_t& state, ai_state_t& aiState, script_info_t& script )
{
    /// @author ZZ
    /// @details This function does the scripted arithmetic in OPERATOR, OPERAND pscriptrs

	if (!_currentModule->getObjectHandler().exists(aiState.getSelf())) return;
	Object *pchr = _currentModule->getObjectHandler().get(aiState.getSelf());

    Object *ptarget = nullptr;
	if (_currentModule->getObjectHandler().exists(aiState.getTarget()))
    {
		ptarget = _currentModule->getObjectHandler().get(aiState.getTarget());
    }

    Object *powner = nullptr;
	if (_currentModule->getObjectHandler().exists(aiState.owner))
    {
		powner = _currentModule->getObjectHandler().get(aiState.owner);
    }

    std::string varname;

    // get the operator
    int32_t iTmp = 0;
    
    uint8_t operation = script._instructions[script.get_pos()].getDataBits();
    if (script._instructions[script.get_pos()].isLdc()) {
        // Get the working opcode from a constant, constants are all but high 5 bits
        auto constantIndex = script._instructions[script.get_pos()].getValueBits();
        const auto& constant = script._instructions.getConstantPool().getConstant(constantIndex);
        iTmp = constant.getAsInteger();
        if (debug_scripts) {
            std::stringstream stringStream;
            stringStream << iTmp;
            varname = stringStream.str();
        }
    }
    else
    {
        // Get the variable opcode from a register
        uint8_t variable = script._instructions[script.get_pos()].getValueBits();
        varname = getVariableName(variable);
        switch ( variable )
        {
            case VARTMPX:
                iTmp = state.x;
                break;

            case VARTMPY:
                iTmp = state.y;
                break;

            case VARTMPDISTANCE:
                iTmp = state.distance;
                break;

            case VARTMPTURN:
                iTmp = state.turn;
                break;

            case VARTMPARGUMENT:
                iTmp = state.argument;
                break;

            case VARRAND:
                iTmp = Random::next(std::numeric_limits<uint16_t>::max());
                break;

            case VARSELFX:
                iTmp = pchr->getPosX();
                break;

            case VARSELFY:
                iTmp = pchr->getPosY();
                break;

            case VARSELFTURN:
                iTmp = uint16_t(pchr->ori.facing_z);
                break;

            case VARSELFCOUNTER:
				iTmp = aiState.order_counter;
                break;

            case VARSELFORDER:
				iTmp = aiState.order_value;
                break;

            case VARSELFMORALE:
                iTmp = _currentModule->getTeamList()[pchr->team_base].getMorale();
                break;

            case VARSELFLIFE:
                iTmp = FLOAT_TO_FP8(pchr->getLife());
                break;

            case VARTARGETX:
                iTmp = ( nullptr == ptarget ) ? 0 : ptarget->getPosX();
                break;

            case VARTARGETY:
                iTmp = ( nullptr == ptarget ) ? 0 : ptarget->getPosY();
                break;

            case VARTARGETDISTANCE:
                if ( nullptr == ptarget )
                {
                    iTmp = 0x7FFFFFFF;
                }
                else
                {
                    iTmp = std::abs(ptarget->getPosX() - pchr->getPosX())
                         + std::abs(ptarget->getPosY() - pchr->getPosY());
                }
                break;

            case VARTARGETTURN:
                iTmp = ( nullptr == ptarget ) ? 0 : uint16_t(ptarget->ori.facing_z);
                break;

            case VARLEADERX:
            {
                iTmp = pchr->getPosX();
                std::shared_ptr<Object> leader = _currentModule->getTeamList()[pchr->team].getLeader();
                if ( leader )
                    iTmp = leader->getPosX();
                break;
            }

            case VARLEADERY:
            {
                iTmp = pchr->getPosY();
                std::shared_ptr<Object> leader = _currentModule->getTeamList()[pchr->team].getLeader();
                if ( leader )
                    iTmp = leader->getPosY();

                break; 
            }

            case VARLEADERDISTANCE:
                {
                    std::shared_ptr<Object> pleader = _currentModule->getTeamList()[pchr->team].getLeader();
                    if ( !pleader )
                    {
                        iTmp = 0x7FFFFFFF;
                    }
                    else
                    {
                        iTmp = std::abs(pleader->getPosX() - pchr->getPosX())
                             + std::abs(pleader->getPosY() - pchr->getPosY());
                    }
                }
                break;

            case VARLEADERTURN:
                iTmp = uint16_t(pchr->ori.facing_z);
                if ( _currentModule->getTeamList()[pchr->team].getLeader() )
                    iTmp = uint16_t(_currentModule->getTeamList()[pchr->team].getLeader()->ori.facing_z);

                break;

            case VARGOTOX:
				ai_state_t::ensure_wp(aiState);

				if (!aiState.wp_valid)
                {
                    iTmp = pchr->getPosX();
                }
                else
                {
					iTmp = aiState.wp[kX];
                }
                break;

            case VARGOTOY:
				ai_state_t::ensure_wp(aiState);

				if (!aiState.wp_valid)
                {
                    iTmp = pchr->getPosY();
                }
                else
                {
					iTmp = aiState.wp[kY];
                }
                break;

            case VARGOTODISTANCE:
				ai_state_t::ensure_wp(aiState);

				if (!aiState.wp_valid)
                {
                    iTmp = 0x7FFFFFFF;
                }
                else
                {
					iTmp = std::abs(aiState.wp[kX] - pchr->getPosX())
						 + std::abs(aiState.wp[kY] - pchr->getPosY());
                }
                break;

            case VARTARGETTURNTO:
                if ( NULL == ptarget )
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = FACING_T(vec_to_facing( ptarget->getPosX() - pchr->getPosX() , ptarget->getPosY() - pchr->getPosY() ));
                    iTmp = Ego::Math::clipBits<16>( iTmp );
                }
                break;

            case VARPASSAGE:
				iTmp = aiState.passage;
                break;

            case VARWEIGHT:
                iTmp = pchr->holdingweight;
                break;

            case VARSELFALTITUDE:
                iTmp = pchr->getPosZ() - pchr->getObjectPhysics().getGroundElevation();
                break;

            case VARSELFID:
				iTmp = pchr->getProfile()->getIDSZ(IDSZ_TYPE).toUint32();
                break;

            case VARSELFHATEID:
				iTmp = pchr->getProfile()->getIDSZ(IDSZ_HATE).toUint32();
                break;

            case VARSELFMANA:
                iTmp = FLOAT_TO_FP8(pchr->getMana());
                if ( pchr->getAttribute(Ego::Attribute::CHANNEL_LIFE) )  iTmp += FLOAT_TO_FP8(pchr->getLife());

                break;

            case VARTARGETSTR:
                iTmp = ( NULL == ptarget ) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::MIGHT));
                break;

            case VARTARGETINT:
                iTmp = ( NULL == ptarget ) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::INTELLECT));
                break;

            case VARTARGETDEX:
                iTmp = ( NULL == ptarget ) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::AGILITY));
                break;

            case VARTARGETLIFE:
                iTmp = ( NULL == ptarget ) ? 0 : FLOAT_TO_FP8(ptarget->getLife());
                break;

            case VARTARGETMANA:
                if ( NULL == ptarget )
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = FLOAT_TO_FP8(ptarget->getMana());
                    if ( ptarget->getAttribute(Ego::Attribute::CHANNEL_LIFE) ) iTmp += FLOAT_TO_FP8(ptarget->getLife());
                }

                break;

            case VARTARGETLEVEL:
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->experiencelevel;
                break;

            case VARTARGETSPEEDX:
                iTmp = ( NULL == ptarget ) ? 0 : std::abs(ptarget->vel[kX]);
                break;

            case VARTARGETSPEEDY:
                iTmp = ( NULL == ptarget ) ? 0 : std::abs(ptarget->vel[kY]);
                break;

            case VARTARGETSPEEDZ:
                iTmp = ( NULL == ptarget ) ? 0 : std::abs(ptarget->vel[kZ]);
                break;

            case VARSELFSPAWNX:
                iTmp = pchr->getSpawnPosition()[kX];
                break;

            case VARSELFSPAWNY:
                iTmp = pchr->getSpawnPosition()[kY];
                break;

            case VARSELFSTATE:
				iTmp = aiState.state;
                break;

            case VARSELFCONTENT:
				iTmp = aiState.content;
                break;

            case VARSELFSTR:
                iTmp = FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::MIGHT));
                break;

            case VARSELFINT:
                iTmp = FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::INTELLECT));
                break;

            case VARSELFDEX:
                iTmp = FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::AGILITY));
                break;

            case VARSELFMANAFLOW:
                iTmp = FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::SPELL_POWER));
                break;

            case VARTARGETMANAFLOW:
                iTmp = ( NULL == ptarget ) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::SPELL_POWER));
                break;

            case VARSELFATTACHED:
				iTmp = number_of_attached_particles(aiState.getSelf());
                break;

            case VARSWINGTURN:
                {
					auto camera = CameraSystem::get().getCamera(aiState.getSelf());
                    iTmp = nullptr != camera ? camera->getSwing() << 2 : 0;
                }
                break;

            case VARXYDISTANCE:
                iTmp = std::sqrt( state.x * state.x + state.y * state.y );
                break;

            case VARSELFZ:
                iTmp = pchr->getPosZ();
                break;

            case VARTARGETALTITUDE:
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->getPosZ() - ptarget->getObjectPhysics().getGroundElevation();
                break;

            case VARTARGETZ:
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->getPosZ();
                break;

            case VARSELFINDEX:
				iTmp = aiState.getSelf().get();
                break;

            case VAROWNERX:
                iTmp = ( NULL == powner ) ? 0 : powner->getPosX();
                break;

            case VAROWNERY:
                iTmp = ( NULL == powner ) ? 0 : powner->getPosY();
                break;

            case VAROWNERTURN:
                iTmp = ( NULL == powner ) ? 0 : uint16_t(powner->ori.facing_z);
                break;

            case VAROWNERDISTANCE:
                if ( NULL == powner )
                {
                    iTmp = 0x7FFFFFFF;
                }
                else
                {
                    iTmp = std::abs(powner->getPosX() - pchr->getPosX())
                         + std::abs(powner->getPosY() - pchr->getPosY());
                }
                break;

            case VAROWNERTURNTO:
                if ( NULL == powner )
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = FACING_T(vec_to_facing( powner->getPosX() - pchr->getPosX() , powner->getPosY() - pchr->getPosY() ));
                    iTmp = Ego::Math::clipBits<16>( iTmp );
                }
                break;

            case VARXYTURNTO:
                iTmp = FACING_T(vec_to_facing( state.x - pchr->getPosX() , state.y - pchr->getPosY() ));
                iTmp = Ego::Math::clipBits<16>( iTmp );
                break;

            case VARSELFMONEY:
                iTmp = pchr->getMoney();
                break;

            case VARSELFACCEL:
                iTmp = ( pchr->getAttribute(Ego::Attribute::ACCELERATION) * 100.0f );
                break;

            case VARTARGETEXP:
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->experience;
                break;

            case VARSELFAMMO:
                iTmp = pchr->ammo;
                break;

            case VARTARGETAMMO:
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->ammo;
                break;

            case VARTARGETMONEY:
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->getMoney();
                break;

            case VARTARGETTURNAWAY:
                if ( NULL == ptarget )
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = FACING_T(vec_to_facing( ptarget->getPosX() - pchr->getPosX() , ptarget->getPosY() - pchr->getPosY() ));
                    iTmp = Ego::Math::clipBits<16>( iTmp );
                }
                break;

            case VARSELFLEVEL:
                iTmp = pchr->experiencelevel;
                break;

            case VARTARGETRELOADTIME:
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->reload_timer;
                break;

            case VARSPAWNDISTANCE:
                iTmp = std::abs( pchr->getSpawnPosition()[kX] - pchr->getPosX() )
                     + std::abs( pchr->getSpawnPosition()[kY] - pchr->getPosY() );
                break;

            case VARTARGETMAXLIFE:
                iTmp = ( NULL == ptarget ) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::MAX_LIFE));
                break;

            case VARTARGETTEAM:
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->team;
                //iTmp = REF_TO_INT( chr_get_iteam( pself->target ) );
                break;

            case VARTARGETARMOR:
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->skin;
                break;

            case VARDIFFICULTY:
                iTmp = static_cast<uint32_t>(egoboo_config_t::get().game_difficulty.getValue());
                break;

            case VARTIMEHOURS:
                iTmp = Ego::Time::LocalTime().getHours();
                break;

            case VARTIMEMINUTES:
                iTmp = Ego::Time::LocalTime().getMinutes();
                break;

            case VARTIMESECONDS:
                iTmp = Ego::Time::LocalTime().getSeconds();
                break;

            case VARDATEMONTH:
                iTmp = Ego::Time::LocalTime().getMonth() + 1; /// @todo The addition of +1 should be removed and
				                                              /// the whole Ego::Time::LocalTime class should be
				                                              /// made available via EgoScript. However, EgoScript
				                                              /// is not yet ready for that ... not yet.
                break;

            case VARDATEDAY:
                iTmp = Ego::Time::LocalTime().getDayOfMonth();
                break;

            default:
				Log::get().message("%s:%d:%s: script error - model == %d, class name == \"%s\" - Unknown variable found!\n", \
					               __FILE__, __LINE__, __FUNCTION__, REF_TO_INT(script_error_model), script_error_classname);
                break;
        }
    }

    // Now do the math
    std::string op = "UNKNOWN";
    switch ( operation )
    {
        case OPADD:
            op = "ADD";
            state.operationsum = int(state.operationsum) + iTmp;
            break;

        case OPSUB:
            op = "SUB";
            state.operationsum = int(state.operationsum) - iTmp;
            break;

        case OPAND:
            op = "AND";
            state.operationsum = int(state.operationsum) & iTmp;
            break;

        case OPSHR:
            op = "SHR";
            state.operationsum = int(state.operationsum) >> iTmp;
            break;

        case OPSHL:
            op = "SHL";
            state.operationsum = int(state.operationsum) << iTmp;
            break;

        case OPMUL:
            op = "MUL";
            state.operationsum = int(state.operationsum) * iTmp;
            break;

        case OPDIV:
            op = "DIV";
            if ( iTmp != 0 )
            {
                state.operationsum = static_cast<float>(state.operationsum) / iTmp;
            }
            else
            {
				Log::get().message("%s:%d:%s: script error - model == %d, class name == \"%s\" - Cannot divide by zero!\n", \
					               __FILE__, __LINE__, __FUNCTION__, REF_TO_INT(script_error_model), script_error_classname);
            }
            break;

        case OPMOD:
            op = "MOD";
            if ( iTmp != 0 )
            {
                state.operationsum = int(state.operationsum) % iTmp;
            }
            else
            {
				Log::get().message("%s:%d:%s: script error - model == %d, class name == \"%s\" - Cannot modulo by zero!\n",\
					               __FILE__, __LINE__, __FUNCTION__, REF_TO_INT( script_error_model ), script_error_classname );
            }
            break;

        default:
			Log::get().message("%s:%d:%s: script error - model == %d, class name == \"%s\" - unknown op\n",\
				               __FILE__, __LINE__, __FUNCTION__, REF_TO_INT( script_error_model ), script_error_classname );
            break;
    }

    if ( debug_scripts && debug_script_file )
    {
        vfs_printf( debug_script_file, "%s %s(%d) ", op.c_str(), varname.c_str(), iTmp );
    }
}

//--------------------------------------------------------------------------------------------

bool script_info_t::increment_pos() {
	if (_position >= _instructions.getNumberOfInstructions()) {
		return false;
	}
	_position++;
    return true;
}

size_t script_info_t::get_pos() const {
	return _position;
}

bool script_info_t::set_pos(size_t position) {
	if (position >= _instructions.getNumberOfInstructions()) {
		return false;
	}
	_position = position;
    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool ai_state_t::get_wp( ai_state_t& self )
{
    // try to load up the top waypoint

    if ( !_currentModule->getObjectHandler().exists( self.getSelf() ) ) return false;

    self.wp_valid = waypoint_list_t::peek( self.wp_lst, self.wp );

    return true;
}

//--------------------------------------------------------------------------------------------
bool ai_state_t::ensure_wp(ai_state_t& self)
{
    // is the current waypoint is not valid, try to load up the top waypoint

	if (!_currentModule->getObjectHandler().exists(self.getSelf())) {
		return false;
	}
	if (self.wp_valid) {
		return true;
	}
    return ai_state_t::get_wp(self);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void set_alerts( const ObjectRef character )
{
    /// @author ZZ
    /// @details This function polls some alert conditions

    // invalid characters do not think
	if (!_currentModule->getObjectHandler().exists(character)) {
		return;
	}
	Object *pchr = _currentModule->getObjectHandler().get(character);
	ai_state_t& aiState = pchr->ai;

	if (waypoint_list_t::empty(aiState.wp_lst)) {
		return;
	}

    // let's let mounts get alert updates...
    // imagine a mount, like a racecar, that needs to make sure that it follows X
    // waypoints around a track or something

    // mounts do not get alerts
    // if ( _currentModule->getObjectHandler().exists(pchr->attachedto) ) return;

    // is the current waypoint is not valid, try to load up the top waypoint
    ai_state_t::ensure_wp(aiState);

    bool at_waypoint = false;
	if (aiState.wp_valid)
    {
		at_waypoint = (std::abs(pchr->getPosX() - aiState.wp[kX]) < WAYTHRESH) &&
			          (std::abs(pchr->getPosY() - aiState.wp[kY]) < WAYTHRESH);
    }

    if ( at_waypoint )
    {
		SET_BIT(aiState.alert, ALERTIF_ATWAYPOINT);

		if (waypoint_list_t::finished(aiState.wp_lst))
        {
            // we are now at the last waypoint
            // if the object can be alerted to last waypoint, do it
            // this test needs to be done because the ALERTIF_ATLASTWAYPOINT
            // doubles for "at last waypoint" and "not put away"
            if ( !pchr->getProfile()->isEquipment() )
            {
				SET_BIT(aiState.alert, ALERTIF_ATLASTWAYPOINT);
            }

            // !!!!restart the waypoint list, do not clear them!!!!
			waypoint_list_t::reset(aiState.wp_lst);

            // load the top waypoint
			ai_state_t::get_wp(aiState);
        }
		else if (waypoint_list_t::advance(aiState.wp_lst))
        {
            // load the top waypoint
			ai_state_t::get_wp(aiState);
        }
    }
}

//--------------------------------------------------------------------------------------------
void issue_order( const ObjectRef character, Uint32 value )
{
    /// @author ZZ
    /// @details This function issues an value for help to all teammates
    int counter = 0;

    const std::shared_ptr<Object> &pchr = _currentModule->getObjectHandler()[character];

    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
    {
        if ( object->isTerminated() ) continue;

        if ( object->getTeam() == pchr->getTeam() )
        {
            ai_state_t::add_order(object->ai, value, counter);
            counter++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void issue_special_order( uint32_t value, const IDSZ2& idsz )
{
    /// @author ZZ
    /// @details This function issues an order to all characters with the a matching special IDSZ
    int counter = 0;

    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
    {
        if ( object->isTerminated() ) continue;

        if ( idsz == object->getProfile()->getIDSZ(IDSZ_SPECIAL) )
        {
            ai_state_t::add_order(object->ai, value, counter);
            counter++;
        }
    }
}

//--------------------------------------------------------------------------------------------

ai_state_t::ai_state_t()
    : AI::State<ObjectRef>() {
	_clock = std::make_shared<Ego::Time::Clock<Ego::Time::ClockPolicy::NonRecursive>>("", 8);
	poof_time = -1;
	changed = false;
	terminate = false;

	// who are we related to?
	owner = ObjectRef::Invalid;
	child = ObjectRef::Invalid;

	// some local storage
	alert = 0;
	state = 0;
	content = 0;
	passage = 0;
	timer = 0;
	for (size_t i = 0; i < STOR_COUNT; ++i) {
		x[i] = 0;
		y[i] = 0;
	}

	// ai memory from the last event
	bumplast_time = 0;

	hitlast = ObjectRef::Invalid;
	directionlast = Facing(0);
	damagetypelast = DamageType::DAMAGE_DIRECT;
	lastitemused = ObjectRef::Invalid;

	// message handling
	order_value = 0;
	order_counter = 0;

	// waypoints
	wp_valid = false;
	wp_lst._head = wp_lst._tail = 0;
	astar_timer = 0;
}

ai_state_t::~ai_state_t() {
	_clock = nullptr;
}

void ai_state_t::reset(ai_state_t& self)
{
	self._clock->reinit();

	self.poof_time = -1;
	self.changed = false;
	self.terminate = false;

	// who are we related to?
	self.setSelf(ObjectRef::Invalid);
    self.setTarget(ObjectRef::Invalid);
    self.setOldTarget(ObjectRef::Invalid);
    self.setBumped(ObjectRef::Invalid);
    self.setLastAttacker(ObjectRef::Invalid);

	self.owner = ObjectRef::Invalid;
	self.child = ObjectRef::Invalid;

	// some local storage
	self.alert = 0;         ///< Alerts for AI script
	self.state = 0;
	self.content = 0;
	self.passage = 0;
	self.timer = 0;
	for (size_t i = 0; i < STOR_COUNT; ++i) {
		self.x[i] = 0;
		self.y[i] = 0;
	}
    self.maxSpeed = 1.0f;

	// ai memory from the last event

	self.bumplast_time = 0;


	self.hitlast = ObjectRef::Invalid;
	self.directionlast = Facing(0);
	self.damagetypelast = DamageType::DAMAGE_DIRECT;
	self.lastitemused = ObjectRef::Invalid;

	// message handling
	self.order_value = 0;
	self.order_counter = 0;

	// waypoints
	self.wp_valid = false;
	self.wp_lst._head = self.wp_lst._tail = 0;
	self.astar_timer = 0;
}

bool ai_state_t::add_order(ai_state_t& self, Uint32 value, Uint16 counter)
{
    // this function is only truely valid if there is no other order
	bool retval = HAS_NO_BITS(self.alert, ALERTIF_ORDERED);

	SET_BIT(self.alert, ALERTIF_ORDERED);
    self.order_value   = value;
    self.order_counter = counter;

    return retval;
}

bool ai_state_t::set_changed(ai_state_t& self)
{
    /// @author BB
    /// @details do something tricky here

    bool retval = false;

	if (HAS_NO_BITS(self.alert, ALERTIF_CHANGED))
    {
		SET_BIT(self.alert, ALERTIF_CHANGED);
        retval = true;
    }

	if (!self.changed)
    {
		self.changed = true;
        retval = true;
    }

    return retval;
}

bool ai_state_t::set_bumplast(ai_state_t& self, const ObjectRef ichr)
{
    /// @author BB
    /// @details bumping into a chest can initiate whole loads of update messages.
    ///     Try to throttle the rate that new "bump" messages can be passed to the ai

	if (!_currentModule->getObjectHandler().exists(ichr)) {
		return false;
	}

    // 5 bumps per second?
	if (self.getBumped() != ichr || update_wld > self.bumplast_time + GameEngine::GAME_TARGET_UPS / 5) {
		self.bumplast_time = update_wld;
		SET_BIT(self.alert, ALERTIF_BUMPED);
    }
	self.setBumped(ichr);

    return true;
}

void ai_state_t::spawn(ai_state_t& self, const ObjectRef index, const PRO_REF iobj, uint16_t rank)
{
    const std::shared_ptr<Object> &pchr = _currentModule->getObjectHandler()[index];
	ai_state_t::reset(self);

	if (!pchr) {
		return;
	}

	self.setSelf(index);
    self.setTarget(index);
    self.setOldTarget(index);
    self.setBumped(index);
	self.alert = ALERTIF_SPAWNED;
	self.state = pchr->getProfile()->getStateOverride();
	self.content = pchr->getProfile()->getContentOverride();
	self.passage = 0;
	self.owner = index;
	self.child = index;

    waypoint_list_t::push(self.wp_lst, pchr->getSpawnPosition().x(), pchr->getSpawnPosition().y());

    self.maxSpeed = 1.0f;

	self.hitlast = index;

	self.order_counter = rank;
	self.order_value = 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
script_state_t::script_state_t()
	: x(0), y(0), turn(0), distance(0),
	  argument(0), operationsum()
{
}
