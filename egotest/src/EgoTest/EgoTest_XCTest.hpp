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

#define EgoTest_DeclareTestCase(TESTCASENAME) \
@interface TESTCASENAME : XCTestCase

#define EgoTest_EndDeclaration() \
@end

#define EgoTest_TestCase(TESTCASENAME) \
@implementation TESTCASENAME

#define EgoTest_EndTestCase() \
@end

#define EgoTest_Test(TESTNAME) \
- (void)test_##TESTNAME

#define EgoTest_Assert(EXPRESSION) \
XCTAssert(EXPRESSION)
