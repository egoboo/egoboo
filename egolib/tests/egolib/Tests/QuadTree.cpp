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
namespace Test {

EgoTest_TestCase(QuadTree) {
    class QuadTreeElement {
    public:
        QuadTreeElement(float x, float y, float size) : _bounds(Point2f(x - size, y - size), Point2f(x + size, y + size)) {
            //ctor
        }

        AABB2f& getAABB2D() { return _bounds; }

        bool isTerminated() { return false; }

    private:
        AABB2f _bounds;
    };

    static AABB2f anAABBFromARect(float centerX, float centerY, float size) {
        return AABB2f(Point2f(centerX - size, centerY - size), Point2f(centerX + size, centerY + size));
    }

    EgoTest_Test(runQuadTreeTestStatic) {
        Ego::QuadTree<QuadTreeElement> _quadTree;
        std::vector<std::shared_ptr<QuadTreeElement>> _testElements;

        //Put a fat element in the middle of the main tree
        _testElements.push_back(std::make_shared<QuadTreeElement>(128, 128, 20));

        //Put one element in each corner
        _testElements.push_back(std::make_shared<QuadTreeElement>(0, 0, 5));
        _testElements.push_back(std::make_shared<QuadTreeElement>(256, 0, 5));
        _testElements.push_back(std::make_shared<QuadTreeElement>(0, 256, 5));
        _testElements.push_back(std::make_shared<QuadTreeElement>(256, 256, 5));

        //Now build the quad tree
        _quadTree.clear(0, 0, 256, 256);
        for (const std::shared_ptr<QuadTreeElement> &element : _testElements) {
            _quadTree.insert(element);
        }

        std::vector<std::shared_ptr<QuadTreeElement>> findResults;

        //Searching outside the tree should produce no results
        _quadTree.find(anAABBFromARect(-50, -50, 20), findResults);
        EgoTest_Assert(findResults.empty());
        findResults.clear();

        //Searching around each corner should find one element
        _quadTree.find(anAABBFromARect(0, 0, 50), findResults);
        EgoTest_Assert(findResults.size() == 1);
        findResults.clear();

        _quadTree.find(anAABBFromARect(256, 0, 50), findResults);
        EgoTest_Assert(findResults.size() == 1);
        findResults.clear();

        _quadTree.find(anAABBFromARect(0, 256, 50), findResults);
        EgoTest_Assert(findResults.size() == 1);
        findResults.clear();

        _quadTree.find(anAABBFromARect(256, 256, 50), findResults);
        EgoTest_Assert(findResults.size() == 1);
        findResults.clear();

        //Searching in the middle should find exactly one element
        _quadTree.find(anAABBFromARect(128, 128, 50), findResults);
        EgoTest_Assert(findResults.size() == 1);
        findResults.clear();

        //Searching whole tree should find all elements
        _quadTree.find(anAABBFromARect(128, 128, 128), findResults);
        EgoTest_Assert(findResults.size() == _testElements.size());
        findResults.clear();

        //Now move all elements in bottom right corner
        for (int i = 0; i < _testElements.size(); ++i) {
            float x = Random::next(128, 256);
            float y = Random::next(128, 256);
            _testElements[i]->getAABB2D() = AABB2f(Point2f(x, y), Point2f(x + 10, y + 10));
        }

        //Rebuild the tree
        _quadTree.clear(0, 0, 256, 256);
        for (const std::shared_ptr<QuadTreeElement> &element : _testElements) {
            _quadTree.insert(element);
        }

        //All elements should be found in bottom right now
        std::vector<std::shared_ptr<QuadTreeElement>> result;
        AABB2f searchArea = AABB2f(Point2f(128, 128), Point2f(256, 256));
        _quadTree.find(searchArea, result);
        EgoTest_Assert(result.size() == _testElements.size());

        //If we look top half, we should find nothing now
        result.clear();
        searchArea = AABB2f(Point2f(0, 0), Point2f(256, 127));
        _quadTree.find(searchArea, result);
        EgoTest_Assert(result.empty());
    }

};

} // namespace Test
} // namespace Ego

