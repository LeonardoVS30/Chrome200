cmake_minimum_required(VERSION 3.24)

project(Chrome2000 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CHROME2000_BUILD_STANDALONE "Build the standalone application" ON)
option(CHROME2000_ENABLE_ASAN "Enable AddressSanitizer for crash diagnostics" OFF)
option(CHROME2000_BUILD_TESTS "Build offline audio engine smoke tests" ON)
set(CHROME2000_JUCE_PATH "" CACHE PATH "Optional path to a local JUCE 8 checkout")

if(CHROME2000_JUCE_PATH)
    add_subdirectory("${CHROME2000_JUCE_PATH}" JUCE)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.deps/JUCE/CMakeLists.txt")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/.deps/JUCE" JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.12
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(CHROME2000_FORMATS VST3)
if(CHROME2000_BUILD_STANDALONE)
    list(APPEND CHROME2000_FORMATS Standalone)
endif()

juce_add_plugin(Chrome2000
    COMPANY_NAME "LEVSAN"
    BUNDLE_ID "com.levsan.chrome2000"
    PLUGIN_MANUFACTURER_CODE Ch2K
    PLUGIN_CODE C2KS
    FORMATS ${CHROME2000_FORMATS}
    PRODUCT_NAME "CHROME 2000"
    PLUGIN_NAME "CHROME 2000"
    DESCRIPTION "Hybrid granular, spectral and tape sample synthesizer"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES "Instrument|Synth")

juce_generate_juce_header(Chrome2000)

juce_add_binary_data(Chrome2000Assets SOURCES
    Source/Assets/Fonts/Inter-Variable.ttf
    Source/Assets/Fonts/Rajdhani-Medium.ttf
    Source/Assets/Fonts/Rajdhani-SemiBold.ttf
    Source/Assets/Fonts/Rajdhani-Bold.ttf
    Source/Assets/Fonts/DMMono-Regular.ttf
    Source/Assets/Fonts/DMMono-Medium.ttf
    Source/Assets/UI/chrome2000_figma_skin.png
    Source/Assets/UI/chrome_logo.png
    Source/Assets/UI/chrome_wordmark.svg
    Source/Assets/UI/knob_small_face.png
    Source/Assets/UI/knob_small_rim.svg
    Source/Assets/UI/knob_small_shadow.svg
    Source/Assets/UI/knob_small_patina.svg
    Source/Assets/UI/knob_small_specular.svg
    Source/Assets/UI/knob_big_face.png
    Source/Assets/UI/knob_big_rim.svg
    Source/Assets/UI/knob_big_shadow.svg
    Source/Assets/UI/knob_big_patina.svg
    Source/Assets/UI/knob_big_specular.svg
    Source/Assets/UI/knob_big_halo.svg)

target_sources(Chrome2000 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Parameters/ParameterLayout.cpp
    Source/Parameters/ParameterLayout.h
    Source/SampleEngine/SampleData.cpp
    Source/SampleEngine/SampleData.h
    Source/SampleEngine/PitchDetector.cpp
    Source/SampleEngine/PitchDetector.h
    Source/SampleEngine/SampleLoader.cpp
    Source/SampleEngine/SampleLoader.h
    Source/SampleEngine/SampleEngine.cpp
    Source/SampleEngine/SampleEngine.h
    Source/Engine/Voice.cpp
    Source/Engine/Voice.h
    Source/DSP/VintageProcessor.cpp
    Source/DSP/VintageProcessor.h
    Source/DSP/ChromeProcessor.cpp
    Source/DSP/ChromeProcessor.h
    Source/DSP/DigitalProcessor.cpp
    Source/DSP/DigitalProcessor.h
    Source/Effects/MasterEffects.cpp
    Source/Effects/MasterEffects.h
    Source/PresetManager/PresetManager.cpp
    Source/PresetManager/PresetManager.h
    Source/GUI/ChromeLookAndFeel.cpp
    Source/GUI/ChromeLookAndFeel.h
    Source/GUI/WaveformComponent.cpp
    Source/GUI/WaveformComponent.h
    Source/GUI/SampleSlotComponent.cpp
    Source/GUI/SampleSlotComponent.h)

