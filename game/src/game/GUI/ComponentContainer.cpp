#include "game/GUI/ComponentContainer.hpp"
#include "game/GameStates/GameState.hpp"
#include "game/Core/GameEngine.hpp"
#include "game/GUI/GUIComponent.hpp"

ComponentContainer::ComponentContainer() :
    _componentList(),
    _componentListMutex(),
    _componentDestroyed(false)
{
	//ctor
}

void ComponentContainer::addComponent(std::shared_ptr<GUIComponent> component)
{
    std::lock_guard<std::recursive_mutex> lock(_componentListMutex);
    _componentList.push_back(component);
    component->_parent = this;
}

void ComponentContainer::removeComponent(std::shared_ptr<GUIComponent> component)
{
    std::lock_guard<std::recursive_mutex> lock(_componentListMutex);
    _componentList.erase(std::remove(_componentList.begin(), _componentList.end(), component), _componentList.end());
}

void ComponentContainer::clearComponents()
{
    std::lock_guard<std::recursive_mutex> lock(_componentListMutex);
    _componentList.clear();
}

size_t ComponentContainer::getComponentCount() const
{
	return _componentList.size();
}

void ComponentContainer::drawAll()
{
    //Render the container itself
    drawContainer();

    //Draw reach GUI component
    _gameEngine->getUIManager()->beginRenderUI();
    std::lock_guard<std::recursive_mutex> lock(_componentListMutex);
    for(const std::shared_ptr<GUIComponent> component : _componentList)
    {
        if(!component->isVisible()) continue;  //Ignore hidden/destroyed components
        component->draw();
    }
    _gameEngine->getUIManager()->endRenderUI();
}

bool ComponentContainer::notifyMouseMoved(const int x, const int y)
{
    {
        std::lock_guard<std::recursive_mutex> lock(_componentListMutex);
        for (auto i = _componentList.rbegin(); i != _componentList.rend(); ++i ) { 
            const std::shared_ptr<GUIComponent> &component = *i;
            if(!component->isEnabled()) continue;
            if(component->notifyMouseMoved(x, y)) return true;
        }
    }
    cleanDestroyedComponents();
    return false;
}

bool ComponentContainer::notifyKeyDown(const int keyCode)
{
    {
        std::lock_guard<std::recursive_mutex> lock(_componentListMutex);
        //Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first
        for (auto i = _componentList.rbegin(); i != _componentList.rend(); ++i ) { 
            const std::shared_ptr<GUIComponent> &component = *i;
            if(!component->isEnabled()) continue;
            if(component->notifyKeyDown(keyCode)) return true;
        } 
    }
    cleanDestroyedComponents();
    return false;
}

bool ComponentContainer::notifyMouseClicked(const int button, const int x, const int y)
{
    {
        std::lock_guard<std::recursive_mutex> lock(_componentListMutex);
        for (auto i = _componentList.rbegin(); i != _componentList.rend(); ++i ) { 
            const std::shared_ptr<GUIComponent> &component = *i;
            if(!component->isEnabled()) continue;
            if(component->notifyMouseClicked(button, x, y)) return true;
        }
    } 
    cleanDestroyedComponents();
    return false;
}

bool ComponentContainer::notifyMouseScrolled(const int amount)
{
    {
        std::lock_guard<std::recursive_mutex> lock(_componentListMutex);
        for (auto i = _componentList.rbegin(); i != _componentList.rend(); ++i ) { 
            const std::shared_ptr<GUIComponent> &component = *i;
            if(!component->isEnabled()) continue;
            if(component->notifyMouseScrolled(amount)) return true;
        } 
    }
    cleanDestroyedComponents();
    return false;
}

void ComponentContainer::notifyDestruction()
{
    //Try to remove destroyed components immediatly if possible
    if(_componentListMutex.try_lock())
    {
        _componentList.erase(std::remove_if(_componentList.begin(), _componentList.end(), 
            [](const std::shared_ptr<GUIComponent> &component) {return component->isDestroyed(); }), _componentList.end());
        _componentListMutex.unlock();
    }
    else
    {
        //Deferred destruction
        _componentDestroyed = true;
    }
}

void ComponentContainer::cleanDestroyedComponents()
{
    if(!_componentDestroyed) return;
    _componentDestroyed = false;
    std::lock_guard<std::recursive_mutex> lock(_componentListMutex);

    _componentList.erase(std::remove_if(_componentList.begin(), _componentList.end(), 
        [](const std::shared_ptr<GUIComponent> &component) {return component->isDestroyed(); }), 
        _componentList.end());
}

void ComponentContainer::bringComponentToFront(std::shared_ptr<GUIComponent> component)
{
    removeComponent(component);
    addComponent(component);
}
