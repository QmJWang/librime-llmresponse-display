project(rime-llmresponse)
cmake_minimum_required(VERSION 3.10)

aux_source_directory(src llmresponse_src)

add_library(rime-llmresponse-objs OBJECT ${llmresponse_src})
if(BUILD_SHARED_LIBS)
  set_target_properties(rime-llmresponse-objs
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON)
endif()
if(MSVC)
  target_compile_options(rime-llmresponse-objs PRIVATE /utf-8)
endif()

set(plugin_name rime-llmresponse PARENT_SCOPE)
set(plugin_objs $<TARGET_OBJECTS:rime-llmresponse-objs> PARENT_SCOPE)
set(plugin_deps ${rime_library} PARENT_SCOPE)
set(plugin_modules "llmresponse" PARENT_SCOPE)
