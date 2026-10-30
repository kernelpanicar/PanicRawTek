cmake_minimum_required(VERSION 3.22)

# Mac: universal (Apple Silicon + Intel), macOS 11+. Tiene que ir antes de project().
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Mac architectures")
# Windows: runtime de C++ estático (no hace falta instalar Visual C++ Redistributable)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(PanicRawTek VERSION 3.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE se descarga solo. Para usar una copia local:
#   cmake -B build -DFETCHCONTENT_SOURCE_DIR_JUCE=/ruta/a/JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

set(PANIC_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PANIC_FORMATS AU)
endif()

juce_add_plugin(PanicRawTek
    COMPANY_NAME                "Panic Audio"
    BUNDLE_ID                   "com.panicaudio.panicrawtek"
    PRODUCT_NAME                "Panic Raw Tek"
    PLUGIN_NAME                 "Panic Raw Tek"
    PLUGIN_MANUFACTURER_CODE    Pnic
    PLUGIN_CODE                 Pnrt
    DESCRIPTION                 "Raw & groove techno synthesizer"
    ICON_BIG                    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icon/icon_1024.png"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Instrument Synth
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    FORMATS                     ${PANIC_FORMATS}
)

# Pack de presets y fuentes embebidos en el plugin
file(GLOB_RECURSE PANIC_PRESET_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Presets/*.rtkpreset")
juce_add_binary_data(PanicPresetData NAMESPACE PresetData HEADER_NAME PresetData.h SOURCES ${PANIC_PRESET_FILES})
juce_add_binary_data(PanicFontData NAMESPACE FontData HEADER_NAME FontData.h SOURCES
    Resources/Fonts/BebasNeue-Bold.otf
    Resources/Fonts/JetBrainsMono-Bold.ttf
    Resources/Fonts/JetBrainsMono-Medium.ttf)
set_target_properties(PanicPresetData PanicFontData PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_sources(PanicRawTek PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

if(MSVC)
    target_compile_options(PanicRawTek PRIVATE /utf-8 /bigobj)
endif()

target_compile_definitions(PanicRawTek PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

option(PANIC_BUILD_TESTS "Build offline render tools" OFF)
if(PANIC_BUILD_TESTS)
    juce_add_console_app(PanicRenderTest PRODUCT_NAME "PanicRenderTest")
    target_sources(PanicRenderTest PRIVATE Tests/RenderTest.cpp)
    target_compile_definitions(PanicRenderTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(PanicRenderTest PRIVATE PanicRawTek juce::juce_audio_formats)

    juce_add_console_app(PanicRenderMidi PRODUCT_NAME "PanicRenderMidi")
    target_sources(PanicRenderMidi PRIVATE Tests/RenderMidi.cpp)
    target_compile_definitions(PanicRenderMidi PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(PanicRenderMidi PRIVATE PanicRawTek juce::juce_audio_formats)
endif()

target_link_libraries(PanicRawTek
    PRIVATE
        PanicPresetData
        PanicFontData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
