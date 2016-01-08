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

/// @file EgoTest/EgoTest_VSCppUnitTest.hpp
/// @brief Visual Studio C++ Unit Test Framework backend for EgoTest
/// @ingroup EgoTest

#include "CppUnitTest.h"
#include "CppUnitTestAssert.h"

#define EgoTest_DeclareTestCase(TESTCASENAME)

#define EgoTest_EndDeclaration()

#define EgoTest_BeginTestCase(TESTCASENAME) \
TEST_CLASS(TESTCASENAME) {

#define EgoTest_EndTestCase() };

#define EgoTest_Test(TESTNAME) \
TEST_METHOD(TESTNAME)

#define EgoTest_Assert(EXPRESSION) \
    do { \
        ::Microsoft::VisualStudio::CppUnitTestFramework::__LineInfo lineInfo(__WFILE__, __FUNCTION__, __LINE__); \
        try { \
            ::Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsTrue(EXPRESSION, L"" #EXPRESSION, &lineInfo); \
        } catch (...) { \
            ::Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(L"uncaught exception", &lineInfo); \
        } \
    } while (0)

using namespace Microsoft::VisualStudio::CppUnitTestFramework;