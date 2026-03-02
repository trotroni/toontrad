cmake_minimum_required(VERSION 3.22)
project(ToonTrad LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)

# Active l'automatisation pour les UI, MOC et RCC
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Fichiers sources et headers
set(SOURCES
        main.cpp
        mainwindow.cpp
)

set(HEADERS
        mainwindow.h
)

set(UIS
        mainwindow.ui
)

# Crée l'exécutable
add_executable(${PROJECT_NAME}
        ${SOURCES}
        ${HEADERS}
        ${UIS}  # Qt CMake va générer ui_mainwindow.h automatiquement
)

# Lie Qt Widgets
target_link_libraries(${PROJECT_NAME} Qt6::Widgets)