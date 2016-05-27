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

/// @file   egolib/Core/QuadTree.hpp
/// @brief  QuadTree data structure for fast element lookup based on bounding boxes
/// @author Johan Jansen

#pragma once

#include "egolib/Math/_Include.hpp"
#include "egolib/Math/Standard.hpp"

namespace Ego
{

template<typename T>
class QuadTree
{
public:

    /**
    * @brief
    *   Construct a root QuadTree node with infinite bounds
    **/
    QuadTree() : QuadTree(
        std::numeric_limits<float>::lowest(), 
        std::numeric_limits<float>::lowest(), 
        std::numeric_limits<float>::max(), 
        std::numeric_limits<float>::max())
    {
        //ctor  
    }

    /**
    * @brief
    *   Constructor with bounded limits
    **/
    QuadTree(const float minX, const float minY, const float maxX, const float maxY) :
        _bounds(Point2f(minX, minY), Point2f(maxX, maxY)),
        _nodes(),
        _northWest(nullptr),
        _northEast(nullptr),
        _southWest(nullptr),
        _southEast(nullptr)
    {
        //ctor
    }

    /**
    * @brief
    *   Inserts an element into this QuadTree
    * @return
    *   true if the object fits within the bounds of this tree
    **/
    bool insert(const std::shared_ptr<T> &element)
    {
        Ego::Math::Intersects<AxisAlignedBox2f,AxisAlignedBox2f> intersects;
        //Element does not belong in this tree
        if(!intersects(_bounds, element->getAxisAlignedBox2D())) {
            return false;
        }

        //Check if we have room
        if(_nodes.size() < QUAD_TREE_NODE_CAPACITY) {
            _nodes.push_back(element);
            return true;
        }

        // Otherwise, subdivide and then add the point to whichever node will accept it
        if(_northWest == nullptr) {
            subdivide();
        }

        //Add element to a sub-tree
        _northWest->insert(element);
        _northEast->insert(element);
        _southWest->insert(element);
        _southEast->insert(element);

        //Should be added to at least 1
        return true;
    }

    /**
    * @brief
    *   Find all elements that are within range of a specified point in this QuadTree's
    *   bounding box.
    * @param searchArea
    *   The bounding box which is used for finding elements
    * @param result
    *   Vector of all elements that fit within the search area
    **/
    void find(const AxisAlignedBox2f &searchArea, std::vector<std::shared_ptr<T>> &result) const
    {
        Ego::Math::Intersects<AxisAlignedBox2f, AxisAlignedBox2f> intersects;
        //Search grid is not part of our bounds
        if(!intersects(_bounds, searchArea)) {
            return;
        }

        //Check all nodes in this QuadTree
        for(const std::weak_ptr<T> &weakElement : _nodes) {
            std::shared_ptr<T> element = weakElement.lock();

            //Make sure element still exists
            if(element != nullptr) {

                //Already added?
                if(std::find(result.begin(), result.end(), element) != result.end()) {
                    continue;
                }

                //Check if element is within search area
                if(intersects(element->getAxisAlignedBox2D(), searchArea)) {
                    result.push_back(element);
                }
            }
        }

        //Check subtrees (if any)
        if(_northWest != nullptr) {
            _northWest->find(searchArea, result);
            _northEast->find(searchArea, result);
            _southWest->find(searchArea, result);
            _southEast->find(searchArea, result);
        }
    }

    /**
    * @brief
    *   Clears all elements from this QuadTree and all its children
    **/
    void clear(const float minX, const float minY, const float maxX, const float maxY)
    {
        //Reset bounds
        _bounds = AxisAlignedBox2f(Point2f(minX, minY), Point2f(maxX, maxY));

        //Clear children and all elements
        _nodes.clear();
        _northWest.reset();
        _northEast.reset();
        _southWest.reset();
        _southEast.reset();
    }

private:
    /**
    * @brief
    *   Helper function to subdivide this QuadTree into four more QuadTrees
    **/
    void subdivide()
    {
        float topLeftX = _bounds.getMin()[kX];
        float topLeftY = _bounds.getMin()[kY];

        float bottomRightX = _bounds.getMax()[kX];
        float bottomRightY = _bounds.getMax()[kY];

        float midX = (topLeftX + bottomRightX) * 0.5f;
        float midY = (topLeftY + bottomRightY) * 0.5f;

        //Allocate memory for the subdivision
        _northWest = std::make_unique<QuadTree<T>>(topLeftX, topLeftY, midX, midY);
        _northEast = std::make_unique<QuadTree<T>>(midX, topLeftY, bottomRightX, midY);
        _southWest = std::make_unique<QuadTree<T>>(topLeftX, midY, midX, bottomRightY);
        _southEast = std::make_unique<QuadTree<T>>(midX, midY, bottomRightX, bottomRightY);
    }

private:
    static const size_t QUAD_TREE_NODE_CAPACITY = 4;    //< Maximum number of nodes in tree before subdivision

    AxisAlignedBox2f _bounds;                                     //< 2D AABB

    std::vector<std::weak_ptr<T>> _nodes;               //< List of nodes contained in this QuadTree

    std::unique_ptr<QuadTree<T>> _northWest;
    std::unique_ptr<QuadTree<T>> _northEast;
    std::unique_ptr<QuadTree<T>> _southWest;
    std::unique_ptr<QuadTree<T>> _southEast;
};

} //namespace Ego
