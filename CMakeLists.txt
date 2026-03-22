cmake_minimum_required(VERSION 3.16)
project(ToonTrad VERSION 2.0.0-beta LANGUAGES CXX)

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
        core/BubbleDetector.cpp
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
        core/BubbleDetector.h
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
        windows/main/mainwindow.ui
        windows/OCR/OCRwindow.ui
        windows/settings/SettingsWindow.ui
)

# ── macOS : bundle .app ───────────────────────────────────────────────────────
if(APPLE)
    set(MACOSX_BUNDLE_PROPERTIES
        MACOSX_BUNDLE                    TRUE
        MACOSX_BUNDLE_BUNDLE_NAME        "ToonTrad"
        MACOSX_BUNDLE_GUI_IDENTIFIER     "com.toontrad.app"
        MACOSX_BUNDLE_BUNDLE_VERSION     "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif()

qt_add_executable(ToonTrad
        ${SOURCES}
        ${HEADERS}
        ${UIS}
)

if(APPLE)
    set_target_properties(ToonTrad PROPERTIES ${MACOSX_BUNDLE_PROPERTIES})
endif()

target_link_libraries(ToonTrad PRIVATE Qt6::Widgets Qt6::Network)
target_include_directories(ToonTrad PRIVATE ${CMAKE_SOURCE_DIR})

# ── Détection de Python ───────────────────────────────────────────────────────
find_program(PYTHON_EXEC NAMES python3 python REQUIRED)
message(STATUS "Python trouvé : ${PYTHON_EXEC}")

# ── Téléchargement automatique de tessdata au build ───────────────────────────
set(TESSDATA_DIR "${CMAKE_SOURCE_DIR}/resources/tessdata")
add_custom_target(download_tessdata ALL
        COMMAND ${PYTHON_EXEC}
        "${CMAKE_SOURCE_DIR}/scripts/download_tessdata.py"
        "${TESSDATA_DIR}"
        COMMENT "Vérification/téléchargement des modèles Tesseract (tessdata)..."
        VERBATIM
)
add_dependencies(ToonTrad download_tessdata)

# ── Copie python/ à côté de l'exécutable (build local) ───────────────────────
add_custom_command(TARGET ToonTrad POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/python"
        "$<TARGET_FILE_DIR:ToonTrad>/python"
        COMMENT "Copie python/ dans le répertoire de build"
)

add_custom_command(TARGET ToonTrad POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:ToonTrad>/resources"
        COMMENT "Copie resources/tessdata dans le répertoire de build"
)

# ═════════════════════════════════════════════════════════════════════════════
#  INSTALL — règles utilisées par CPack pour construire les installateurs
# ═════════════════════════════════════════════════════════════════════════════

if(APPLE)
    # Sur macOS : installe le .app complet (macdeployqt est lancé dans la CI
    # AVANT cpack, ce qui peuple le .app avec les frameworks Qt)
    install(TARGETS ToonTrad
            BUNDLE  DESTINATION .
            RUNTIME DESTINATION bin
    )
    # python/ et resources/ à l'intérieur du bundle
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/python"
            DESTINATION "ToonTrad.app/Contents/Resources"
    )
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources"
            DESTINATION "ToonTrad.app/Contents/Resources"
    )
else()
    # Windows / Linux : install à plat dans le répertoire d'install
    install(TARGETS ToonTrad
            RUNTIME DESTINATION .
    )
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/python"
            DESTINATION .
    )
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources"
            DESTINATION .
    )
endif()

# ═════════════════════════════════════════════════════════════════════════════
#  CPACK — configuration commune
# ═════════════════════════════════════════════════════════════════════════════
set(CPACK_PACKAGE_NAME            "ToonTrad")
set(CPACK_PACKAGE_VENDOR          "ToonTrad")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Outil de scantrad manga/webtoon")
set(CPACK_PACKAGE_VERSION         "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR   "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR   "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH   "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "ToonTrad")
set(CPACK_RESOURCE_FILE_LICENSE   "${CMAKE_SOURCE_DIR}/LICENSE")

# ── macOS : DragNDrop → .dmg ──────────────────────────────────────────────────
if(APPLE)
    set(CPACK_GENERATOR           "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME     "ToonTrad ${PROJECT_VERSION}")
    set(CPACK_DMG_FORMAT          "UDZO")
    # Nom du fichier final : ToonTrad-2.0-Darwin.dmg
    set(CPACK_PACKAGE_FILE_NAME   "ToonTrad-${PROJECT_VERSION}-Darwin")
endif()

# ── Windows : NSIS → .exe installateur ───────────────────────────────────────
if(WIN32)
    set(CPACK_GENERATOR              "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME      "ToonTrad ${PROJECT_VERSION}")
    set(CPACK_NSIS_PACKAGE_NAME      "ToonTrad")
    set(CPACK_NSIS_INSTALL_ROOT      "$PROGRAMFILES64")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH       OFF)
    # Créer un raccourci bureau vers ToonTrad.exe
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortcut '$DESKTOP\\\\ToonTrad.lnk' '$INSTDIR\\\\ToonTrad.exe'"
    )
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$DESKTOP\\\\ToonTrad.lnk'"
    )
    # Nom du fichier final : ToonTrad-2.0-win64.exe
    set(CPACK_PACKAGE_FILE_NAME      "ToonTrad-${PROJECT_VERSION}-win64")
endif()

include(CPack)
