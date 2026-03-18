cmake_minimum_required(VERSION 3.16)
project(ToonTrad VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

# ── Sources ───────────────────────────────────────────────────────────────────
set(SOURCES
        main.cpp
        config.cpp

        core/TextBlock.cpp
        core/Bubble.cpp
        core/OCRManager.cpp
        core/Exporter.cpp
        core/Translator.cpp
        core/ProjectManager.cpp

        gui/canvas/ImageCanvas.cpp

        windows/main/mainwindow.cpp
        windows/OCR/OCRwindow.cpp
        windows/settings/SettingsWindow.cpp
)

set(HEADERS
        config.h

        core/TextBlock.h
        core/Bubble.h
        core/OCRManager.h
        core/OCRConfig.h
        core/Exporter.h
        core/Translator.h
        core/ProjectManager.h
        core/models.h

        gui/canvas/ImageCanvas.h

        windows/main/mainwindow.h
        windows/OCR/OCRwindow.h
        windows/settings/SettingsWindow.h
)

set(UIS
        windows/main/mainwindow.uii
        windows/OCR/OCRwindow.ui
        windows/settings/SettingsWindow.ui
)

qt_add_executable(ToonTrad
        ${SOURCES}
        ${HEADERS}
        ${UIS}
)

target_link_libraries(ToonTrad PRIVATE Qt6::Widgets Qt6::Network)
target_include_directories(ToonTrad PRIVATE ${CMAKE_SOURCE_DIR})

# ── Détection de Python (même interpréteur que celui configuré dans l'app) ────
find_program(PYTHON_EXEC NAMES python3 python REQUIRED)
message(STATUS "Python trouvé : ${PYTHON_EXEC}")

# ── Téléchargement automatique de tessdata au moment du build ─────────────────
set(TESSDATA_DIR "${CMAKE_SOURCE_DIR}/resources/tessdata")
add_custom_target(download_tessdata ALL
        COMMAND ${PYTHON_EXEC}
        "${CMAKE_SOURCE_DIR}/scripts/download_tessdata.py"
        "${TESSDATA_DIR}"
        COMMENT "Vérification/téléchargement des modèles Tesseract (tessdata)..."
        VERBATIM
)
add_dependencies(ToonTrad download_tessdata)

# ── Copie python/ à côté de l'exécutable ─────────────────────────────────────
add_custom_command(TARGET ToonTrad POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/python"
        "$<TARGET_FILE_DIR:ToonTrad>/python"
        COMMENT "Copie python/ dans le répertoire de build"
)

# ── Copie resources/ (tessdata bundlé) à côté de l'exécutable ────────────────
add_custom_command(TARGET ToonTrad POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:ToonTrad>/resources"
        COMMENT "Copie resources/tessdata dans le répertoire de build"
)