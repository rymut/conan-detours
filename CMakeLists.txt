cmake_minimum_required(VERSION 3.20)
if(POLICY CMP0076)
  cmake_policy(SET CMP0076 NEW)
endif()
project(detours LANGUAGES C CXX)

option(DETOURS_DEBUG "Detours debug" OFF)
if (MSVC)
    option(DETOURS_SOURCE_BROWSING "Detours generate SBR file" ON)
    option(DETOURS_ANALIZE "Detours analize code using MSVC" OFF)
endif()


if(MSVC)
    string(REGEX REPLACE "/W[1-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

# object target
set(SRC
    src/creatwth.cpp
    src/detours.cpp
    src/disasm.cpp
    src/disolarm.cpp
    src/disolarm64.cpp
    src/disolia64.cpp
    src/disolx64.cpp
    src/disolx86.cpp
    src/image.cpp
    src/modules.cpp)

add_library(${PROJECT_NAME}_objects OBJECT)
target_sources(${PROJECT_NAME}_objects
    PRIVATE
        ${SRC}
    PUBLIC
        src/detours.h
        src/detver.h)
target_compile_definitions(${PROJECT_NAME}_objects PRIVATE DETOUR_DEBUG=${DETOURS_DEBUG})
if (MSVC)
    target_compile_options(${PROJECT_NAME}_objects PRIVATE /we4777 /we4800 /W4 /WX)
    if (DETOURS_SOURCE_BROWSING)
        find_program(BSCMAKE_COMMAND bscmake REQUIRED)
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/src")
        string(REPLACE "/" "\\" _p "${CMAKE_CURRENT_BINARY_DIR}/src/")
        target_compile_options(${PROJECT_NAME}_objects PRIVATE "/FR${_p}")
    else()
        target_compile_definitions(${PROJECT_NAME}_objects PRIVATE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x501)
    endif()
    if (DETOURS_ANALIZE)
        target_compile_options(${PROJECT_NAME}_objects PRIVATE "/analize")
    endif()
endif()

# target library
add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE $<TARGET_OBJECTS:${PROJECT_NAME}_objects>)

# target browse information
set(OUT_BSC "${CMAKE_CURRENT_BINARY_DIR}/source_browse.bsc")
set(SBR "")
foreach(file_source IN LISTS SRC)
    get_filename_component(file_name ${file_source} NAME_WE)
    list(APPEND SBR "${CMAKE_CURRENT_BINARY_DIR}/src/${file_name}.sbr")
endforeach()
add_custom_target(
    ${PROJECT_NAME}_source_browse
    COMMAND bscmake /v /n /o "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.bsc" ${SBR}
    COMMENT "Generate BSC file"
    DEPENDS $<TARGET_OBJECTS:${PROJECT_NAME}_objects>
    VERBATIM
)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_source_browse)
