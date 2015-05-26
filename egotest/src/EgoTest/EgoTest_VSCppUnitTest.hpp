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

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#define EgoTest_TestCase(TESTCASENAME) \
TEST_CLASS(TESTCASENAME)

#define EgoTest_Test(TESTNAME) \
TEST_METHOD(TESTNAME)

#define EgoTest_SetUpTest() \
TEST_METHOD_INITIALIZE(setUp)

#define EgoTest_TearDownTest() \
TEST_METHOD_CLEANUP(tearDown)

#define EgoTest_SetUpTestCase() \
TEST_CLASS_INITIALIZE(setUpClass)

#define EgoTest_TearDownTestCase() \
TEST_CLASS_CLEANUP(tearDownClass)

#define EgoTest_Assert(EXPRESSION) \
Assert::IsTrue(EXPRESSION)
