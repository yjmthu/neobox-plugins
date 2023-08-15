cmake_minimum_required(VERSION 3.10.1)

set(plugin_name calculator)
file(GLOB src_files src/*.cpp)

add_library(${plugin_name} SHARED ${src_files})
target_include_directories(${plugin_name} PRIVATE "include")

target_link_libraries(${plugin_name} PRIVATE
  Qt::Core pluginmgr
)
