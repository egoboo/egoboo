#pragma once

/**
* This interface class allows the object to listen to any SDL events occuring. If any of the functions
* return true, then it means the event will be consumed (and no other listeners will receive it)
**/
class InputListener
{
    public:
        virtual ~InputListener();
        virtual bool notifyMouseMoved(const int x, const int y);
        virtual bool notifyKeyDown(const int keyCode);
        virtual bool notifyMouseClicked(const int button, const int x, const int y);
        virtual bool notifyMouseScrolled(const int amount);
};
