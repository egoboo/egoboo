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

#include "EgoTest/EgoTest.hpp"

#include <string>

namespace Parsing {
    EgoTest_TestCase(TestCase1) {
        EgoTest_Test(No1) {
            std::string a = "qa\\
\
\
"}EgoTest_Test(Fail1) {";
            EgoTest_Assert(a == "qa\"}EgoTest_Test(Fail1) {");
        }
        
        // Hello everyone \
        EgoTest_Test(Fail2) { \
            EgoTest_Assert(false);\
        }
        
        /********/ // hee hee /* */ */ *//*/
        EgoTest_Test(No2) {
            const char ch = '\\
\
\
n';
            EgoTest_Assert(ch == '\n');
        }
        
        /*/
         
        EgoTest_Test(Fail3) {
            EgoTest_Assert(false);
        }
         
        /*/
    };
    
    EgoTest_TestCase(TestCase2) {
        
        EgoTest_Test(No3) {
            std::string test = R"whahaha(
        } EgoTest_Test(Fail4) {
            EgoTest_Assert(false);)whahaha";
            
            EgoTest_Assert(!test.empty());
        }
    
        EgoTest_Test(No4) {
            std::string test = R"(""""""""'''''\\23rkewsA\ba,qp1\
            \112ifka31pw//
            qwkac11111/*1111*/1)";
            
            EgoTest_Assert(!test.empty());
        }
        
        EgoTest_Test
        
        
        
        
        
        (No5){
            std::string test = R"[(((((((()))))|)())))))]"))))[";
        }
        
    };
}
