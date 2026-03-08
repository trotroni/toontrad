cmake_minimum_required(VERSION 3.16)

project(ToonTrad)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(SOURCES
        app/App.cpp

        core/OCRManager.cpp

        gui/canvas/ImageCanvas.cpp

        windows/main/mainwindow.cpp
        windows/OCR/OCRwindow.cpp
        main.cpp
)

set(HEADERS
        app/App.h

        core/OCRManager.h
        core/models.h

        gui/canvas/ImageCanvas.h

        windows/main/mainwindow.h
        windows/OCR/OCRwindow.h
        main.cpp
)

set(UIS
        app/App.ui
        windows/main/mainwindow.ui
        windows/OCR/OCRwindow.ui
        main.cpp
        core/BubbleDetector.cpp
        core/Exporter.cpp
        core/TextBlock.cpp
        core/Translator.cpp
)
qt_add_executable(ToonTrad
        ${SOURCES}
        ${HEADERS}
        ${UIS}
        main.cpp
)

target_link_libraries(ToonTrad PRIVATE Qt6::Widgets)