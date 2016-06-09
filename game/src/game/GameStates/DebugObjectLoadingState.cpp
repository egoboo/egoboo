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

/// @file game/GameStates/DebugObjectLoadingState.cpp
/// @details Debugging state where one can debug loading objects
///          from the global repository or individual modules.
/// @author Johan Jansen, penguinflyer5234

#include "game/GameStates/DebugObjectLoadingState.hpp"
#include "game/GameStates/PlayingState.hpp"
#include "game/GameStates/LoadPlayerElement.hpp"
#include "game/Core/GameEngine.hpp"
#include "egolib/egoboo_setup.h"
#include "game/graphic.h"
#include "game/GUI/Button.hpp"
#include "game/GUI/Label.hpp"
#include "game/GUI/Image.hpp"
#include "game/GUI/ScrollableList.hpp"
#include "egolib/Math/Random.hpp"
#include "egolib/Audio/AudioSystem.hpp"

//For loading stuff
#include "game/Graphics/CameraSystem.hpp"
#include "game/game.h"
#include "game/graphic_billboard.h"
#include "game/link.h"
#include "egolib/fileutil.h"

class DebugObjectLoadingState::GrowableLabel : public GUIComponent
{
public:
    GrowableLabel(const std::string &text) :
    _text(text),
    _font(_gameEngine->getUIManager()->getFont(UIManager::FONT_DEBUG)),
    _textRenderer()
    {
        redraw();
    }
    
    void draw() override {
        _textRenderer->render(getX(), getY());
    }
    
    void setText(const std::string &text) {
        _text = text;
        redraw();
    }
    
    void setWidth(const int width) override {
        GUIComponent::setWidth(width);
        redraw();
    }
    
    void doParentRelayout();
    
private:
    void redraw() {
        int textHeight;
        _textRenderer = _font->layoutTextBox(_text, getWidth(), 0, 0, nullptr, &textHeight);
        setHeight(textHeight);
        doParentRelayout();
    }
    
    std::string _text;
    std::shared_ptr<Ego::Font> _font;
    std::shared_ptr<Ego::Font::LaidTextRenderer> _textRenderer;
};

struct DebugObjectLoadingState::ObjectGUIContainer : public ComponentContainer, public GUIComponent
{
    ObjectGUIContainer(const std::string &objectName,
                       const std::weak_ptr<ModuleLoader> &module) :
    _objectPath("/mp_objects/" + objectName),
    _objectName(std::make_shared<Button>(objectName)),
    _loadingText(std::make_shared<DebugObjectLoadingState::GrowableLabel>("Not Loaded")),
    _module(module)
    {
        _objectName->setSize(250, 30);
        
        addComponent(_objectName);
        addComponent(_loadingText);
        
        const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
        setSize(SCREEN_WIDTH - 50, 30);
        _loadingText->setWidth(SCREEN_WIDTH - 50 - 265);
        relayout();
    }
    
    void setOnClick(const std::function<void()> &onClick)
    {
        _objectName->setOnClickFunction(onClick);
    }
    
    void draw() override
    {
        drawAll();
    }
    
    void drawContainer() override {}
    
    void setPosition(const int x, const int y) override
    {
        GUIComponent::setPosition(x, y);
        _objectName->setPosition(x, y);
        _loadingText->setPosition(x + 265, y + (getHeight() - _loadingText->getHeight()) / 2);
    }
    
    bool notifyMouseClicked(const Ego::Events::MouseClickedEventArgs& e) override
    {
        return ComponentContainer::notifyMouseClicked(e);
    }
    
    bool notifyMouseMoved(const Ego::Events::MouseMovedEventArgs& e) override
    {
        return ComponentContainer::notifyMouseMoved(e);
    }
    
    void relayout() {
        int height = std::max(_objectName->getHeight(), _loadingText->getHeight());
        setHeight(height);
        _loadingText->setY(getY() + (height - _loadingText->getHeight()) / 2);
        auto scrollList = dynamic_cast<ScrollableList *>(getParent());
        if (!scrollList) return;
        scrollList->forceUpdate();
    }
    
