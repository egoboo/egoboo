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

#include "egolib/Tests/Math/MathTestUtilities.hpp"

namespace Ego {
namespace Math {
namespace Test {

EgoTest_TestCase(Interpolate_Linear) {
public:
    template <typename T>
    using list = std::vector<T>;
    template <typename T1, typename T2>
    using pair = std::pair<T1, T2>;

public:
    using Colour3f = Colour3f;

    static void test(const Colour3f& x, const Colour3f& y) {
        auto f = Math::Interpolate<Colour3f, Math::InterpolationMethod::Linear>();
        EgoTest_Assert(x == f(x, y, 0.0f));
        EgoTest_Assert(y == f(x, y, 1.0f));
    }
 
    EgoTest_Test(testColor3f) {
        test(Colour3f::red(), Colour3f::red());
        test(Colour3f::red(), Colour3f::green());
        test(Colour3f::red(), Colour3f::blue());

        test(Colour3f::green(), Colour3f::green());
        test(Colour3f::green(), Colour3f::blue());
        test(Colour3f::green(), Colour3f::red());
        
        test(Colour3f::blue(), Colour3f::blue());
        test(Colour3f::blue(), Colour3f::green());
        test(Colour3f::blue(), Colour3f::red());
    }


public:
    using Colour4f = Colour4f;

    static void test(const Colour4f& x, const Colour4f& y) {
        auto f = Math::Interpolate<Colour4f,Math::InterpolationMethod::Linear>();
        EgoTest_Assert(x == f(x, y, 0.0f));
        EgoTest_Assert(y == f(x, y, 1.0f));
    }


    EgoTest_Test(testColor4f) {
        test(Colour4f::red(), Colour4f::red());
        test(Colour4f::red(), Colour4f::green());
        test(Colour4f::red(), Colour4f::blue());

        test(Colour4f::green(), Colour4f::green());
        test(Colour4f::green(), Colour4f::blue());
        test(Colour4f::green(), Colour4f::red());

        test(Colour4f::blue(), Colour4f::blue());
        test(Colour4f::blue(), Colour4f::green());
        test(Colour4f::blue(), Colour4f::red());
    }

public:
    using Vector3f = ::Vector3f;
    // Add basis vectors (1,0,0), (0,1,0), (0,0,1).
    list<Vector3f> basis(list<Vector3f> l) {
        list<Vector3f> l1{l};
        l1.emplace_back(1.0f, 0.0f, 0.0f);
        l1.emplace_back(0.0f, 1.0f, 0.0f);
        l1.emplace_back(0.0f, 0.0f, 1.0f);
        return l1;
    }
    // Add the zero vector to a list.
    list<Vector3f> zero(list<Vector3f> l) {
        list<Vector3f> l1{l};
        l1.emplace_back(0.0f, 0.0f, 0.0f);
        return l1;
    }
    // Add negation of all list elements to a list.
    template <typename T>
    list<T> negation(list<T> l) {
        list<T> l1{l};
        for (auto e : l) {
            auto e1 = -e;
            l1.emplace_back(-e1);
        }
        return l1;
    }
    // Cartesian product list of two list.
    template <typename T>
    list<pair<T, T>> cartesian(const list<T>& a, const list<T>& b) {
        list<pair<T, T>> c;
        for (auto x : a) {
            for (auto y : b) {
                c.emplace_back(x,y);
            }
        }
        return c;
    }

    static void test(const Vector3f& x, const Vector3f& y) {
        auto f = Math::Interpolate<Vector3f,Math::InterpolationMethod::Linear>();
        EgoTest_Assert(x == f(x, y, 0.0f));
        EgoTest_Assert(y == f(x, y, 1.0f));
    }

    EgoTest_Test(testVector3f) {
        list<Vector3f> l = zero(negation(basis(list<Vector3f>())));
        list<pair<Vector3f, Vector3f>> cl = cartesian(l,l);
        for (auto c : cl) {
            test(c.first, c.second);
        }
    }

    EgoTest_Test(testPoint3f) {
    }
};

}
}
} // end namespaces Ego::Math::Test
