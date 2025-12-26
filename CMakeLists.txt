cmake_minimum_required(VERSION 4.0)
cmake_policy(VERSION 4.0)
cmake_policy(SET CMP0072 NEW)

project(MultiprojectOpenGL VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

IF(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/modules/)

IF(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
ENDIF(WIN32)

link_directories(${CMAKE_SOURCE_DIR}/lib)

# find the required packages
find_package(GLM REQUIRED)
message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")
find_package(GLFW3 REQUIRED)
message(STATUS "GLFW3 included at ${GLFW3_INCLUDE_DIR}")
find_package(ASSIMP REQUIRED)
message(STATUS "ASSIMP included at ${ASSIMP_INCLUDE_DIR}")

IF(WIN32)
    set(LIBS glfw3 opengl32 assimp)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ELSEIF(UNIX AND NOT APPLE)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
    find_package(OpenGL REQUIRED)
    add_definitions(${OPENGL_DEFINITIONS})
    find_package(X11 REQUIRED)
    set(LIBS ${GLFW3_LIBRARY} X11 Xrandr Xinerama Xi Xxf86vm Xcursor GL dl pthread ${ASSIMP_LIBRARY})
    set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")
ELSEIF(APPLE)
    include_directories(/System/Library/Frameworks)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(OpenGL_LIBRARY OpenGL)
    find_library(IOKit_LIBRARY IOKit)
    find_library(CoreVideo_LIBRARY CoreVideo)
    mark_as_advanced(COCOA_LIBRARY OpenGL_LIBRARY)
    set(APPLE_LIBS ${COCOA_LIBRARY} ${IOKit_LIBRARY} ${OpenGL_LIBRARY} ${CoreVideo_LIBRARY})
    set(APPLE_LIBS ${APPLE_LIBS} ${GLFW3_LIBRARY} ${ASSIMP_LIBRARY} ${FREETYPE_LIBRARIES})
    set(LIBS ${LIBS} ${APPLE_LIBS})
ELSE()
    set(LIBS )
ENDIF(WIN32)

MACRO(subdirlist result_var current_dir)
    file(GLOB children "${current_dir}/*")

    set(dir_list "")
    FOREACH(child ${children})
        IF(IS_DIRECTORY ${child})
            list(APPEND dir_list ${child})
        ENDIF()
    ENDFOREACH()

    set(${result_var} ${dir_list} PARENT_SCOPE)
ENDMACRO()

subdirlist(PROJECTS ${CMAKE_SOURCE_DIR}/projects)

configure_file(configuration/root_directory.h.in configuration/root_directory.h)
include_directories(${CMAKE_BINARY_DIR}/configuration)

add_library(STB_IMAGE stb_image.cpp)
set(LIBS ${LIBS} STB_IMAGE)

add_library(GLAD gl.c)
set(LIBS ${LIBS} GLAD)

MACRO(makeLink src dest target)
  add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${src} ${dest}  DEPENDS  ${dest} COMMENT "mklink ${src} -> ${dest}")
ENDMACRO()

FUNCTION(create_project project)
	file(GLOB SOURCE
            "projects/${project}/*.h"
            "projects/${project}/*.cpp"
            "projects/${project}/*.vs"
            "projects/${project}/*.fs"
            "projects/${project}/*.tcs"
            "projects/${project}/*.tes"
            "projects/${project}/*.gs"
            "projects/${project}/*.cs"
            "src/*.h"
            "src/*.cpp"
            "src/.vs"
            "src/.fs"
            "src/.tcs"
            "src/.tes"
            "src/.gs"
            "src/.cs"
    )
	set(NAME "${project}")
    add_executable(${NAME} ${COMMON} ${SOURCE})
    target_link_libraries(${NAME} ${LIBS})
    IF(MSVC)
		target_compile_options(${NAME} PRIVATE /std:c++20 /MP)
        target_link_options(${NAME} PUBLIC /ignore:4099)
    ENDIF(MSVC)
    IF(WIN32)
        set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${chapter}")
        set_target_properties(${NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${chapter}/Debug")
    ELSEIF(UNIX AND NOT APPLE)
        set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${chapter}")
    ELSEIF(APPLE)
        set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${chapter}")
        set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/${chapter}")
        set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/${chapter}")
    ENDIF(WIN32)
    # copy shader files to build directory
    file(GLOB SHADERS
            "projects/${project}/*.vs"
            "projects/${project}/*.fs"
            "projects/${project}/*.tcs"
            "projects/${project}/*.tes"
            "projects/${project}/*.gs"
            "projects/${project}/*.cs"
            "src/.vs"
            "src/.fs"
            "src/.tcs"
            "src/.tes"
            "src/.gs"
            "src/.cs"
    )
	# copy dlls
	file(GLOB DLLS "dlls/*.dll")
    FOREACH(SHADER ${SHADERS})
        IF(WIN32)
            # configure_file(${SHADER} "test")
            add_custom_command(TARGET ${NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SHADER} $<TARGET_FILE_DIR:${NAME}>)
			add_custom_command(TARGET ${NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLLS} $<TARGET_FILE_DIR:${NAME}>)
        ELSEIF(UNIX AND NOT APPLE)
            file(COPY ${SHADER} DESTINATION ${CMAKE_SOURCE_DIR}/bin/${chapter})
        ELSEIF(APPLE)
            # create symbolic link for *.vs *.fs *.gs
            get_filename_component(SHADERNAME ${SHADER} NAME)
            makeLink(${SHADER} ${CMAKE_SOURCE_DIR}/bin/${chapter}/${SHADERNAME} ${NAME})
        ENDIF(WIN32)
    ENDFOREACH(SHADER)
    # if compiling for visual studio, also use configure file for each project (specifically to set up working directory)
    IF(MSVC)
        configure_file(${CMAKE_SOURCE_DIR}/configuration/visualstudio.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.vcxproj.user @ONLY)
    ENDIF(MSVC)
ENDFUNCTION()

FOREACH(PROJECT ${PROJECTS})
    create_project(${PROJECT})
ENDFOREACH(PROJECT)

include_directories(${CMAKE_SOURCE_DIR}/includes)
