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

/// @file EgoLib/Script/RuntimeStatistics.hpp
/// @brief Declaration of tatistics for a runtime
/// @author Michael Heilmann

#pragma once

#include "idlib/platform.hpp"
#include <string>
#include <map>

namespace Ego::Script {

/// @brief Collect statistics of a runtime.
/// @remark
/// Typical usage is callback based:
/// - runtime x performs action a and measures the time required for performing of that action.
/// - runtime x notifies a runtime stats object on the action that was performed and the time required for performing that action.
template <typename FunctionName>
struct IRuntimeStatistics {
protected:
    /// @brief Statistics for a single function.
    struct FunctionStatistics {
        /// @brief The number of times the function was called.
        long numberOfCalls;
        /// @brief The sum of the times spend on invocations of the function.
        double totalTime;
        /// @brief The maximum time spend on an invocation of the function.
        double maxTime;

        /// @brief Construct these function statistics with the specified values
        /// @param numberOfCalls the number of times the function was called.
        /// @param totalTime the sum of the times spend on invocations of the function.
        /// @param maxTime the maximum time spend on an invocation of the function.
        FunctionStatistics(long numberOfCalls, double totalTime, double maxTime)
            : numberOfCalls(numberOfCalls),
            totalTime(totalTime),
            maxTime(maxTime) {}

        /// @brief Copy-construct these function statistics from other function statistics.
        /// @param other the other function statistics
        FunctionStatistics(const FunctionStatistics& other) : numberOfCalls(other.numberOfCalls), totalTime(other.totalTime) {}

        /// @brief Assign these function statistics from other function statistics.
        /// @param other the other function statistics
        /// @return these function statistics
        const FunctionStatistics& operator=(const FunctionStatistics& other) {
            numberOfCalls = other.numberOfCalls;
            totalTime = other.totalTime;
            maxTime = other.maxTime;
            return *this;
        }
    };

    /// @brief Statistics for each individual function.
    std::map<FunctionName, FunctionStatistics> _functionStatistics;

protected:

    /// @brief Construct these runtime statistics.
    IRuntimeStatistics() {}

public:
    //// @brief Destruct these runtime statistics.
    virtual ~IRuntimeStatistics() {}

public:
    /// @brief Invoked if a function was called.
    /// @param functionName the name of the function that was called
    /// @param time the time (in milliseconds) that was spent on that call
    void onFunctionInvoked(const FunctionName& functionName, double time) {
        auto it = _functionStatistics.find(functionName);
        if (_functionStatistics.cend() == it) {
            _functionStatistics.emplace(functionName, FunctionStatistics(1, time, time));
        } else {
            (*it).second.numberOfCalls = (*it).first + 1;
            (*it).second.totalTime = time;
            (*it).second.maxTime = std::max((*it).second.maxTime, time);
        }
    }

    /// @brief Append the runtime statistics to the specified file.
    /// @param pathname the pathname of the file to append the runtime statistics to
    virtual void append(const std::string& pathname) = 0;
};

} // namespace Ego::Script
