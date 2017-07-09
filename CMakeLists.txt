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

set(IDLIB_SOURCE_FILES
        #idlib/tests/idlib/tests/color/addition_subtraction.cpp
        #idlib/tests/idlib/tests/color/brightening_darkening.cpp
        #idlib/tests/idlib/tests/color/compose_construction.cpp
        #idlib/tests/idlib/tests/color/copy_construction.cpp
        #idlib/tests/idlib/tests/color/decompose_construction.cpp
        #idlib/tests/idlib/tests/color/interpolation.cpp
        #idlib/tests/idlib/tests/color/inversion.cpp
        #idlib/tests/idlib/tests/parsing_expressions/choice.cpp
        #idlib/tests/idlib/tests/parsing_expressions/name.cpp
        #idlib/tests/idlib/tests/parsing_expressions/qualified_name.cpp
        #idlib/tests/idlib/tests/parsing_expressions/symbols.cpp
        #idlib/tests/idlib/tests/Compilation.cpp
        #idlib/tests/idlib/tests/Math.cpp
        #idlib/tests/idlib/tests/prefix.cpp
        #idlib/tests/idlib/tests/qualified_name.cpp
        #idlib/tests/idlib/tests/Signal.cpp
        #idlib/tests/idlib/tests/suffix.cpp
        #idlib/tests/idlib/tests/to_lower.cpp
        #idlib/tests/idlib/tests/to_upper.cpp
        )

