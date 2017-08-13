cmake_minimum_required(VERSION 3.8)
project(egoboo CXX)

#Enable ccache if it is available
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

#Set compile flags
set(CMAKE_CXX_STANDARD 17)
add_definitions("-std=c++17 -Wall -Wextra -ftemplate-depth=1024")
#add_definitions("-O3 -flto -fuse-ld=gold")
add_definitions(-ggdb3 -Og)

#Disable some warnings
add_definitions("-Wno-reorder -Wno-sign-compare -Wno-missing-braces -Wno-unused-parameter ")

#Include directory locations
INCLUDE_DIRECTORIES(
        game/src
        egolib/src
        idlib/src)

#Source files
set(EGOLIB_SOURCE_FILES
        #egolib/tests/egolib/Tests/Math/Closure.cpp
        #egolib/tests/egolib/Tests/Math/Constants.cpp
        #egolib/tests/egolib/Tests/Math/Contains.cpp
        #egolib/tests/egolib/Tests/Math/Interpolate_Linear.cpp
        #egolib/tests/egolib/Tests/Math/Intersects.cpp
        #egolib/tests/egolib/Tests/Math/MatrixMath.cpp
        #egolib/tests/egolib/Tests/Math/PointMath.cpp
        #egolib/tests/egolib/Tests/Math/Translate.cpp
        #egolib/tests/egolib/Tests/Math/VectorMath.cpp
        #egolib/tests/egolib/Tests/Compilation.cpp
        #egolib/tests/egolib/Tests/MeshInfoIterator.cpp
        #egolib/tests/egolib/Tests/QuadTree.cpp
        #egolib/tests/egolib/Tests/Singleton.cpp
        #egolib/tests/egolib/Tests/StringUtilities.cpp
        )

