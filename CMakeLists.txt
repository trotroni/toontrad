cmake_minimum_required(VERSION 3.16)
project(ToonTrad VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

set(SOURCES
    main.cpp
    config.h
    config.cpp

    core/TextBlock.h
    core/TextBlock.cpp
    core/OCRConfig.h
    core/models.h
    core/OCRManager.h
    core/OCRManager.cpp
    core/BubbleDetector.h
    core/BubbleDetector.cpp
    core/Exporter.h
    core/Exporter.cpp
    core/ProjectManager.h
    core/ProjectManager.cpp

    gui/canvas/ImageCanvas.h
    gui/canvas/ImageCanvas.cpp

    windows/main/MainWindow.h
    windows/main/MainWindow.cpp
    windows/main/MainWindow.ui

    windows/image/ImageWindow.h
    windows/image/ImageWindow.cpp
    windows/image/ImageWindow.ui

    windows/text/TextWindow.h
    windows/text/TextWindow.cpp
    windows/text/TextWindow.ui

    windows/settings/SettingsWindow.h
    windows/settings/SettingsWindow.cpp
    windows/settings/SettingsWindow.ui
)

qt_add_executable(ToonTrad ${SOURCES})

target_link_libraries(ToonTrad PRIVATE Qt6::Widgets Qt6::Network)
target_include_directories(ToonTrad PRIVATE ${CMAKE_SOURCE_DIR})

# ── Python ────────────────────────────────────────────────────────────────────
find_program(PYTHON_EXEC NAMES python3 python REQUIRED)
message(STATUS "Python trouvé : ${PYTHON_EXEC}")

# ── Copie python/ à côté de l'exe ─────────────────────────────────────────────
add_custom_command(TARGET ToonTrad POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/python"
        "$<TARGET_FILE_DIR:ToonTrad>/python"
    COMMENT "Copie python/ dans le répertoire de build"
)

# ── Copie tessdata si présent ──────────────────────────────────────────────────
if(EXISTS "${CMAKE_SOURCE_DIR}/resources/tessdata")
    add_custom_command(TARGET ToonTrad POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:ToonTrad>/resources"
        COMMENT "Copie resources/ dans le répertoire de build"
    )
endif()
