#include <vector>
#include "EgoTest/EgoTest.hpp"
#include "egolib/Core/QuadTree.hpp"
#include "egolib/Math/Standard.hpp"
#include "egolib/Math/Random.hpp"

EgoTest_TestCase(QuadTreeTest)
{

class QuadTreeElement
{
public:
    QuadTreeElement(float x, float y, float size) : _bounds(Vector2f(x-size, y-size), Vector2f(x+size, y+size))
    {
        //ctor
    }

    AABB2f& getAABB2D() { return _bounds; }

    bool isTerminated() { return false; }

private:
    AABB2f _bounds;
};
    
static AABB2f anAABBFromARect(float centerX, float centerY, float size) {
    return AABB2f(Vector2f(centerX - size, centerY - size), Vector2f(centerX + size, centerY + size));
}

EgoTest_Test(runQuadTreeTestStatic)
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
    for(int i = 0; i < _testElements.size(); ++i) {
        float x = Random::next(128, 256);
        float y = Random::next(128, 256);
        _testElements[i]->getAABB2D() = AABB2f(Vector2f(x, y), Vector2f(x+10, y+10));
    }

    //Rebuild the tree
    _quadTree.clear(0, 0, 256, 256);
    for(const std::shared_ptr<QuadTreeElement> &element : _testElements) {
        _quadTree.insert(element);
    }

    //All elements should be found in bottom right now
    std::vector<std::shared_ptr<QuadTreeElement>> result;
    AABB2f searchArea = AABB2f(Vector2f(128, 128), Vector2f(256, 256));
    _quadTree.find(searchArea, result);
    EgoTest_Assert(result.size() == _testElements.size());

    //If we look top half, we should find nothing now
    result.clear();
    searchArea = AABB2f(Vector2f(0, 0), Vector2f(256, 127));
    _quadTree.find(searchArea, result);
    EgoTest_Assert(result.empty());
}
    
};
