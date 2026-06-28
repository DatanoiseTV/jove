# -----------------------------------------------------------------------------
# Jove — analog-inspired polysynth instrument plugin.
#
# Self-contained target that reuses the parent repo's vendored JUCE (added by
# the root CMakeLists before this subdirectory). Doobie and Jove share nothing
# but the JUCE checkout, so this builds/ships independently. Rename / split the
# repo later without touching the engine.
# -----------------------------------------------------------------------------

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
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    NEEDS_WEB_BROWSER        TRUE
    COPY_PLUGIN_AFTER_BUILD  ${DOOBIE_INSTALL_LOCAL}
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

    # JUCE-free engine + preset audit (no NaN/clip/silence/stuck-voice).
    add_executable(jove_engine_audit tests/engine_audit.cpp
        src/engine/SynthEngine.cpp src/engine/SynthPresets.cpp)
    target_include_directories(jove_engine_audit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/engine)
    add_test(NAME engine_audit COMMAND jove_engine_audit)
endif()
