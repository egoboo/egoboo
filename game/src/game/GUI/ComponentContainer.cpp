#include "game/GUI/ComponentContainer.hpp"
#include "game/GameStates/GameState.hpp"
#include "game/Core/GameEngine.hpp"
#include "game/GUI/GUIComponent.hpp"

ComponentContainer::ComponentContainer() :
    _componentList(),
    _componentDestroyed(false),
    _semaphoreLock(0),
    _containerMutex()
{
	//ctor
}

ComponentContainer::~ComponentContainer()
{
    if(_semaphoreLock != 0) 
    {
        throw new std::logic_error("Destructing ComponentContainer while iterating");
    }
}


void ComponentContainer::addComponent(std::shared_ptr<GUIComponent> component)
{
    std::lock_guard<std::mutex> lock(_containerMutex);
    _componentList.push_back(component);
    component->_parent = this;
}

void ComponentContainer::removeComponent(std::shared_ptr<GUIComponent> component)
{
    std::lock_guard<std::mutex> lock(_containerMutex);
    _componentList.erase(std::remove(_componentList.begin(), _componentList.end(), component), _componentList.end());
}

void ComponentContainer::clearComponents()
{
    std::lock_guard<std::mutex> lock(_containerMutex);
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
    for(const std::shared_ptr<GUIComponent> component : iterator())
    {
        if(!component->isVisible()) continue;  //Ignore hidden/destroyed components
        component->draw();
    }
    _gameEngine->getUIManager()->endRenderUI();
}

bool ComponentContainer::notifyMouseMoved(const int x, const int y)
{
    //Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first
    ComponentIterator it = iterator();
    for (auto i = it.rbegin(); i != it.rend(); ++i ) { 
        std::shared_ptr<GUIComponent> component = *i;
        if(!component->isEnabled()) continue;
        if(component->notifyMouseMoved(x, y)) return true;
    }
    return false;
}

bool ComponentContainer::notifyKeyDown(const int keyCode)
{
    //Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first
    ComponentIterator it = iterator();
    for (auto i = it.rbegin(); i != it.rend(); ++i ) { 
        std::shared_ptr<GUIComponent> component = *i;
        if(!component->isEnabled()) continue;
        if(component->notifyKeyDown(keyCode)) return true;
    } 
    return false;
}

bool ComponentContainer::notifyMouseClicked(const int button, const int x, const int y)
{
    //Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first
    ComponentIterator it = iterator();
    for (auto i = it.rbegin(); i != it.rend(); ++i ) { 
        std::shared_ptr<GUIComponent> component = *i;
        if(!component->isEnabled()) continue;
        if(component->notifyMouseClicked(button, x, y)) return true;
    }
    return false;
}

bool ComponentContainer::notifyMouseReleased(const int button, const int x, const int y)
{
    //Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first
    ComponentIterator it = iterator();
    for (auto i = it.rbegin(); i != it.rend(); ++i ) { 
        std::shared_ptr<GUIComponent> component = *i;
        if(!component->isEnabled()) continue;
        if(component->notifyMouseReleased(button, x, y)) return true;
    }
    return false;
}

bool ComponentContainer::notifyMouseScrolled(const int amount)
{
    //Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first
    ComponentIterator it = iterator();
    for (auto i = it.rbegin(); i != it.rend(); ++i ) { 
        std::shared_ptr<GUIComponent> component = *i;
        if(!component->isEnabled()) continue;
        if(component->notifyMouseScrolled(amount)) return true;
    } 
    return false;
}

void ComponentContainer::notifyDestruction()
{
    //Deferred destruction
    _componentDestroyed = true;
}


void ComponentContainer::bringComponentToFront(std::shared_ptr<GUIComponent> component)
{
    removeComponent(component);
    addComponent(component);
}

void ComponentContainer::lock()
{
    std::lock_guard<std::mutex> lock(_containerMutex);
    _semaphoreLock++;
}

void ComponentContainer::unlock()
{
    std::lock_guard<std::mutex> lock(_containerMutex);
    if(_semaphoreLock == 0) 
    {
        throw new std::logic_error("ComponentContainer calling unlock() without lock()");
    }

    //Release one lock
    _semaphoreLock--;

    //If all locks are released, remove all destroyed components
    if(_semaphoreLock == 0 && _componentDestroyed) {
        _componentList.erase(std::remove_if(_componentList.begin(), _componentList.end(), 
            [](std::shared_ptr<GUIComponent> component) {return component->isDestroyed(); }), 
            _componentList.end());
        _componentDestroyed = false;
    }
}

void ComponentContainer::setComponentList(const std::vector<std::shared_ptr<GUIComponent>> &list)
{
    clearComponents();
    for(const auto& component : list) addComponent(component);
}
