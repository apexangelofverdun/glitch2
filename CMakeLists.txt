cmake_minimum_required(VERSION 3.22)
project(GlitchSeq VERSION 0.1.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4)
FetchContent_MakeAvailable(JUCE)

if(DEFINED ENV{CI})
    set(GLITCHSEQ_COPY FALSE)
else()
    set(GLITCHSEQ_COPY TRUE)
endif()

juce_add_plugin(GlitchSeq
    COMPANY_NAME "YourName"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Ynam
    PLUGIN_CODE Glsq
    FORMATS VST3 Standalone
    COPY_PLUGIN_AFTER_BUILD ${GLITCHSEQ_COPY}
    PRODUCT_NAME "GlitchSeq")

target_sources(GlitchSeq PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h)

target_compile_definitions(GlitchSeq PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(GlitchSeq
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
