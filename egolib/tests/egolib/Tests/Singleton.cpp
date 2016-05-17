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

namespace Ego {
namespace Core {
namespace Test {

struct Bar : public Ego::Core::Singleton<Bar> {
protected:
    friend struct Ego::Core::CreateFunctor<Bar>;
    friend struct Ego::Core::DestroyFunctor<Bar>;
    Bar() {
        std::cout << "Bar::Bar()" << std::endl;
    }
    ~Bar() {
        std::cout << "Bar::~Bar()" << std::endl;
    }
};

struct Foo : public Ego::Core::Singleton<Foo,Foo> {
protected:
    friend struct Ego::Core::CreateFunctor<Foo>;
    friend struct Ego::Core::DestroyFunctor<Foo>;
    Foo(const std::string& p) {
        std::cout << "Foo::Foo(" << p << ")" << std::endl;
    }
    ~Foo() {
        std::cout << "Foo::~Foo()" << std::endl;
    }
};

EgoTest_TestCase(Singleton) {

EgoTest_Test(run) {
    Bar::initialize();
    Bar::uninitialize();
    Foo::initialize(std::string("Hello, World!"));
    Foo::uninitialize();
}

}; // Singleton

} // namespace Test

template <>
struct CreateFunctor<Ego::Core::Test::Foo> {
    Ego::Core::Test::Foo *operator()(const std::string& x) const {
        return new Ego::Core::Test::Foo(x);
    }
};

} // namespace Core
} // namespace Ego
