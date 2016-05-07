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
#include "egolib/egolib.h"

EgoTest_TestCase(Signal) {

EgoTest_Test(signal) {
    Ego::Signal<void(const std::string&)> signal;
    // (1) Invoke with no subscriber.
    signal("Hello, World!");
    // (2) Invoke with one subscriber.
    bool invoked = false;
    auto function = [&invoked](const std::string& s) { if (s == "Hello, World!") { invoked = true; } };
    auto subscription = signal.subscribe(function);
    signal("Hello, World!");
    signal.unsubscribe(subscription);
    EgoTest_Assert(true == invoked);
}

};
