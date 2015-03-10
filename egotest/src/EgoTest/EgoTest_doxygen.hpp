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

/// @file EgoTest/EgoTest_doxygen.hpp
/// @brief Doxygen comments for the EgoTest backends
/// @ingroup EgoTest

#pragma once

#ifndef DOXYGEN_SHOULD_IGNORE_THIS
#error This file is only used for Doxygen comments; it should never be included.
#endif

/**
 * @brief
 * Declare a test case.
 * @param TESTCASENAME
 * The test case's class name.
 */
#define EgoTest_DeclareTestCase(TESTCASENAME)

/**
 * @brief
 * End the declaration of a test case.
 */
#define EgoTest_EndDeclaration()

/**
 * @brief 
 * Define a test case.
 * @param TESTCASENAME
 * The test case's class name.
 * @note
 * The test case's class name should be the same as the one used by @c EgoTest_DeclareTestCase.
 */
#define EgoTest_TestCase(TESTCASENAME) class TESTCASENAME {

/**
 * @brief
 * End a definition of a test case.
 */
#define EgoTest_EndTestCase() };

/**
 * @brief
 * Define a test.
 * @param TESTNAME
 * The test's method name.
 */
#define EgoTest_Test(TESTNAME) void TESTNAME()

/**
 * @brief
 * Test an expression.
 * @param EXPRESSION
 * The expression to test, if it evaluates to @c false, the current test fails.
 */
#define EgoTest_Assert(EXPRESSION)
