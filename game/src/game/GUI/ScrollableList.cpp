#include "game/GUI/ScrollableList.hpp"
#include "game/GUI/Button.hpp"

const size_t ScrollableList::COMPONENT_LINE_SPACING = 5;

ScrollableList::ScrollableList() :
	_currentIndex(0),
	_mouseOver(false),
	_downButton(std::make_shared<Button>("+")),
	_upButton(std::make_shared<Button>("-"))
{
	_downButton->setSize(32, 32);
	_downButton->setOnClickFunction([this]{
		setScrollPosition(_currentIndex + 1);
	});

	_upButton->setSize(32, 32);
	_upButton->setOnClickFunction([this]{
		setScrollPosition(_currentIndex - 1);
	});
	_upButton->setEnabled(false);
	updateScrollButtons();
}

void ScrollableList::setScrollPosition(int position)
{
	//Limit bounds
	if(position < 0 || position >= _componentList.size()) {
		return;
	}

	//Set new scrolling position
	_currentIndex = position;

	//Dynamically enable and disable scrolling buttons as needed
	_upButton->setEnabled(_currentIndex > 0);
	_downButton->setEnabled(_currentIndex < _componentList.size()-1);

	//Shift position of all components in container
	int yOffset = 0;
	for(int i = 0; i < _componentList.size(); ++i) {
		if(i < _currentIndex || yOffset >= getHeight()) {
			_componentList[i]->setVisible(false);
			continue;
		}

		_componentList[i]->setVisible(true);
		_componentList[i]->setPosition(getX(), getY() + yOffset);
		yOffset += _componentList[i]->getHeight() + COMPONENT_LINE_SPACING;
	}
}

void ScrollableList::updateScrollButtons()
{
	_upButton->setPosition(getX() + getWidth() - _upButton->getWidth(), getY());
	_downButton->setPosition(getX() + getWidth() - _downButton->getWidth(), getY()+getHeight());
}

void ScrollableList::setWidth(const int width)
{
	GUIComponent::setWidth(width);
	updateScrollButtons();
}

void ScrollableList::setHeight(const int height)
{
	GUIComponent::setHeight(height);
	updateScrollButtons();
}

void ScrollableList::setX(const int x)
{
	GUIComponent::setX(x);
	updateScrollButtons();
}

void ScrollableList::setY(const int y)
{
	GUIComponent::setY(y);
	updateScrollButtons();
}

void ScrollableList::drawContainer()
{
	//TODO
}

void ScrollableList::draw()
{
	//First draw the container itself
	drawContainer();

	//Now draw all components inside it
	int y = 0;
    _componentListMutex.lock();
    for(size_t i = _currentIndex; i < _componentList.size() && y < getHeight(); ++i) {
        if(!_componentList[i]->isVisible()) continue;  //Ignore hidden/destroyed components
        _componentList[i]->draw();
        y += _componentList[i]->getHeight();
    }
    _componentListMutex.unlock();

    //Draw up and down buttons
    _downButton->draw();
    _upButton->draw();
}

bool ScrollableList::notifyMouseScrolled(const int amount)
{
	if(_mouseOver)
	{
		if(amount > 0) {
			_upButton->doClick();
		}
		else {
			_downButton->doClick();
		}
	}
	return _mouseOver;
}

bool ScrollableList::notifyMouseMoved(const int x, const int y)
{
	_mouseOver = contains(x, y);

    if(_downButton->notifyMouseMoved(x, y)) return true;
    if(_upButton->notifyMouseMoved(x, y)) return true;

	return ComponentContainer::notifyMouseMoved(x, y);
}

bool ScrollableList::notifyMouseClicked(const int button, const int x, const int y) 
{
    if(_downButton->notifyMouseClicked(button, x, y)) return true;
    if(_upButton->notifyMouseClicked(button, x, y)) return true;
	return ComponentContainer::notifyMouseClicked(button, x, y);
}
