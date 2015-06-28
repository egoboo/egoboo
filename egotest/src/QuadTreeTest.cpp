#include <assert>
#include <vector>
#include "egolib/Core/QuadTree.hpp"
#include "egolib/Math/Standard.hpp"
#include "egolib/Math/Random.hpp"

class QuadTreeElement
{
public:
    QuadTreeElement(float x, float y, float size) : _bounds(Vector2f(x-size, y-size), Vector2f(x+size, y+size))
    {
        //ctor
    }

    AABB_2D& getAABB2D() { return _bounds; }

    bool isTerminated() { return false; }

private:
    AABB_2D _bounds;
};

void runQuadTreeTestStatic()
{
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
    for(const std::shared_ptr<QuadTreeElement> &element : _testElements) {
        _quadTree.insert(element);
    }

    //Searching outside the tree should produce no results
    assert(_quadTree.find(-50, -50, 20).empty());

    //Searching around each corner should find one element
    assert(_quadTree.find(0, 0, 50).size() == 1);
    assert(_quadTree.find(256, 0, 50).size() == 1);
    assert(_quadTree.find(0, 256, 50).size() == 1);
    assert(_quadTree.find(256, 256, 50).size() == 1);

    //Searching in the middle should find exactly one element
    assert(_quadTree.find(128, 128, 50).size() == 1);

    //Searching whole tree should find all elements
    assert(_quadTree.find(128, 128, 128).size() == _testElements.size());

    //Now move all elements in bottom right corner
    for(int i = 0; i < _testElements.size(); ++i) {
        float x = Random::next(128, 256);
        float y = Random::next(128, 256);
        _testElements[i]->getAABB2D()._min = Vector2f(x, y);
        _testElements[i]->getAABB2D()._max = Vector2f(x+10, y+10);
    }

    //Rebuild the tree
    _quadTree.clear(0, 0, 256, 256);
    for(const std::shared_ptr<QuadTreeElement> &element : _testElements) {
        _quadTree.insert(element);
    }

    //All elements should be found in bottom right now
    std::vector<std::shared_ptr<QuadTreeElement>> result;
    AABB_2D searchArea = AABB_2D(Vector2f(128, 128), Vector2f(256, 256));
    _quadTree.find(searchArea, result);
    assert(result.size() == _testElements.size());

    //If we look top half, we should find nothing now
    result.clear();
    searchArea = AABB_2D(Vector2f(0, 0), Vector2f(256, 127));
    _quadTree.find(searchArea, result);
    assert(result.empty());
}