target_compile_definitions(Chrome2000 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0
    DONT_SET_USING_JUCE_NAMESPACE=1)

target_link_libraries(Chrome2000 PRIVATE
    Chrome2000Assets
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

if(CHROME2000_ENABLE_ASAN AND MSVC)
    target_compile_options(Chrome2000 PRIVATE /fsanitize=address /Zi)
    target_link_options(Chrome2000 PRIVATE /fsanitize=address /INCREMENTAL:NO)
    foreach(client_target Chrome2000_Standalone Chrome2000_VST3)
        if(TARGET ${client_target})
            target_compile_options(${client_target} PRIVATE /fsanitize=address /Zi)
            target_link_options(${client_target} PRIVATE /fsanitize=address /INCREMENTAL:NO)
        endif()
    endforeach()
endif()

if(MSVC AND TARGET Chrome2000_Standalone)
    target_link_options(Chrome2000_Standalone PRIVATE
        "/MAP:${CMAKE_BINARY_DIR}/Chrome2000_Standalone.map"
        /MAPINFO:EXPORTS)
endif()

if(CHROME2000_BUILD_TESTS)
    enable_testing()
    juce_add_console_app(Chrome2000EngineSmoke
        PRODUCT_NAME "CHROME 2000 Engine Smoke Test")

    juce_generate_juce_header(Chrome2000EngineSmoke)

    target_sources(Chrome2000EngineSmoke PRIVATE
        Tests/EngineSmokeTest.cpp
        Source/Parameters/ParameterLayout.cpp
        Source/Parameters/ParameterLayout.h
        Source/SampleEngine/SampleData.cpp
        Source/SampleEngine/SampleData.h
        Source/SampleEngine/PitchDetector.cpp
        Source/SampleEngine/PitchDetector.h
        Source/SampleEngine/SampleLoader.cpp
        Source/SampleEngine/SampleLoader.h
        Source/SampleEngine/SampleEngine.cpp
        Source/SampleEngine/SampleEngine.h
        Source/Engine/Voice.cpp
        Source/Engine/Voice.h
        Source/DSP/VintageProcessor.cpp
        Source/DSP/VintageProcessor.h
        Source/DSP/ChromeProcessor.cpp
        Source/DSP/ChromeProcessor.h
        Source/DSP/DigitalProcessor.cpp
        Source/DSP/DigitalProcessor.h
        Source/Effects/MasterEffects.cpp
        Source/Effects/MasterEffects.h)

    target_compile_definitions(Chrome2000EngineSmoke PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_REPORT_APP_USAGE=0
        DONT_SET_USING_JUCE_NAMESPACE=1)

    target_link_libraries(Chrome2000EngineSmoke PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

    juce_add_console_app(Chrome2000CharacterDSP
        PRODUCT_NAME "CHROME 2000 Character DSP Test")

    juce_generate_juce_header(Chrome2000CharacterDSP)
    target_sources(Chrome2000CharacterDSP PRIVATE
        Tests/CharacterDSPTest.cpp
        Source/DSP/VintageProcessor.cpp
        Source/DSP/VintageProcessor.h
        Source/DSP/ChromeProcessor.cpp
        Source/DSP/ChromeProcessor.h
        Source/DSP/DigitalProcessor.cpp
        Source/DSP/DigitalProcessor.h)
    target_compile_definitions(Chrome2000CharacterDSP PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_REPORT_APP_USAGE=0
        DONT_SET_USING_JUCE_NAMESPACE=1)
    target_link_libraries(Chrome2000CharacterDSP PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

    juce_add_console_app(Chrome2000PitchAndTuning
        PRODUCT_NAME "CHROME 2000 Pitch and Tuning Test")
    juce_generate_juce_header(Chrome2000PitchAndTuning)
    target_sources(Chrome2000PitchAndTuning PRIVATE
        Tests/PitchAndTuningTest.cpp
        Source/SampleEngine/PitchDetector.cpp
        Source/SampleEngine/PitchDetector.h
        Source/SampleEngine/SampleData.cpp
        Source/SampleEngine/SampleData.h
        Source/Engine/Voice.cpp
        Source/Engine/Voice.h)
    target_compile_definitions(Chrome2000PitchAndTuning PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_REPORT_APP_USAGE=0
        DONT_SET_USING_JUCE_NAMESPACE=1)
    target_link_libraries(Chrome2000PitchAndTuning PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

    juce_add_console_app(Chrome2000ProcessorSmoke
        PRODUCT_NAME "CHROME 2000 Processor Smoke Test")

    juce_generate_juce_header(Chrome2000ProcessorSmoke)
    target_sources(Chrome2000ProcessorSmoke PRIVATE Tests/ProcessorSmokeTest.cpp)
    target_compile_definitions(Chrome2000ProcessorSmoke PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0
        DONT_SET_USING_JUCE_NAMESPACE=1)
    target_link_libraries(Chrome2000ProcessorSmoke PRIVATE
        Chrome2000
        Chrome2000Assets
        juce::juce_audio_utils
        juce::juce_dsp
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

    juce_add_console_app(Chrome2000FeatureSmoke
        PRODUCT_NAME "CHROME 2000 Feature Smoke Test")
    juce_generate_juce_header(Chrome2000FeatureSmoke)
    target_sources(Chrome2000FeatureSmoke PRIVATE Tests/FeatureSmokeTest.cpp)
    target_compile_definitions(Chrome2000FeatureSmoke PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0
        DONT_SET_USING_JUCE_NAMESPACE=1)
    target_link_libraries(Chrome2000FeatureSmoke PRIVATE
        Chrome2000
        Chrome2000Assets
        juce::juce_audio_utils
        juce::juce_dsp
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

    juce_add_console_app(Chrome2000VST3HostSmoke
        PRODUCT_NAME "CHROME 2000 Installed VST3 Host Smoke Test")
    juce_generate_juce_header(Chrome2000VST3HostSmoke)
    target_sources(Chrome2000VST3HostSmoke PRIVATE Tests/VST3HostSmokeTest.cpp)
    target_compile_definitions(Chrome2000VST3HostSmoke PRIVATE
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_VST=0
        JUCE_PLUGINHOST_AU=0
        JUCE_PLUGINHOST_LADSPA=0
        JUCE_PLUGINHOST_LV2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_REPORT_APP_USAGE=0
        DONT_SET_USING_JUCE_NAMESPACE=1)
    target_link_libraries(Chrome2000VST3HostSmoke PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

    juce_add_console_app(Chrome2000EditorSnapshot
        PRODUCT_NAME "CHROME 2000 Editor Snapshot")
    juce_generate_juce_header(Chrome2000EditorSnapshot)
    target_sources(Chrome2000EditorSnapshot PRIVATE Tests/EditorSnapshotTest.cpp)
    target_compile_definitions(Chrome2000EditorSnapshot PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0
        DONT_SET_USING_JUCE_NAMESPACE=1)
    target_link_libraries(Chrome2000EditorSnapshot PRIVATE
        Chrome2000
        Chrome2000Assets
        juce::juce_audio_utils
        juce::juce_dsp
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

    add_test (NAME Chrome2000EngineSmoke COMMAND Chrome2000EngineSmoke)
    add_test (NAME Chrome2000CharacterDSP COMMAND Chrome2000CharacterDSP)
    add_test (NAME Chrome2000PitchAndTuning COMMAND Chrome2000PitchAndTuning)
    add_test (NAME Chrome2000ProcessorSmoke COMMAND Chrome2000ProcessorSmoke)
    add_test (NAME Chrome2000FeatureSmoke COMMAND Chrome2000FeatureSmoke)
    add_test (NAME Chrome2000EditorSnapshot COMMAND Chrome2000EditorSnapshot)
    add_test (NAME Chrome2000VST3HostSmoke COMMAND Chrome2000VST3HostSmoke)
endif()
