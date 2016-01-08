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

/// @file EgoTest/EgoTest_XCTest.hpp
/// @brief Xcode 5 Test Framework backend for EgoTest
/// @ingroup EgoTest

#pragma once

#ifndef __OBJC__
#error Objective-C is required to use XCTest as the EgoTest backend.
#endif

#import <XCTest/XCTest.h>

namespace EgoTest
{
    class TestCase
    {
    protected:
        TestCase() {}
    public:
        virtual ~TestCase() {}
        virtual void setUp() {}
        virtual void tearDown() {}
        virtual void setUpClass() {}
        virtual void tearDownClass() {}
    };
    extern XCTestCase *currentTestCase;
}

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
do { \
    XCTestCase *self = ::EgoTest::currentTestCase; \
    XCTAssert(EXPRESSION); \
} while (0)
