cmake_minimum_required(VERSION 3.16)
project(touhou8 C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Upstream game logic (YomotsuHisami/th08, th08_web/cpp/game + platform), the
# shared GLES renderer, and the Switch host in sdl/ (upstream's SDL host with
# native replacements for the browser pieces).
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/game/*.cpp)
list(REMOVE_ITEM GAME_SOURCES ${CMAKE_SOURCE_DIR}/game/RuntimeExports.cpp)
set(PLATFORM_SOURCES
    platform/BrowserRuntime.cpp
    platform/GameAudioManager.cpp
    platform/ResourceManager.cpp
)
set(HOST_SOURCES
    sdl/Platform.cpp
    sdl/FileHost.cpp
    sdl/GraphicsHost.cpp
    sdl/AudioHost.cpp
    sdl/BgmStream.cpp
    sdl/FontHost.cpp
    portable/sdl/Renderer.cpp
)

# CP932 -> UTF-16 map for Japanese text (same derivation as upstream).
set(CP932_TABLE ${CMAKE_BINARY_DIR}/generated/cp932_table.inc)
add_custom_command(OUTPUT ${CP932_TABLE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/generated
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/gen_cp932.py ${CP932_TABLE}
    DEPENDS ${CMAKE_SOURCE_DIR}/scripts/gen_cp932.py
    COMMENT "Generating CP932 table" VERBATIM)

# Berkeley SoftFloat: the x87-exact arithmetic the game logic relies on,
# built with upstream's flags.
add_library(th08_softfloat OBJECT third_party/softfloat.c)
target_compile_options(th08_softfloat PRIVATE -O2 -ffp-contract=off -fno-strict-aliasing)
target_compile_definitions(th08_softfloat PRIVATE SOFTFLOAT_FAST_INT64 INLINE_LEVEL=5)

# Game + host objects, shared by the game and the host tests.
add_library(th08_core OBJECT ${GAME_SOURCES} ${PLATFORM_SOURCES} ${HOST_SOURCES} ${CP932_TABLE})
target_include_directories(th08_core PUBLIC sdl portable/sdl ${CMAKE_BINARY_DIR}/generated)

# Same code-generation contract as the upstream web build: no FMA
# contraction (bit-exact float order with the original), no strict aliasing,
# no exceptions/RTTI. GCC defaults to -ffp-contract=fast on AArch64, so this
# flag is load-bearing for replay/RNG determinism.
target_compile_options(th08_core PUBLIC
    -O2 -ffp-contract=off -fno-strict-aliasing
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-class-memaccess> $<$<COMPILE_LANGUAGE:CXX>:-Wno-invalid-offsetof> -Wno-misleading-indentation -Wno-parentheses)
target_compile_definitions(th08_core PUBLIC TH_SDL3=1 TH_NATIVE_PLATFORM=1)

add_executable(touhou8 sdl/main.cpp)
target_link_libraries(touhou8 PRIVATE th08_core th08_softfloat)

if(NINTENDO_SWITCH)
    # ---- Nintendo Switch (devkitA64; configure with the devkitPro Switch
    # toolchain file, which defines NINTENDO_SWITCH) ----
    # Layout/naming follows the Saekaze Touhou ports: touhou8.nro +
    # sdmc:/switch/th08/ (th08.dat, thbgm.dat, msgothic.ttc).
    if(NOT DEFINED ENV{DEVKITPRO})
        set(ENV{DEVKITPRO} "/opt/devkitpro")
    endif()
    set(TH_DEVKITPRO "$ENV{DEVKITPRO}")
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/platform/switch/icon.jpg")
        message(FATAL_ERROR "platform/switch/icon.jpg (256x256 JPEG) not found.")
    endif()
    foreach(target th08_core th08_softfloat)
        target_compile_definitions(${target} PUBLIC __SWITCH__)
    endforeach()
    target_include_directories(th08_core PUBLIC
        ${TH_DEVKITPRO}/portlibs/switch/include
        ${TH_DEVKITPRO}/portlibs/switch/include/SDL2
        ${TH_DEVKITPRO}/portlibs/switch/include/freetype2
        ${TH_DEVKITPRO}/libnx/include)
    target_link_directories(touhou8 PRIVATE
        ${TH_DEVKITPRO}/portlibs/switch/lib
        ${TH_DEVKITPRO}/libnx/lib)
    # Static link order matters: dependents first, dependencies after.
    target_link_libraries(touhou8 PRIVATE
        SDL2_ttf SDL2 EGL glapi drm_nouveau GLESv2 freetype harfbuzz png bz2 z nx m)
    set_target_properties(touhou8 PROPERTIES OUTPUT_NAME "touhou8")

    # Default (all languages): romanised title. The original Japanese title is
    # patched into the Japanese slot only - same as saekaze/th07 and th10.
    nx_generate_nacp(OUTPUT "${CMAKE_BINARY_DIR}/touhou8-base.nacp"
        NAME "Touhou 8: Imperishable Night"
        AUTHOR "ZUN / Team Shanghai Alice"
        VERSION "1.00d-r4")
    add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/touhou8.nacp"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/nacp_lang.py"
                "${CMAKE_BINARY_DIR}/touhou8-base.nacp" "${CMAKE_BINARY_DIR}/touhou8.nacp"
                --lang Japanese
                --name "東方永夜抄　～ Imperishable Night"
                --author "上海アリス幻樂団"
                --fill-empty-name "Touhou 8: Imperishable Night"
                --fill-empty-author "ZUN / Team Shanghai Alice"
        DEPENDS "${CMAKE_BINARY_DIR}/touhou8-base.nacp"
                "${CMAKE_SOURCE_DIR}/scripts/nacp_lang.py"
        COMMENT "Patching Japanese title into NACP"
        VERBATIM)
    nx_create_nro(touhou8
        ICON "${CMAKE_SOURCE_DIR}/platform/switch/icon.jpg"
        NACP "${CMAKE_BINARY_DIR}/touhou8.nacp")
else()
    # ---- Linux test build (same sources, SDL2 + GLES3 desktop) ----
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(FT REQUIRED freetype2 SDL2_ttf)
    target_include_directories(th08_core PUBLIC ${SDL2_INCLUDE_DIRS} ${FT_INCLUDE_DIRS})
    target_link_libraries(touhou8 PRIVATE ${SDL2_LIBRARIES} ${FT_LIBRARIES} GLESv2 m)

    # Host tests: ./build/th08_host_tests (run under xvfb-run for the renderer)
    add_executable(th08_host_tests tests/test_host.cpp)
    target_link_libraries(th08_host_tests PRIVATE th08_core th08_softfloat ${SDL2_LIBRARIES} ${FT_LIBRARIES} GLESv2 m)
endif()