set(EGOBOO_SOURCE_FILES
        idlib/src/idlib/color/instantiations.cpp
        idlib/src/idlib/language/location.cpp
        idlib/src/idlib/language/qualified_name.cpp
        idlib/src/idlib/math/instantiations.cpp
        idlib/src/idlib/signal/connection_base.cpp
        idlib/src/idlib/signal/node_base.cpp
        idlib/src/idlib/signal/signal_base.cpp
        idlib/src/idlib/utility/prefix.cpp
        idlib/src/idlib/utility/suffix.cpp
        idlib/src/idlib/utility/to_lower.cpp
        idlib/src/idlib/utility/to_upper.cpp
        idlib/src/idlib/idlib.cpp
        idlib/src/idlib/text_range.cpp

        egolib/src/egolib/AI/AStar.cpp
        egolib/src/egolib/AI/LineOfSight.cpp
        egolib/src/egolib/AI/WaypointList.c
        egolib/src/egolib/Audio/AudioSystem.cpp
        egolib/src/egolib/Console/Console.cpp
        egolib/src/egolib/Console/DefaultConsole.cpp
        egolib/src/egolib/Core/System.cpp
        egolib/src/egolib/Extensions/ogl_extensions.c
        egolib/src/egolib/FileFormats/ConfigFile/configfile.c
        egolib/src/egolib/FileFormats/ConfigFile/ConfigFileToken.cpp
        egolib/src/egolib/FileFormats/ConfigFile/ConfigFileTokenKind.cpp
        egolib/src/egolib/FileFormats/SpawnFile/spawn_file.c
        egolib/src/egolib/FileFormats/SpawnFile/SpawnFileReaderImpl.cpp
        egolib/src/egolib/FileFormats/SpawnFile/SpawnFileToken.cpp
        egolib/src/egolib/FileFormats/SpawnFile/SpawnFileTokenKind.cpp
        egolib/src/egolib/FileFormats/Globals.cpp
        egolib/src/egolib/FileFormats/id_md2.c
        egolib/src/egolib/FileFormats/map_file-v1.c
        egolib/src/egolib/FileFormats/map_file-v2.c
        egolib/src/egolib/FileFormats/map_file-v3.c
        egolib/src/egolib/FileFormats/map_file-v4.c
        egolib/src/egolib/FileFormats/map_file.c
        egolib/src/egolib/FileFormats/map_tile_dictionary.c
        egolib/src/egolib/FileFormats/MapTileDefinitionsDictionary.cpp
        egolib/src/egolib/FileFormats/template.c
        egolib/src/egolib/FileFormats/wawalite_file.c
        egolib/src/egolib/Graphics/SDL/Display.cpp
        egolib/src/egolib/Graphics/SDL/DisplayMode.cpp
        egolib/src/egolib/Graphics/SDL/GraphicsContext.cpp
        egolib/src/egolib/Graphics/SDL/GraphicsSystemNew.cpp
        egolib/src/egolib/Graphics/SDL/GraphicsWindow.cpp
        egolib/src/egolib/Graphics/SDL/Utilities.cpp
        egolib/src/egolib/Graphics/Buffer.cpp
        egolib/src/egolib/Graphics/ColourDepth.cpp
        egolib/src/egolib/Graphics/Display.cpp
        egolib/src/egolib/Graphics/DisplayMode.cpp
        egolib/src/egolib/Graphics/Font.cpp
        egolib/src/egolib/Graphics/FontManager.cpp
        egolib/src/egolib/Graphics/GraphicsContext.cpp
        egolib/src/egolib/Graphics/GraphicsSystem.cpp
        egolib/src/egolib/Graphics/GraphicsSystemNew.cpp
        egolib/src/egolib/Graphics/GraphicsWindow.cpp
        egolib/src/egolib/Graphics/IndexBuffer.cpp
        egolib/src/egolib/Graphics/IndexDescriptor.cpp
        egolib/src/egolib/Graphics/MD2Model.cpp
        egolib/src/egolib/Graphics/ModelDescriptor.cpp
        egolib/src/egolib/Graphics/PixelFormat.cpp
        egolib/src/egolib/Graphics/TextureManager.cpp
        egolib/src/egolib/Graphics/VertexBuffer.cpp
        egolib/src/egolib/Graphics/VertexDescriptor.cpp
        egolib/src/egolib/Graphics/VertexElementDescriptor.cpp
        egolib/src/egolib/Graphics/VertexFormat.cpp
        egolib/src/egolib/Graphics/Viewport.cpp
        egolib/src/egolib/Image/Image.cpp
        egolib/src/egolib/Image/ImageLoader.cpp
        egolib/src/egolib/Image/ImageLoader_SDL.cpp
        egolib/src/egolib/Image/ImageLoader_SDL_image.cpp
        egolib/src/egolib/Image/ImageManager.cpp
        egolib/src/egolib/Image/SDL_Image_Extensions.c
        egolib/src/egolib/InputControl/ControlSettingsFile.cpp
        egolib/src/egolib/InputControl/InputDevice.cpp
        egolib/src/egolib/InputControl/InputSystem.cpp
        egolib/src/egolib/Log/_Include.cpp
        egolib/src/egolib/Log/ConsoleColor.cpp
        egolib/src/egolib/Log/DefaultTarget.cpp
        egolib/src/egolib/Log/EndOfEntry.cpp
        egolib/src/egolib/Log/EndOfLine.cpp
        egolib/src/egolib/Log/Entry.cpp
        egolib/src/egolib/Log/Level.cpp
        egolib/src/egolib/Log/Target.cpp
        egolib/src/egolib/Logic/MissileTreatment.cpp
        egolib/src/egolib/Logic/Perk.cpp
        egolib/src/egolib/Logic/PerkHandler.cpp
        egolib/src/egolib/Logic/Team.cpp
        egolib/src/egolib/Logic/TreasureTables.cpp
        egolib/src/egolib/Math/Random.cpp
        egolib/src/egolib/Math/Standard.cpp
        egolib/src/egolib/Mesh/Info.cpp
        egolib/src/egolib/Mesh/TileFX.cpp
        egolib/src/egolib/Network/_Include.cpp
        egolib/src/egolib/Platform/file_linux.c
        #        egolib/src/egolib/Platform/file_win.c
        egolib/src/egolib/Profiles/EnchantProfile.cpp
        egolib/src/egolib/Profiles/EnchantProfileWriter.cpp
        egolib/src/egolib/Profiles/LocalParticleProfileRef.cpp
        egolib/src/egolib/Profiles/ModuleProfile.cpp
        egolib/src/egolib/Profiles/ObjectProfile.cpp
        egolib/src/egolib/Profiles/ParticleProfile.cpp
        egolib/src/egolib/Profiles/ParticleProfileWriter.cpp
        egolib/src/egolib/Profiles/ProfileSystem.cpp
        egolib/src/egolib/Profiles/RandomName.cpp
        egolib/src/egolib/Renderer/OpenGL/AccumulationBuffer.cpp
        egolib/src/egolib/Renderer/OpenGL/ColourBuffer.cpp
        egolib/src/egolib/Renderer/OpenGL/DefaultTexture.cpp
        egolib/src/egolib/Renderer/OpenGL/DepthBuffer.cpp
        egolib/src/egolib/Renderer/OpenGL/Renderer.cpp
        egolib/src/egolib/Renderer/OpenGL/RendererInfo.cpp
        egolib/src/egolib/Renderer/OpenGL/StencilBuffer.cpp
        egolib/src/egolib/Renderer/OpenGL/Texture.cpp
        egolib/src/egolib/Renderer/OpenGL/TextureUnit.cpp
        egolib/src/egolib/Renderer/OpenGL/Utilities.cpp
        egolib/src/egolib/Renderer/DeferredTexture.cpp
        egolib/src/egolib/Renderer/Renderer.cpp
        egolib/src/egolib/Renderer/RendererInfo.cpp
        egolib/src/egolib/Renderer/Texture.cpp
        egolib/src/egolib/Renderer/TextureSampler.cpp
        egolib/src/egolib/Script/Interpreter/Configuration.cpp
        egolib/src/egolib/Script/Interpreter/InvalidCastException.cpp
        egolib/src/egolib/Script/Interpreter/SafeCast.cpp
        egolib/src/egolib/Script/Interpreter/Tag.cpp
        egolib/src/egolib/Script/Interpreter/TaggedValue.cpp
        egolib/src/egolib/Script/Buffer.cpp
        egolib/src/egolib/Script/CLogEntry.cpp
        egolib/src/egolib/Script/Constant.cpp
        egolib/src/egolib/Script/ConstantPool.cpp
        egolib/src/egolib/Script/Conversion.cpp
        egolib/src/egolib/Script/DDLToken.cpp
        egolib/src/egolib/Script/DDLTokenDecoder.cpp
        egolib/src/egolib/Script/DDLTokenKind.cpp
        egolib/src/egolib/Script/Errors.cpp
        egolib/src/egolib/Script/IRuntimeStatistics.cpp
        egolib/src/egolib/Script/OpcodeInfo.cpp
        egolib/src/egolib/Script/PDLToken.cpp
        egolib/src/egolib/Script/PDLTokenKind.cpp
        egolib/src/egolib/Script/Scanner.cpp
        egolib/src/egolib/Script/script.c
        egolib/src/egolib/Script/TextFile.cpp
        egolib/src/egolib/Script/TextInputFile.cpp
        egolib/src/egolib/Time/LocalTime.cpp
        egolib/src/egolib/Time/Time.cpp
        egolib/src/egolib/VFS/FsPath.cpp
        egolib/src/egolib/VFS/VfsPath.cpp
        egolib/src/egolib/_math.c
        egolib/src/egolib/App.cpp
        egolib/src/egolib/bbox.c
        egolib/src/egolib/Clock.cpp
        egolib/src/egolib/Debug.cpp
        egolib/src/egolib/egoboo_setup.c
        egolib/src/egolib/endian.c
        egolib/src/egolib/file_common.c
        egolib/src/egolib/fileutil.c
        egolib/src/egolib/Float.cpp
        egolib/src/egolib/font_bmp.c
        egolib/src/egolib/frustum.c
        egolib/src/egolib/geometry.c
        egolib/src/egolib/IDSZ.cpp
        egolib/src/egolib/map_functions.c
        egolib/src/egolib/strutil.c
        egolib/src/egolib/typedef.c
        egolib/src/egolib/vfs.c

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
add_executable(egoboo ${IDLIB_SOURCE_FILES} ${EGOBLIB_SOURCE_FILES} ${EGOBOO_SOURCE_FILES})

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