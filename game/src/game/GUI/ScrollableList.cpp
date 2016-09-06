#include "game/GUI/ScrollableList.hpp"
#include "game/GUI/Button.hpp"

namespace Ego {
namespace GUI {

const size_t ScrollableList::COMPONENT_LINE_SPACING = 5;

ScrollableList::ScrollableList() :
    _currentIndex(0),
    _mouseOver(false),
    _downButton(std::make_shared<Button>("+")),
    _upButton(std::make_shared<Button>("-")) {

    // Add the down button.
    _downButton->setSize(Vector2f(32, 32));
    _downButton->setOnClickFunction([this] {
        setScrollPosition(_currentIndex + 1);
    });
    _downButton->setEnabled(false);
    _downButton->setParent(this);
    addComponent(_downButton);

    // Add the up button.
    _upButton->setSize(Vector2f(32, 32));
    _upButton->setOnClickFunction([this] {
        setScrollPosition(_currentIndex - 1);
    });
    _upButton->setEnabled(false);
    _upButton->setParent(this);
    addComponent(_upButton);

    updateScrollButtons();
}

void ScrollableList::setScrollPosition(int position) {
    //Limit bounds
    if (position < 0 || position >= getComponentCount()) {
        return;
    }

    //Set new scrolling position
    _currentIndex = position;

    //Dynamically enable and disable scrolling buttons as needed
    _upButton->setEnabled(_currentIndex > 0);
    _downButton->setEnabled(_currentIndex < getComponentCount() - 1);

    //Shift position of all components in container
    int yOffset = 0;
    int componentCount = 0;
    for (const std::shared_ptr<Component> &component : iterator()) {
        if (component != _upButton && component != _downButton) {
            if (componentCount < _currentIndex || yOffset + component->getHeight() >= getHeight()) {
                component->setVisible(false);
                componentCount++;
                continue;
            }

            component->setVisible(true);
            component->setPosition(Point2f(0, yOffset));
            yOffset += component->getHeight() + COMPONENT_LINE_SPACING;
            componentCount++;
        }
    }
}

void ScrollableList::updateScrollButtons() {
    _upButton->setPosition(Point2f(getWidth() - _upButton->getWidth(), 0));
    _downButton->setPosition(Point2f(getWidth() - _downButton->getWidth(), getHeight() - _downButton->getHeight()));
}

void ScrollableList::setWidth(float width) {
    Component::setWidth(width);
    updateScrollButtons();
}

void ScrollableList::setHeight(float height) {
    Component::setHeight(height);
    updateScrollButtons();
}

void ScrollableList::setX(float x) {
    Component::setX(x);
    updateScrollButtons();
}

void ScrollableList::setY(float y) {
    Component::setY(y);
    updateScrollButtons();
}

void ScrollableList::drawContainer(DrawingContext& drawingContext) {
    //TODO
}

void ScrollableList::draw(DrawingContext& drawingContext) {
    //First draw the container itself
    drawContainer(drawingContext);

    //Now draw all components inside it
    for (const std::shared_ptr<Component>& component : iterator()) {
        if (!component->isVisible()) continue;  //Ignore hidden/destroyed components
        component->draw(drawingContext);
    }
}

bool ScrollableList::notifyMouseWheelTurned(const Events::MouseWheelTurnedEventArgs& e) {
    if (_mouseOver) {
        if (e.getDelta().y() > 0) {
            _upButton->doClick();
        } else {
            _downButton->doClick();
        }
    }
    return _mouseOver;
}

bool ScrollableList::notifyMouseMoved(const Events::MouseMovedEventArgs& e) {
    _mouseOver = contains(e.getPosition());

    if (_downButton->notifyMouseMoved(e)) return true;
    if (_upButton->notifyMouseMoved(e)) return true;

    return Container::notifyMouseMoved(e);
}

bool ScrollableList::notifyMouseButtonPressed(const Events::MouseButtonPressedEventArgs& e) {
    if (_downButton->notifyMouseButtonPressed(e)) return true;
    if (_upButton->notifyMouseButtonPressed(e)) return true;
    return Container::notifyMouseButtonPressed(e);
}

void ScrollableList::forceUpdate() {
    setScrollPosition(_currentIndex);
}

void ScrollableList::setPosition(const Point2f& position) {
    Component::setPosition(position);
    updateScrollButtons();
    setScrollPosition(_currentIndex);
}

} // namespace GUI
} // namespace Ego
