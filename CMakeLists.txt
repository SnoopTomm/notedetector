cmake_minimum_required(VERSION 3.22)
project(NoteDetector VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.4)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(NoteDetector
  COMPANY_NAME "YourName"
  PLUGIN_MANUFACTURER_CODE Yrnm
  PLUGIN_CODE Ntdt
  FORMATS VST3 Standalone
  PRODUCT_NAME "Note Detector"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT TRUE
  IS_MIDI_EFFECT FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(NoteDetector PRIVATE
  Source/PluginProcessor.cpp)

target_compile_definitions(NoteDetector PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(NoteDetector PRIVATE
  juce::juce_audio_utils
  PUBLIC
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags)
