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

/// @file egolib/game/GameStates/DebugObjectLoadingState.cpp
/// @details Debugging state where one can debug loading objects
///          from the global repository or individual modules.
/// @author Johan Jansen, penguinflyer5234

#include "egolib/game/GameStates/DebugObjectLoadingState.hpp"
#include "egolib/game/GameStates/PlayingState.hpp"
#include "egolib/game/GameStates/LoadPlayerElement.hpp"
#include "egolib/game/Core/GameEngine.hpp"
#include "egolib/egoboo_setup.h"
#include "egolib/game/graphic.h"
#include "egolib/game/GUI/Button.hpp"
#include "egolib/game/GUI/Label.hpp"
#include "egolib/game/GUI/Image.hpp"
#include "egolib/game/GUI/ScrollableList.hpp"
#include "egolib/Math/Random.hpp"
#include "egolib/Audio/AudioSystem.hpp"

//For loading stuff
#include "egolib/game/Graphics/CameraSystem.hpp"
#include "egolib/game/game.h"
#include "egolib/game/Graphics/Billboard.hpp"
#include "egolib/game/link.h"
#include "egolib/fileutil.h"

class DebugObjectLoadingState::GrowableLabel : public Ego::GUI::Component
{
public:
    GrowableLabel(const std::string &text) :
    _text(text),
    _font(_gameEngine->getUIManager()->getFont(Ego::GUI::UIManager::FONT_DEBUG)),
    _textRenderer()
    {
        redraw();
    }
    
    void draw(Ego::GUI::DrawingContext& drawingContext) override {
        _textRenderer->render(getX(), getY());
    }
    
    void setText(const std::string &text) {
        _text = text;
        redraw();
    }
    
    void setWidth(float width) override {
        Component::setWidth(width);
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

struct DebugObjectLoadingState::ObjectGUIContainer : public Container
{
    ObjectGUIContainer(const std::string &objectName,
                       const std::weak_ptr<ModuleLoader> &module) :
    _objectPath("/mp_objects/" + objectName),
    _objectName(std::make_shared<Ego::GUI::Button>(objectName)),
    _loadingText(std::make_shared<DebugObjectLoadingState::GrowableLabel>("Not Loaded")),
    _module(module)
    {
        _objectName->setSize(Vector2f(250, 30));
        
        addComponent(_objectName);
        addComponent(_loadingText);
        
        const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
        setSize(Vector2f(SCREEN_WIDTH - 50, 30));
        _loadingText->setWidth(SCREEN_WIDTH - 50 - 265);
        relayout();
    }
    
    void setOnClick(const std::function<void()> &onClick)
    {
        _objectName->setOnClickFunction(onClick);
    }
    
    void draw(Ego::GUI::DrawingContext& drawingContext) override
    {
        drawAll(drawingContext);
    }
    
    void drawContainer(Ego::GUI::DrawingContext& drawingContext) override {}
    
    void setPosition(const Point2f& position) override
    {
        Component::setPosition(position);
        _objectName->setPosition(position);
        _loadingText->setPosition(position + Vector2f(265, (getHeight() - _loadingText->getHeight()) / 2));
    }
        
    void relayout() {
        int height = std::max(_objectName->getHeight(), _loadingText->getHeight());
        setHeight(height);
        _loadingText->setY(getY() + (height - _loadingText->getHeight()) / 2);
        auto scrollList = dynamic_cast<Ego::GUI::ScrollableList *>(getParent());
        if (!scrollList) return;
        scrollList->forceUpdate();
    }
    
    std::string _objectPath;
    std::shared_ptr<Ego::GUI::Button> _objectName;
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
        SearchContext *ctxt = new SearchContext(Ego::VfsPath(_objectLocationPath), Ego::Extension("obj"), VFS_SEARCH_DIR | VFS_SEARCH_BARE);
        while (ctxt->hasData()) {
            auto objName = ctxt->getData();
            _objects.emplace_back(new ObjectGUIContainer(objName.string(), self));
            ctxt->nextData();
        }
        delete ctxt;
        ctxt = nullptr;
    }
    
    void setupPaths() {
        setup_init_module_vfs_paths(_moduleName);
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
    
    _scrollableList = std::make_shared<Ego::GUI::ScrollableList>();
    _scrollableList->setPosition(Point2f(8, 8));
    _scrollableList->setSize(Vector2f(SCREEN_WIDTH - 16, SCREEN_HEIGHT - 56));
    
    _moduleList.emplace_back(new GlobalLoader());
    
    SearchContext *context = new SearchContext(Ego::VfsPath("/modules"), Ego::Extension("mod"), VFS_SEARCH_DIR | VFS_SEARCH_BARE);
    
    while (context->hasData())
    {
        auto moduleName = context->getData();
        auto module = std::make_shared<ModuleLoader>(moduleName.string());
        _moduleList.emplace_back(module);
        context->nextData();
    }
    delete context;
    context = nullptr;
    
    for (const auto &loader : _moduleList) {
        auto button = std::make_shared<Ego::GUI::Button>(loader->getModuleName());
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
    
    auto back = std::make_shared<Ego::GUI::Button>("Back");
    back->setPosition(Point2f(8, SCREEN_HEIGHT - 30 - 8));
    back->setSize(Vector2f(150, 30));
    back->setOnClickFunction([this] { endState(); });
    addComponent(back);
    
    auto loadAll = std::make_shared<Ego::GUI::Button>("Load All");
    loadAll->setPosition(Point2f(SCREEN_WIDTH - 150 - 8, SCREEN_HEIGHT - 30 - 8));
    loadAll->setSize(Vector2f(150, 30));
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
    gfx_do_clear_screen();
    
    _toLoad.front()->_loadingText->setText(loadingText);
    
    Ego::GUI::DrawingContext drawingContext;
    drawAll(drawingContext);
    
    // flip the graphics page
    gfx_do_flip_pages();
    SDL_PumpEvents();
}
//TODO: HACK END


void DebugObjectLoadingState::update()
{
    if (!_toLoad.empty()) loadObjectData();
}

void DebugObjectLoadingState::drawContainer(Ego::GUI::DrawingContext& drawingContext)
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
        
        ObjectProfileRef ref = ProfileSystem::get().loadOneProfile(objectPath, 0);
        bool isValid = ProfileSystem::get().isLoaded(ref);
        ProfileSystem::get().reset();
        if (!isValid)
            throw std::string("Invalid profile ref returned, check log");
        
        //Complete!
        singleThreadRedrawHack("Finished!");
    }
    catch (idlib::exception &ex)
    {
        std::string out = std::string("Ego::Exception: ") + ex.to_string();
        singleThreadRedrawHack(out);
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "error loading ", objectPath,
                                         "... ", out, Log::EndOfEntry);
    }
    catch (std::exception &ex)
    {
        std::string out = std::string("std::exception: ") + ex.what();
        singleThreadRedrawHack(out);
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "error loading ", objectPath,
                                         "... ", out, Log::EndOfEntry);
    }
    catch (std::string &ex)
    {
        std::string out = std::string("std::string: ") + ex;
        singleThreadRedrawHack(out);
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "error loading ", objectPath,
                                         "... ", out, Log::EndOfEntry);
    }
    catch (char *ex)
    {
        std::string out = std::string("C string: ") + ex;
        singleThreadRedrawHack(out);
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "error loading ", objectPath,
                                         "... ", out, Log::EndOfEntry);
    }
    catch (...)
    {
        std::string out = "unknown error";
        singleThreadRedrawHack(out);
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "error loading ", objectPath,
                                         "... ", out, Log::EndOfEntry);
    }
    _toLoad.pop_front();
}