    std::string _objectPath;
    std::shared_ptr<Button> _objectName;
    std::shared_ptr<DebugObjectLoadingState::GrowableLabel> _loadingText;
    std::weak_ptr<ModuleLoader> _module;
};

void DebugObjectLoadingState::GrowableLabel::doParentRelayout() {
    auto myGUIContainer = dynamic_cast<ObjectGUIContainer *>(getParent());
    if (!myGUIContainer) return;
    myGUIContainer->relayout();
}


struct DebugObjectLoadingState::ModuleLoader : public std::enable_shared_from_this<ModuleLoader> {
    ModuleLoader(const std::string &moduleName) :
    ModuleLoader(moduleName, "/modules/" + moduleName + "/objects")
    {}
    
protected:
    ModuleLoader(const std::string &moduleName, const std::string &objectLocationPath) :
    _moduleName(moduleName),
    _objectLocationPath(objectLocationPath)
    {}
    
public:
    void loadObjectList() {
        auto self = shared_from_this();
        setupPaths();
        vfs_search_context_t *context = vfs_findFirst(_objectLocationPath.c_str(), "obj", VFS_SEARCH_DIR | VFS_SEARCH_BARE);
        while (context) {
            std::string objName = vfs_search_context_get_current(context);
            _objects.emplace_back(new ObjectGUIContainer(objName, self));
            vfs_findNext(&context);
        }
        vfs_findClose(&context);
    }
    
    void setupPaths() {
        setup_init_module_vfs_paths(_moduleName.c_str());
    }
    
    std::vector<std::shared_ptr<ObjectGUIContainer>> getObjectList() {
        return _objects;
    }
    
    virtual std::string getModuleName() {
        return _moduleName;
    }
    
private:
    std::string _moduleName;
    std::string _objectLocationPath;
    std::vector<std::shared_ptr<ObjectGUIContainer>> _objects;
};

struct DebugObjectLoadingState::GlobalLoader : public ModuleLoader {
    GlobalLoader() :
    ModuleLoader("---invalid---", "/mp_objects")
    {}
    
    std::string getModuleName() override {
        return "~ GLOBAL ~";
    }
};

DebugObjectLoadingState::DebugObjectLoadingState() :
#ifdef _MSC_VER
_finishedLoading({0}),
#else
_finishedLoading(false),
#endif
_loadingThread(),
_scrollableList(),
_moduleList(),
_toLoad(),
_currentLoader()
{
    const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();
    
    _scrollableList = std::make_shared<ScrollableList>();
    _scrollableList->setPosition(8, 8);
    _scrollableList->setSize(SCREEN_WIDTH - 16, SCREEN_HEIGHT - 56);
    
    _moduleList.emplace_back(new GlobalLoader());
    
    vfs_search_context_t *context = vfs_findFirst("/modules", "mod", VFS_SEARCH_DIR | VFS_SEARCH_BARE);
    
    while (context)
    {
        std::string moduleName = vfs_search_context_get_current(context);
        auto module = std::make_shared<ModuleLoader>(moduleName);
        _moduleList.emplace_back(module);
        vfs_findNext(&context);
    }
    vfs_findClose(&context);
    
    for (const auto &loader : _moduleList) {
        auto button = std::make_shared<Button>(loader->getModuleName());
        std::weak_ptr<ModuleLoader> loaderPtr = loader;
        button->setWidth(SCREEN_WIDTH - 72);
        button->setOnClickFunction([this, loaderPtr] { addToQueue(loaderPtr.lock()); });
        _scrollableList->addComponent(button);
        
        _currentLoader = loader;
        loader->loadObjectList();
        
        for (const auto &object : loader->getObjectList()) {
            std::weak_ptr<ObjectGUIContainer> objectPtr = object;
            object->setOnClick([this, objectPtr] { addToQueue(objectPtr.lock()); });
            _scrollableList->addComponent(object);
        }
    }
    _scrollableList->forceUpdate();
    addComponent(_scrollableList);
    
    std::shared_ptr<Button> back = std::make_shared<Button>("Back");
    back->setPosition(8, SCREEN_HEIGHT - 30 - 8);
    back->setSize(150, 30);
    back->setOnClickFunction([this] { endState(); });
    addComponent(back);
    
    std::shared_ptr<Button> loadAll = std::make_shared<Button>("Load All");
    loadAll->setPosition(SCREEN_WIDTH - 150 - 8, SCREEN_HEIGHT - 30 - 8);
    loadAll->setSize(150, 30);
    loadAll->setOnClickFunction([this] { for (const auto &a : _moduleList) addToQueue(a); });
    addComponent(loadAll);
}

