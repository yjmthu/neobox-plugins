cmake_minimum_required(VERSION 3.21.2)

# C++20标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 导出动态库
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
# 导出ninja编译命令
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Neobox主程序安装位置
set(PLUGIN_MANAGER_PATH ../Neobox/install)

project(neobox_plugins VERSION 2.4.3 LANGUAGES CXX C)

function(load_resources plugin_name files_list)
  message("- <${plugin_name}> ${${files_list}}")

  file(GLOB _resource_files ${${files_list}})
  set(resource_files)
  foreach(full_filepath ${_resource_files})
    file(RELATIVE_PATH file_path ${CMAKE_CURRENT_SOURCE_DIR} ${full_filepath})
    message(STATUS "res: '${file_path}'")
    list(APPEND resource_files ${file_path})
  endforeach()

  qt_add_resources(${plugin_name} "${plugin_name}"
    PREFIX "/"
    FILES ${resource_files}
  )
  message("- <${plugin_name}>")
endfunction()

if (WIN32)
  add_definitions(-DUNICODE -D_UNICODE)
endif()

if(MSVC)
  add_compile_options("/utf-8")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

include_directories(${PLUGIN_MANAGER_PATH}/include)
link_directories(${PLUGIN_MANAGER_PATH}/lib)
add_subdirectory(plugins)
