cmake_minimum_required(VERSION 3.10)

project(AquiumUpdater)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(AquiumUpdater
    src/main.cpp
)

target_link_libraries(AquiumUpdater
    PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
)
