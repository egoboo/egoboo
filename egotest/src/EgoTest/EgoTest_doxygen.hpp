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
 *  Define a test case.
 * @param TESTCASENAME
 *  The test case's class name.
 */
#define EgoTest_TestCase(TESTCASENAME) \
class TESTCASENAME

/**
 * @brief
 *  Define a test.
 * @param TESTNAME
 *  The test's method name.
 * @note
 *  The method name may not be one of the following: setUp tearDown setUpClass tearDownClass
 */
#define EgoTest_Test(TESTNAME) \
void TESTNAME()

/**
 * @brief
 *  Define a method that runs before each test.
 */
#define EgoTest_SetUpTest() \
void setUp()

/**
 * @brief
 *  Define a method that runs after each test.
 */
#define EgoTest_TearDownTest() \
void tearDown()

/**
 * @brief
 *  Define a method that runs before all tests.
 */
#define EgoTest_SetUpTestCase() \
void setUpClass()

/**
 * @brief
 *  Define a method that runs after all tests.
 */
#define EgoTest_TearDownTestCase() \
void tearDownClass()

/**
 * @brief
 *  Test an expression.
 * @param EXPRESSION
 *  The expression to test, if it evaluates to @c false, the current test fails.
 */
#define EgoTest_Assert(EXPRESSION)
