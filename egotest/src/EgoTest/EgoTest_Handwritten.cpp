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
}

namespace EgoTest
{

    TestCase::TestCase() 
    {
        currentTestCase = this;
    }

    TestCase::~TestCase()
    {
        currentTestCase = nullptr;
    }

    void TestCase::setUp() {}
    void TestCase::tearDown() {}
    void TestCase::setUpClass() {}
    void TestCase::tearDownClass() {}
}

void EgoTest::doAssert(bool condition, std::string conditionStr, std::string function, std::string file, int line)
{
    if (condition) return;
    currentTestFailures++;
    std::cout << function << " (" << file << ":" << line << "): " << conditionStr << "\n";
}

int EgoTest::handleTest(const std::function<void(void)> &test)
{
    currentTestFailures = 0;
    bool setUp = false;
    totalTestsRan++;
    
    try
    {
        currentTestCase->setUp();
        setUp = true;
        currentTestsRan++;
        test();
    }
    catch (...)
    {
        std::cerr << "Uncaught exception in EgoTest::handleTest.\n";
        currentTestFailures++;
    }
    
    if (setUp)
    {
        try
        {
            currentTestCase->tearDown();
        }
        catch (...)
        {
            std::cerr << "Uncaught exception while cleaning up after a test.\n";
            currentTestFailures++;
        }
    }
    return currentTestFailures;
}

int main(int argc, char *argv[])
{
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
            int failures = testCase.second();
            numTestCasesRan++;
            if (failures) numTestCasesFailured++;
            totalFailures += failures;
            std::cout << "Test case \"" << testCase.first << "\": " << totalTestsRan << " tests, " << failures << " failures\n"; 
        }
        catch (...)
        {
            std::cerr << "Test case \"" << testCase.first << "\" has thrown an exception?\n";
            std::cerr << "Ran " << totalTestsRan << "tests before failure.\n";
        }
        numTestsRan += totalTestsRan;
    }
    
    int numTestCases = testCases.size();
    int failedTestCases = numTestCases - numTestCasesRan;
    
    std::cout << numTestCases << " total test cases, " << failedTestCases << " failed to run, "
                << numTestCasesFailured << " had failures.\n";
    std::cout << numTestsRan << " total tests, " << totalFailures << " failures.\n";
    
    return totalFailures ? EXIT_FAILURE : EXIT_SUCCESS;
}