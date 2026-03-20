cmake_minimum_required(VERSION 3.16)
project(ToonTrad VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Svg)

# ── Icône macOS ───────────────────────────────────────────────────────────────
set(APP_ICON_MACOS "")
if(APPLE)
    set(ICON_FILE "${CMAKE_SOURCE_DIR}/resources/toontrad-icon-red-double-t.svg")
    # macOS préfère .icns — si tu en crées un plus tard, remplace ici
    # set(ICON_FILE "${CMAKE_SOURCE_DIR}/resources/ToonTrad.icns")
    if(EXISTS "${ICON_FILE}")
        set(APP_ICON_MACOS ${ICON_FILE})
        set_source_files_properties(${ICON_FILE} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources")
    endif()
endif()

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

    windows/project/NewProjectDialog.h
    windows/project/NewProjectDialog.cpp
    windows/project/NewProjectDialog.ui

    resources.qrc
)

qt_add_executable(ToonTrad ${SOURCES} ${APP_ICON_MACOS})

# ── Bundle macOS .app ─────────────────────────────────────────────────────────
if(APPLE)
    set_target_properties(ToonTrad PROPERTIES
        MACOSX_BUNDLE                   TRUE
        MACOSX_BUNDLE_BUNDLE_NAME       "ToonTrad"
        MACOSX_BUNDLE_BUNDLE_VERSION    "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_GUI_IDENTIFIER    "com.toontrad.app"
        MACOSX_BUNDLE_INFO_PLIST        "${CMAKE_SOURCE_DIR}/resources/Info.plist.in"
    )
endif()

target_link_libraries(ToonTrad PRIVATE Qt6::Widgets Qt6::Network Qt6::Svg)
target_include_directories(ToonTrad PRIVATE ${CMAKE_SOURCE_DIR})

# ── Python ────────────────────────────────────────────────────────────────────
find_program(PYTHON_EXEC NAMES python3 python REQUIRED)
message(STATUS "Python trouvé : ${PYTHON_EXEC}")

# ── Copie python/ et resources/ dans le bundle .app ou à côté de l'exe ────────
if(APPLE)
    # Dans le bundle : ToonTrad.app/Contents/MacOS/../python/
    set(BUNDLE_CONTENTS "$<TARGET_BUNDLE_CONTENT_DIR:ToonTrad>")
    add_custom_command(TARGET ToonTrad POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/python"
            "${BUNDLE_CONTENTS}/python"
        COMMENT "Copie python/ dans le bundle .app"
    )
    add_custom_command(TARGET ToonTrad POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources"
            "${BUNDLE_CONTENTS}/resources"
        COMMENT "Copie resources/ dans le bundle .app"
    )
    # Qt deploy tool — intègre les frameworks Qt dans le bundle
    find_program(MACDEPLOYQT macdeployqt
        HINTS "${Qt6_DIR}/../../../bin")
    if(MACDEPLOYQT)
        add_custom_command(TARGET ToonTrad POST_BUILD
            COMMAND ${MACDEPLOYQT}
                "$<TARGET_BUNDLE_DIR:ToonTrad>"
                -always-overwrite
            COMMENT "macdeployqt — intègre Qt dans le bundle"
        )
    else()
        message(WARNING "macdeployqt introuvable — le .app ne sera pas autonome")
    endif()
else()
    # Linux / Windows : copie à côté de l'exe
    add_custom_command(TARGET ToonTrad POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/python"
            "$<TARGET_FILE_DIR:ToonTrad>/python"
        COMMENT "Copie python/ dans le répertoire de build"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources/tessdata")
        add_custom_command(TARGET ToonTrad POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/resources"
                "$<TARGET_FILE_DIR:ToonTrad>/resources"
            COMMENT "Copie resources/ dans le répertoire de build"
        )
    endif()
endif()
