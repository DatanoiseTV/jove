# -----------------------------------------------------------------------------
# Jove — analog-inspired polysynth instrument plugin (JUCE 8, AU/VST3/Standalone).
# -----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.22)

project(Jove VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Universal binary for distributable plugins; override with
# -DCMAKE_OSX_ARCHITECTURES=arm64 for fast local iteration.
if(APPLE)
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures")
    endif()
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
    endif()
endif()

# JUCE, vendored as a git submodule under external/JUCE.
add_subdirectory(external/JUCE)

# Local-install copy is on by default for developer iteration; CI sets
# -DJOVE_INSTALL_LOCAL=OFF so it doesn't write into ~/Library/Audio/Plug-Ins.
option(JOVE_INSTALL_LOCAL "Copy built plugins into the user's plug-in dirs" ON)

juce_add_plugin(Jove
    COMPANY_NAME             "DatanoiseTV"
    BUNDLE_ID                "com.datanoisetv.jove"
    PLUGIN_MANUFACTURER_CODE Dnse
    PLUGIN_CODE              Jove
    FORMATS                  AU VST3 Standalone
    PRODUCT_NAME             "Jove"
    DESCRIPTION              "Analog-inspired polysynth"
    IS_SYNTH                 TRUE
    NEEDS_MIDI_INPUT         TRUE
    NEEDS_MIDI_OUTPUT        FALSE
    IS_MIDI_EFFECT           FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE   # let the host keep keys (computer-MIDI); WebView focuses its own inputs on click
    NEEDS_WEB_BROWSER        TRUE
    COPY_PLUGIN_AFTER_BUILD  ${JOVE_INSTALL_LOCAL}
    VST3_CATEGORIES          Instrument Synth
    AU_MAIN_TYPE             kAudioUnitType_MusicDevice)

juce_generate_juce_header(Jove)

# UI assets bundled as BinaryData, served by the WebView resource provider.
file(GLOB JOVE_UI_SRC    CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ui/src/*")
file(GLOB JOVE_UI_VENDOR CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ui/vendor/*")
juce_add_binary_data(JoveUI
    SOURCES     ${JOVE_UI_SRC} ${JOVE_UI_VENDOR}
    NAMESPACE   JoveUIData
    HEADER_NAME JoveUIData.h)
set_target_properties(JoveUI PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_sources(Jove PRIVATE
    src/PluginProcessor.cpp
    src/ui/WebEditor.cpp
    src/ui/WebKeyboardFocus_mac.mm
    src/engine/SynthEngine.cpp
    src/engine/SynthPresets.cpp
    src/presets/PresetManager.cpp)

target_include_directories(Jove PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(Jove
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0
    PRIVATE
        JOVE_VERSION="${PROJECT_VERSION}")

target_link_libraries(Jove PRIVATE
    JoveUI
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# -----------------------------------------------------------------------------
# Ad-hoc re-sign the assembled bundles. JUCE/CMake on recent macOS can leave an
# inconsistent code signature ("code has no resources but signature indicates
# they must be present"), which Apple-Silicon hosts reject on load with a
# CODESIGNING "Invalid Page" SIGKILL — i.e. the DAW crashes when instantiating
# the plug-in. A clean `codesign --force -s -` after the bundle is finalised
# (and after JUCE's copy-to-plug-in-folder) fixes it. Runs after JUCE's own
# POST_BUILD copy because it is registered later on the same targets.
# -----------------------------------------------------------------------------
if(APPLE)
    foreach(_fmt AU VST3)
        if(TARGET Jove_${_fmt})
            add_custom_command(TARGET Jove_${_fmt} POST_BUILD
                COMMAND codesign --force -s - "$<TARGET_BUNDLE_DIR:Jove_${_fmt}>"
                VERBATIM)
            if(JOVE_INSTALL_LOCAL)
                if(_fmt STREQUAL AU)
                    set(_jove_dest "$ENV{HOME}/Library/Audio/Plug-Ins/Components/Jove.component")
                else()
                    set(_jove_dest "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/Jove.vst3")
                endif()
                add_custom_command(TARGET Jove_${_fmt} POST_BUILD
                    COMMAND codesign --force -s - "${_jove_dest}"
                    VERBATIM)
            endif()
        endif()
    endforeach()
    if(TARGET Jove_Standalone)
        add_custom_command(TARGET Jove_Standalone POST_BUILD
            COMMAND codesign --force -s - "$<TARGET_BUNDLE_DIR:Jove_Standalone>"
            VERBATIM)
    endif()
endif()

# -----------------------------------------------------------------------------
# Tests.
# -----------------------------------------------------------------------------
option(JOVE_TESTS "Build Jove engine + parameter audits" ON)
if(JOVE_TESTS)
    enable_testing()

    # APVTS <-> patch round-trip (needs JUCE: constructs the real processor).
    juce_add_console_app(jove_param_roundtrip PRODUCT_NAME "jove_param_roundtrip")
    juce_generate_juce_header(jove_param_roundtrip)
    target_sources(jove_param_roundtrip PRIVATE
        tests/param_roundtrip.cpp
        src/PluginProcessor.cpp
        src/ui/WebEditor.cpp
        src/ui/WebKeyboardFocus_mac.mm
        src/engine/SynthEngine.cpp
        src/engine/SynthPresets.cpp
        src/presets/PresetManager.cpp)
    target_include_directories(jove_param_roundtrip PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(jove_param_roundtrip PRIVATE
        JUCE_WEB_BROWSER=1 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(jove_param_roundtrip PRIVATE
        JoveUI juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    add_test(NAME param_roundtrip COMMAND jove_param_roundtrip)

    # Dump every factory preset's live parameter values to JSON (drives the
    # demo-video UI harness so each patch screenshot shows the real plugin UI).
    juce_add_console_app(jove_preset_dump PRODUCT_NAME "jove_preset_dump")
    juce_generate_juce_header(jove_preset_dump)
    target_sources(jove_preset_dump PRIVATE
        tests/preset_dump.cpp
        src/PluginProcessor.cpp
        src/ui/WebEditor.cpp
        src/ui/WebKeyboardFocus_mac.mm
        src/engine/SynthEngine.cpp
        src/engine/SynthPresets.cpp
        src/presets/PresetManager.cpp)
    target_include_directories(jove_preset_dump PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(jove_preset_dump PRIVATE
        JUCE_WEB_BROWSER=1 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(jove_preset_dump PRIVATE
        JoveUI juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

    # JUCE-free engine + preset audit (no NaN/clip/silence/stuck-voice).
    add_executable(jove_engine_audit tests/engine_audit.cpp
        src/engine/SynthEngine.cpp src/engine/SynthPresets.cpp)
    target_include_directories(jove_engine_audit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/engine)
    add_test(NAME engine_audit COMMAND jove_engine_audit)
endif()
