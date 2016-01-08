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

/// @file EgoTest/EgoTest_Handwritten.cpp
/// @brief A handwritten EgoTest backend
/// @ingroup EgoTest

#include "EgoTest/EgoTest_Handwritten.hpp"

#include <iostream>

namespace EgoTest
{
    std::map<std::string, std::function<int(void)>> getTestCases();
}

namespace
{
    static EgoTest::TestCase *currentTestCase;
    static int currentTestFailures;
    static int currentTestsRan;
    static int totalTestsRan;
    
    enum class ColorCodes : uint8_t
    {
        NORMAL = 0,
        RED    = 31,
        YELLOW = 33,
    };
    
    template<class CharT, class Traits>
    std::basic_ostream<CharT, Traits> &operator<<(std::basic_ostream<CharT, Traits> &stream, ColorCodes color)
    {
#ifdef _MSC_VER
        return stream;
#else
        return stream << "\033[" << static_cast<uint16_t>(color) << "m";
#endif
    }
}

namespace EgoTest
{

    TestCase::TestCase() 
    {
        currentTestCase = this;
        setUpClass();
    }

    TestCase::~TestCase()
    {
        tearDownClass();
        currentTestCase = nullptr;
    }

    void TestCase::setUp() {}
    void TestCase::tearDown() {}
    void TestCase::setUpClass() {}
    void TestCase::tearDownClass() {}
}

void EgoTest::doAssert(bool condition, const std::string &conditionStr, const std::string &function, const std::string &file, int line)
{
    if (condition) return;
    currentTestFailures++;
    std::cout << ColorCodes::RED << file << ":" << line << ": " << conditionStr << "\n" << ColorCodes::NORMAL;
}

int EgoTest::handleTestFunc(const std::string &testName, const std::function<void(void)> &test)
{
    currentTestFailures = 0;
    bool setUp = false;
    totalTestsRan++;
    std::cout << "Running test '" << testName << "'...\n";
    
    try
    {
        currentTestCase->setUp();
        setUp = true;
    }
    catch (...)
    {
        std::cout << ColorCodes::RED << "Uncaught exception while setting up.\n" << ColorCodes::NORMAL;
        currentTestFailures++;
    }
    
    if (setUp)
    {
        try
        {
            currentTestsRan++;
            test();
        }
        catch (...)
        {
            std::cout << ColorCodes::RED << "Uncaught exception in EgoTest::handleTestFunc.\n" << ColorCodes::NORMAL;
            currentTestFailures++;
        }
        
        try
        {
            currentTestCase->tearDown();
        }
        catch (...)
        {
            std::cout << ColorCodes::RED << "Uncaught exception while cleaning up.\n" << ColorCodes::NORMAL;
            currentTestFailures++;
        }
    }
    return currentTestFailures;
}

int main(int argc, char *argv[])
{
    std::cout << "\n";
    auto testCases = EgoTest::getTestCases();
    int totalFailures = 0;
    int numTestCasesRan = 0;
    int numTestsRan = 0;
    int numTestCasesFailured = 0;
    
    for (const auto &testCase : testCases)
    {
        totalTestsRan = 0;
        try
        {
            std::cout << "Starting test case \"" << testCase.first << "\".\n";
            int failures = testCase.second();
            numTestCasesRan++;
            if (failures) numTestCasesFailured++;
            totalFailures += failures;
            std::cout << "Test case \"" << testCase.first << "\": " << totalTestsRan << " tests, " << failures << " failures\n\n"; 
        }
        catch (...)
        {
            std::cout << ColorCodes::RED << "Test case \"" << testCase.first << "\" has thrown an exception?\n";
            std::cout << ColorCodes::YELLOW << "Ran " << totalTestsRan << "tests before failure.\n" << ColorCodes::NORMAL;
        }
        numTestsRan += totalTestsRan;
    }
    
    int numTestCases = testCases.size();
    int failedTestCases = numTestCases - numTestCasesRan;
    
    std::cout << numTestCases << " total test cases, " << failedTestCases << " failed to run, "
                << numTestCasesFailured << " had failures.\n";
    std::cout << numTestsRan << " total tests, " << totalFailures << " failures.\n\n";
    
    return totalFailures ? EXIT_FAILURE : EXIT_SUCCESS;
}
