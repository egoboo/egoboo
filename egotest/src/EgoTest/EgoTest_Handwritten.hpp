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

/// @file EgoTest/EgoTest_Handwritten.hpp
/// @brief A handwritten EgoTest backend
/// @ingroup EgoTest

#pragma once

#include <functional>
#include <map>
#include <string>

namespace EgoTest
{
    class TestCase
    {
    protected:
        TestCase();
    public:
        virtual ~TestCase();
        virtual void setUp();
        virtual void tearDown();
        virtual void setUpClass();
        virtual void tearDownClass();
    };
    
    void doAssert(bool condition, const std::string &conditionStr, const std::string &function, const std::string &file, int line);
    
    int handleTest(const std::string &testName, const std::function<void(void)> &test);
}

#if defined(_MSC_VER)
#define EgoTest_FUNCTION __FUNCSIG__
#elif defined(__GNUC__)
#define EgoTest_FUNCTION __PRETTY_FUNCTION__
#else
#define EgoTest_FUNCTION __func__
#endif

#define EgoTest_TestCase(TESTCASENAME) \
struct TESTCASENAME : ::EgoTest::TestCase

#define EgoTest_Test(TESTNAME) \
void TESTNAME()

#define EgoTest_SetUpTest() \
void setUp()

#define EgoTest_TearDownTest() \
void tearDown()

#define EgoTest_SetUpTestCase() \
void setUpClass()

#define EgoTest_TearDownTestCase() \
void tearDownClass()

#define EgoTest_Assert(EXPRESSION) \
try { \
    ::EgoTest::doAssert(EXPRESSION, "\"" #EXPRESSION "\" was false", EgoTest_FUNCTION, __FILE__, __LINE__); \
} catch (...) { \
    ::EgoTest::doAssert(false, "uncaught exception", EgoTest_FUNCTION, __FILE__, __LINE__); \
}
