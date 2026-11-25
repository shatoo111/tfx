cmake_minimum_required(VERSION 3.24)
project(TelegramEffects VERSION 0.5.24 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(TE_BUILD_APP "Build the Qt desktop application" ON)
option(TE_SANITIZE "Enable address and undefined behaviour sanitizers" OFF)
option(TE_GUI_TESTS "Run tests requiring an actual desktop/GPU" OFF)
include(CTest)

add_library(effects_core STATIC
    src/effects/SnowEffect.cpp src/effects/SnowField.cpp
    src/effects/SeasonalEffect.cpp src/effects/RainEffect.cpp src/effects/SakuraEffect.cpp src/effects/EffectFactory.cpp
    src/effects/RetroEffect.cpp src/effects/PixelCanvas.cpp src/effects/VectorCanvas.cpp src/effects/FestiveDecor.cpp
    src/effects/BlackHoleEffect.cpp src/effects/CatEffect.cpp src/effects/AquariumEffect.cpp src/effects/ParadeEffect.cpp)
target_include_directories(effects_core PUBLIC src)
if(MSVC)
    target_compile_options(effects_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(effects_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(TE_SANITIZE AND NOT MSVC)
    target_compile_options(effects_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(effects_core PUBLIC -fsanitize=address,undefined)
endif()

if(BUILD_TESTING)
    add_executable(effects_tests tests/effects_tests.cpp)
    target_link_libraries(effects_tests PRIVATE effects_core)
    add_test(NAME effects_core COMMAND effects_tests)
    add_executable(retro_tests tests/retro_tests.cpp)
    target_link_libraries(retro_tests PRIVATE effects_core)
    add_test(NAME retro_core COMMAND retro_tests)
    add_executable(scene_tests tests/scene_tests.cpp)
    target_link_libraries(scene_tests PRIVATE effects_core)
    add_test(NAME scene_core COMMAND scene_tests)
    add_executable(rain_tests tests/rain_tests.cpp)
    target_link_libraries(rain_tests PRIVATE effects_core)
    add_test(NAME rain_core COMMAND rain_tests)
    add_executable(sakura_tests tests/sakura_tests.cpp)
    target_link_libraries(sakura_tests PRIVATE effects_core)
    add_test(NAME sakura_core COMMAND sakura_tests)
endif()

if(TE_BUILD_APP)
    if(NOT WIN32 AND NOT APPLE)
        message(FATAL_ERROR "The overlay requires Windows or macOS. Use -DTE_BUILD_APP=OFF for engine tests.")
    endif()
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Network Gui Widgets Quick QuickControls2 Qml Svg ShaderTools)
    qt_standard_project_setup(REQUIRES 6.8)
    add_library(theme_renderer STATIC
        src/renderer/IllustrationNode.cpp src/renderer/IllustratedPreview.cpp
        src/renderer/SakuraPreview.cpp src/renderer/SakuraPreview.h
        src/renderer/EffectItem.cpp src/renderer/EffectItem.h)
    target_link_libraries(theme_renderer PUBLIC effects_core Qt6::Quick Qt6::Svg)
    qt_add_shaders(theme_renderer tfx_shaders
        BATCHABLE
        PREFIX "/tfx/shaders"
        BASE "src/renderer/shaders"
        FILES src/renderer/shaders/lightning.vert src/renderer/shaders/lightning.frag
              src/renderer/shaders/backdrop.frag src/renderer/shaders/curtain.frag)
    set(theme_art
        assets/themes/beech.svg
        assets/themes/birch.svg
        assets/themes/bough.svg
        assets/themes/cherry-bloom.svg
        assets/themes/cherry-rose.svg
        assets/themes/cherry-bud.svg
        assets/themes/sakura-left.svg
        assets/themes/sakura-right.svg
        assets/themes/comet.svg
        assets/themes/glow.svg
        assets/themes/maple.svg
        assets/themes/plane.svg
        assets/themes/rays.svg)
    qt_add_executable(TelegramEffects WIN32 MACOSX_BUNDLE
        src/main.cpp
        src/core/ReleaseUpdater.cpp src/core/ReleaseUpdater.h
        src/core/ReleaseNotice.cpp src/core/ReleaseNotice.h src/core/TelegramClients.h
        src/core/AppController.cpp src/core/AppController.h
        src/core/OverlayController.cpp src/core/OverlayController.h
        src/core/WindowTracker.cpp src/core/WindowTracker.h
        src/platform/NativePlatform.h
        src/settings/Settings.cpp src/settings/Settings.h)
    set(TE_VERSION_SUFFIX "" CACHE STRING "Suffix for a prerelease")
    if("$ENV{TFX_PREVIEW_BUILD}" STREQUAL "true")
        if("$ENV{GITHUB_REF_NAME}" MATCHES "^v[0-9.]+(-rc[.][0-9]+)$")
            set(TE_VERSION_SUFFIX "${CMAKE_MATCH_1}")
        else()
            set(TE_VERSION_SUFFIX "-preview")
        endif()
    endif()
    target_compile_definitions(TelegramEffects PRIVATE TE_APP_VERSION="${PROJECT_VERSION}${TE_VERSION_SUFFIX}")
    target_link_libraries(TelegramEffects PRIVATE theme_renderer
        Qt6::Core Qt6::Network Qt6::Gui Qt6::Widgets Qt6::Quick Qt6::QuickControls2 Qt6::Qml Qt6::Svg)
    qt_add_qml_module(TelegramEffects
        URI TelegramEffects VERSION 1.0
        QML_FILES src/ui/Main.qml src/ui/SettingRow.qml src/ui/WeatherPreview.qml src/ui/TfxColors.qml
            src/ui/PixelFrame.qml src/ui/PixelCard.qml src/ui/PixelButton.qml src/ui/PixelSwitch.qml src/ui/PixelSlider.qml
            src/ui/PixelPicker.qml src/ui/PixelSparks.qml src/ui/PixelCurtain.qml src/ui/HeroSprite.qml src/ui/CoinSprite.qml
            src/ui/LedgeWalker.qml src/ui/TypedText.qml src/ui/PixelLand.qml src/ui/PixelWorld.qml src/ui/ThemeGrid.qml
            src/ui/WeatherPixels.js src/ui/LandData.js)
    # Painted by tfx-site/vercel/scripts/export-app-art.mjs with the code that draws the landing page.
    file(GLOB pixel_art RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS assets/pixel/*.png)
    set(ui_assets assets/tfx-icon.svg ${pixel_art}
        assets/fonts/TFXPixel-Regular.ttf assets/fonts/JetBrainsMono-Regular.ttf assets/fonts/JetBrainsMono-Bold.ttf
        assets/fonts/OFL-press-start-2p.txt assets/fonts/OFL-jetbrains-mono.txt)
    qt_add_resources(TelegramEffects assets PREFIX "/" FILES ${theme_art} ${ui_assets})

    if(WIN32)
        set_target_properties(TelegramEffects PROPERTIES OUTPUT_NAME tfx)
        target_sources(TelegramEffects PRIVATE
            src/platform/windows/WindowsPlatform.cpp packaging/windows/tfx-icon.rc)
        if(MSVC)
            target_sources(TelegramEffects PRIVATE packaging/windows/app.manifest)
        else()
            target_sources(TelegramEffects PRIVATE packaging/windows/app.rc)
            target_include_directories(TelegramEffects PRIVATE packaging/windows)
        endif()
        target_compile_definitions(TelegramEffects PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
        target_link_libraries(TelegramEffects PRIVATE dwmapi user32 advapi32 ole32 uuid)
    elseif(APPLE)
        add_executable(tfx-swap packaging/macos/swap-bundles.cpp)
        install(TARGETS tfx-swap RUNTIME DESTINATION TFX.app/Contents/Helpers)
        enable_language(OBJCXX)
        set(CMAKE_OBJCXX_STANDARD 20)
        set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
        target_sources(TelegramEffects PRIVATE src/platform/macos/MacPlatform.mm)
        target_sources(TelegramEffects PRIVATE assets/tfx-icon.icns packaging/macos/apply-update.sh)
        set_source_files_properties(packaging/macos/apply-update.sh PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        set_source_files_properties(assets/tfx-icon.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        set_source_files_properties(src/platform/macos/MacPlatform.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
        target_link_libraries(TelegramEffects PRIVATE "-framework Cocoa" "-framework CoreGraphics")
        set_target_properties(TelegramEffects PROPERTIES
            OUTPUT_NAME TFX
            OBJCXX_STANDARD 20 OBJCXX_STANDARD_REQUIRED YES OBJCXX_EXTENSIONS NO
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist.in"
            MACOSX_BUNDLE_GUI_IDENTIFIER "org.telegrameffects.desktop"
            MACOSX_BUNDLE_BUNDLE_NAME TFX
            MACOSX_BUNDLE_ICON_FILE tfx-icon.icns
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
            MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
    endif()
    if(BUILD_TESTING)
        find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
        qt_add_executable(update_tests tests/update_tests.cpp src/core/ReleaseUpdater.cpp src/core/ReleaseUpdater.h
            src/core/ReleaseNotice.cpp src/core/ReleaseNotice.h)
        target_include_directories(update_tests PRIVATE src)
        target_link_libraries(update_tests PRIVATE Qt6::Core Qt6::Network Qt6::Test)
        add_test(NAME release_lookup COMMAND update_tests)
        qt_add_executable(settings_tests tests/settings_tests.cpp src/settings/Settings.cpp src/settings/Settings.h)
        target_include_directories(settings_tests PRIVATE src)
        target_link_libraries(settings_tests PRIVATE effects_core Qt6::Core Qt6::Test)
        add_test(NAME settings COMMAND settings_tests)
        qt_add_executable(ui_tests tests/ui_tests.cpp src/settings/Settings.cpp src/settings/Settings.h)
        target_include_directories(ui_tests PRIVATE src)
        target_compile_definitions(ui_tests PRIVATE TE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        target_link_libraries(ui_tests PRIVATE theme_renderer Qt6::Test Qt6::Quick Qt6::QuickControls2 Qt6::Widgets Qt6::Svg)
        qt_add_resources(ui_tests ui_assets PREFIX "/" FILES ${theme_art} ${ui_assets})
        add_test(NAME qml_ui COMMAND ui_tests)
        set_tests_properties(qml_ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
        if(WIN32)
            # The offscreen platform needs an explicit font directory for layout metrics.
            set_property(TEST qml_ui APPEND PROPERTY ENVIRONMENT "QT_QPA_FONTDIR=$ENV{SystemRoot}/Fonts")
        endif()
        qt_add_executable(renderer_tests tests/renderer_tests.cpp
            src/core/OverlayController.cpp src/core/OverlayController.h
            src/core/WindowTracker.cpp src/core/WindowTracker.h)
        qt_add_resources(renderer_tests renderer_art PREFIX "/" FILES ${theme_art})
        target_include_directories(renderer_tests PRIVATE src)
        target_link_libraries(renderer_tests PRIVATE theme_renderer Qt6::Quick Qt6::Test)
        if(WIN32)
            target_sources(renderer_tests PRIVATE src/platform/windows/WindowsPlatform.cpp)
            target_compile_definitions(renderer_tests PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
            target_link_libraries(renderer_tests PRIVATE dwmapi user32 advapi32 ole32 uuid)
        elseif(APPLE)
            target_sources(renderer_tests PRIVATE src/platform/macos/MacPlatform.mm)
            set_target_properties(renderer_tests PROPERTIES OBJCXX_STANDARD 20 OBJCXX_STANDARD_REQUIRED YES)
            target_link_libraries(renderer_tests PRIVATE "-framework Cocoa" "-framework CoreGraphics")
        endif()
        if(TE_GUI_TESTS)
            add_test(NAME accelerated_renderer COMMAND renderer_tests)
        endif()
    endif()
    install(TARGETS TelegramEffects BUNDLE DESTINATION . RUNTIME DESTINATION .)
    qt_generate_deploy_qml_app_script(TARGET TelegramEffects OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR)
    install(SCRIPT ${deploy_script})
    set(CPACK_PACKAGE_NAME "TelegramEffects")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_VENDOR "Telegram Effects contributors")
    if(APPLE)
        set(CPACK_GENERATOR "DragNDrop")
    else()
        set(CPACK_GENERATOR "ZIP")
    endif()
    include(CPack)
endif()