file(GLOB IDLIB_SOURCE_FILES idlib/src/idlib/*.cpp)
file(GLOB EGOLIB_SOURCE_FILES egolib/src/egolib/*.cpp egolib/src/egolib/*.c)

set(EGOBOO_SOURCE_FILES
        ${IDLIB_SOURCE_FILES}
        ${EGOLIB_SOURCE_FILES}

        game/src/game/Core/GameEngine.cpp
        game/src/game/Entities/Common.cpp
        game/src/game/Entities/Enchant.cpp
        game/src/game/Entities/Object.cpp
        game/src/game/Entities/ObjectHandler.cpp
        game/src/game/Entities/Particle.cpp
        game/src/game/Entities/ParticleHandler.cpp
        game/src/game/GameStates/AudioOptionsScreen.cpp
        game/src/game/GameStates/DebugFontRenderingState.cpp
        game/src/game/GameStates/DebugMainMenuState.cpp
        game/src/game/GameStates/DebugModuleLoadingState.cpp
        game/src/game/GameStates/DebugObjectLoadingState.cpp
        game/src/game/GameStates/DebugParticlesScreen.cpp
        game/src/game/GameStates/GameState.cpp
        game/src/game/GameStates/InGameMenuState.cpp
        game/src/game/GameStates/InputOptionsScreen.cpp
        game/src/game/GameStates/LoadingState.cpp
        game/src/game/GameStates/LoadPlayerElement.cpp
        game/src/game/GameStates/MainMenuState.cpp
        game/src/game/GameStates/MapEditorSelectModuleState.cpp
        game/src/game/GameStates/MapEditorState.cpp
        game/src/game/GameStates/OptionsScreen.cpp
        game/src/game/GameStates/PlayingState.cpp
        game/src/game/GameStates/SelectCharacterState.cpp
        game/src/game/GameStates/SelectModuleState.cpp
        game/src/game/GameStates/SelectPlayersState.cpp
        game/src/game/GameStates/VictoryScreen.cpp
        game/src/game/GameStates/VideoOptionsScreen.cpp
        game/src/game/Graphics/RenderPasses/BackgroundRenderPass.cpp
        game/src/game/Graphics/RenderPasses/EntityReflectionsRenderPass.cpp
        game/src/game/Graphics/RenderPasses/EntityShadowsRenderPass.cpp
        game/src/game/Graphics/RenderPasses/ForegroundRenderPass.cpp
        game/src/game/Graphics/RenderPasses/HeightmapRenderPass.cpp
        game/src/game/Graphics/RenderPasses/NonOpaqueEntitiesRenderPass.cpp
        game/src/game/Graphics/RenderPasses/NonReflectiveTilesRenderPass.cpp
        game/src/game/Graphics/RenderPasses/OpaqueEntitiesRenderPass.cpp
        game/src/game/Graphics/RenderPasses/ReflectiveTilesFirstRenderPass.cpp
        game/src/game/Graphics/RenderPasses/ReflectiveTilesSecondRenderPass.cpp
        game/src/game/Graphics/RenderPasses/WaterTilesRenderPass.cpp
        game/src/game/Graphics/Billboard.cpp
        game/src/game/Graphics/BillboardSystem.cpp
        game/src/game/Graphics/Camera.cpp
        game/src/game/Graphics/CameraSystem.cpp
        game/src/game/Graphics/DefaultMd2ModelRenderer.cpp
        game/src/game/Graphics/EntityList.cpp
        game/src/game/Graphics/Md2ModelRenderer.cpp
        game/src/game/Graphics/ObjectGraphics.cpp
        game/src/game/Graphics/ParticleGraphics.cpp
        game/src/game/Graphics/RenderPass.cpp
        game/src/game/Graphics/RenderPasses.cpp
        game/src/game/Graphics/TextureAtlasManager.cpp
        game/src/game/Graphics/TileList.cpp
        game/src/game/GUI/Button.cpp
        game/src/game/GUI/CharacterStatus.cpp
        game/src/game/GUI/CharacterWindow.cpp
        game/src/game/GUI/Component.cpp
        game/src/game/GUI/Container.cpp
        game/src/game/GUI/DrawingContext.cpp
        game/src/game/GUI/IconButton.cpp
        game/src/game/GUI/Image.cpp
        game/src/game/GUI/InputListener.cpp
        game/src/game/GUI/InternalDebugWindow.cpp
        game/src/game/GUI/InternalWindow.cpp
        game/src/game/GUI/InventorySlot.cpp
        game/src/game/GUI/Label.cpp
        game/src/game/GUI/Layout.cpp
        game/src/game/GUI/LevelUpWindow.cpp
        game/src/game/GUI/Material.cpp
        game/src/game/GUI/MessageLog.cpp
        game/src/game/GUI/MiniMap.cpp
        game/src/game/GUI/ModuleSelector.cpp
        game/src/game/GUI/OptionsButton.cpp
        game/src/game/GUI/Panel.cpp
        game/src/game/GUI/ProgressBar.cpp
        game/src/game/GUI/ScrollableList.cpp
        game/src/game/GUI/Slider.cpp
        game/src/game/GUI/TabPanel.cpp
        game/src/game/GUI/UIManager.cpp
        game/src/game/Logic/Player.cpp
        game/src/game/Logic/QuestLog.cpp
        #game/src/game/Lua/ego.i
        game/src/game/Module/AnimatedTiles.cpp
        game/src/game/Module/Fog.cpp
        game/src/game/Module/Module.cpp
        game/src/game/Module/module_spawn.c
        game/src/game/Module/Passage.cpp
        game/src/game/Module/Water.cpp
        game/src/game/Module/Weather.cpp
        game/src/game/Physics/CollisionSystem.cpp
        game/src/game/Physics/ObjectPhysics.cpp
        game/src/game/Physics/particle_collision.c
        game/src/game/Physics/ParticlePhysics.cpp
        game/src/game/CharacterMatrix.c
        game/src/game/egoboo.c
        game/src/game/game.c
        game/src/game/graphic.c
        game/src/game/graphic_fan.c
        game/src/game/graphic_mad.c
        game/src/game/graphic_prt.c
        game/src/game/Inventory.cpp
        game/src/game/lighting.c
        game/src/game/link.c
        game/src/game/mesh.c
        game/src/game/physics.c
        game/src/game/renderer_3d.c
        game/src/game/script_compile.c
        game/src/game/script_functions.c
        game/src/game/script_implementation.c
        game/src/game/script_variables.c
        game/src/game/Shop.cpp
        )

#OS specific files
if (UNIX)
    set(EGOBOO_SOURCE_FILES ${EGOBOO_SOURCE_FILES} unix/main.cpp)
endif (UNIX)

#Compile everything as C++
SET_SOURCE_FILES_PROPERTIES(${IDLIB_SOURCE_FILES} PROPERTIES LANGUAGE CXX )
SET_SOURCE_FILES_PROPERTIES(${EGOBLIB_SOURCE_FILES} PROPERTIES LANGUAGE CXX )
SET_SOURCE_FILES_PROPERTIES(${EGOBOO_SOURCE_FILES} PROPERTIES LANGUAGE CXX )

#Define compilation output
add_executable(egoboo ${EGOBOO_SOURCE_FILES})

#Include dependency on SDL
INCLUDE(FindPkgConfig)

PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
PKG_SEARCH_MODULE(SDL2_IMAGE REQUIRED SDL2_image>=2.0.0)
PKG_SEARCH_MODULE(SDL2_MIXER REQUIRED SDL2_mixer>=2.0.0)
PKG_SEARCH_MODULE(SDL2_TTF REQUIRED SDL2_ttf>=2.0.0)
#PKG_SEARCH_MODULE(PHYSFS REQUIRED PHYSFS>=2.0.0)
FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(PhysFS REQUIRED)

INCLUDE_DIRECTORIES(${PHYSFS_INCLUDE_DIR} ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS} ${SDL2_MIXER_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDL2_MIXER_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${OPENGL_gl_LIBRARY})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PHYSFS_LIBRARY})