DebugObjectLoadingState::~DebugObjectLoadingState()
{
    //Wait until thread is dead
    if(_loadingThread.joinable()) {
        _loadingThread.join();
    }
}

void DebugObjectLoadingState::addToQueue(const std::shared_ptr<ModuleLoader> &toAdd)
{
    if (!toAdd) return;
    for (const auto &obj : toAdd->getObjectList()) {
        addToQueue(obj);
    }
}

void DebugObjectLoadingState::addToQueue(const std::shared_ptr<ObjectGUIContainer> &toAdd) {
    if (!toAdd) return;
    if (std::find(_toLoad.begin(), _toLoad.end(), toAdd) != _toLoad.end()) return;
    _toLoad.emplace_back(toAdd);
    toAdd->_loadingText->setText("Waiting...");
}

//TODO: HACK (no multithreading yet)
void DebugObjectLoadingState::singleThreadRedrawHack(const std::string &loadingText)
{
    // clear the screen
    gfx_request_clear_screen();
    gfx_do_clear_screen();
    
    _toLoad.front()->_loadingText->setText(loadingText);
    
    drawAll();
    
    // flip the graphics page
    gfx_request_flip_pages();
    gfx_do_flip_pages();
    SDL_PumpEvents();
}
//TODO: HACK END


void DebugObjectLoadingState::update()
{
    if (!_toLoad.empty()) loadObjectData();
}

void DebugObjectLoadingState::drawContainer()
{
    
}

void DebugObjectLoadingState::beginState()
{
    //Start the background loading thread
    //_loadingThread = std::thread(&LoadingState2::loadModuleData, this);
    AudioSystem::get().playMusic(27); //TODO: needs to be referenced by string
}

void DebugObjectLoadingState::loadObjectData()
{
    auto objectModule = _toLoad.front()->_module.lock();
    if (objectModule != _currentLoader) {
        _currentLoader = objectModule;
        _currentLoader->setupPaths();
    }
    
    std::string objectPath = _toLoad.front()->_objectPath;
    try
    {
        singleThreadRedrawHack("Loading...");
        
        PRO_REF ref = ProfileSystem::get().loadOneProfile(objectPath, 0);
        bool isValid = ProfileSystem::get().isValidProfileID(ref);
        ProfileSystem::get().reset();
        if (!isValid)
            throw std::string("Invalid profile ref returned, check log");
        
        //Complete!
        singleThreadRedrawHack("Finished!");
    }
    catch (Ego::Core::Exception &ex)
    {
        std::string out = std::string("Ego::Exception: ") + std::string(ex);
        singleThreadRedrawHack(out);
        Log::get().warn("error loading %s... %s\n", objectPath.c_str(), out.c_str());
    }
    catch (std::exception &ex)
    {
        std::string out = std::string("std::exception: ") + ex.what();
        singleThreadRedrawHack(out);
        Log::get().warn("error loading %s... %s\n", objectPath.c_str(), out.c_str());
    }
    catch (std::string &ex)
    {
        std::string out = std::string("std::string: ") + ex;
        singleThreadRedrawHack(out);
        Log::get().warn("error loading %s... %s\n", objectPath.c_str(), out.c_str());
    }
    catch (char *ex)
    {
        std::string out = std::string("C string: ") + ex;
        singleThreadRedrawHack(out);
        Log::get().warn("error loading %s... %s\n", objectPath.c_str(), out.c_str());
    }
    catch (...)
    {
        std::string out = "unknown error";
        singleThreadRedrawHack(out);
        Log::get().warn("error loading %s... %s\n", objectPath.c_str(), out.c_str());
    }
    _toLoad.pop_front();
}
