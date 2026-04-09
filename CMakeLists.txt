cmake_minimum_required(VERSION 3.16)
project(ToonTrad VERSION "2.0.3.0" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Svg)

# ── Python ────────────────────────────────────────────────────────────────────
find_program(PYTHON_EXEC NAMES python3 python REQUIRED)
message(STATUS "Python trouvé : ${PYTHON_EXEC}")

# ── 1. Génération automatique des icônes depuis le SVG ────────────────────────
set(SVG_ICON     "${CMAKE_SOURCE_DIR}/resources/toontrad-icon-raw.svg")
set(ICO_ICON     "${CMAKE_SOURCE_DIR}/resources/tt-icon-win.ico")
set(ICNS_ICON    "${CMAKE_SOURCE_DIR}/resources/tt-icon-mac.icns")
set(ICNS_PROJECT "${CMAKE_SOURCE_DIR}/resources/tt-project.icns")

add_custom_command(
    OUTPUT "${ICO_ICON}" "${ICNS_ICON}" "${ICNS_PROJECT}"
    COMMAND ${PYTHON_EXEC}
        "${CMAKE_SOURCE_DIR}/scripts/generate_icons.py"
        "${SVG_ICON}" "${ICO_ICON}" "${ICNS_ICON}" "${ICNS_PROJECT}"
    DEPENDS "${SVG_ICON}" "${CMAKE_SOURCE_DIR}/scripts/generate_icons.py"
    COMMENT "Génération des icônes .ico / .icns depuis le SVG..."
)
add_custom_target(generate_icons
    DEPENDS "${ICO_ICON}" "${ICNS_ICON}" "${ICNS_PROJECT}"
)

# ── 2. Téléchargement automatique de tessdata au build ────────────────────────
set(TESSDATA_DIR "${CMAKE_SOURCE_DIR}/resources/tessdata")

# Fichiers attendus après téléchargement
set(TESSDATA_STAMP "${CMAKE_BINARY_DIR}/tessdata_downloaded.stamp")

add_custom_command(
        OUTPUT "${TESSDATA_STAMP}"
        COMMAND ${PYTHON_EXEC}
        "${CMAKE_SOURCE_DIR}/scripts/download_tessdata.py"
        "${TESSDATA_DIR}"
        COMMAND ${CMAKE_COMMAND} -E touch "${TESSDATA_STAMP}"
        DEPENDS "${CMAKE_SOURCE_DIR}/scripts/download_tessdata.py"
        COMMENT "Vérification/téléchargement des modèles Tesseract (tessdata)..."
        VERBATIM
)

add_custom_target(download_tessdata ALL
        DEPENDS "${TESSDATA_STAMP}"
)

# ── 3. Sources ────────────────────────────────────────────────────────────────
set(SOURCES
        main.cpp
        config.cpp

        core/TextBlock.cpp
        core/BubbleDetector.cpp
        core/OCRManager.cpp
        core/Exporter.cpp
        core/ProjectManager.cpp

        gui/canvas/ImageCanvas.cpp

        windows/main/MainWindow.cpp
        windows/image/ImageWindow.cpp
        windows/text/TextWindow.cpp
        windows/project/NewProjectDialog.cpp
        windows/settings/SettingsWindow.cpp
)

set(HEADERS
        config.h

        core/TextBlock.h
        core/BubbleDetector.h
        core/OCRManager.h
        core/OCRConfig.h
        core/Exporter.h
        core/ProjectManager.h
        core/models.h

        gui/canvas/ImageCanvas.h

        windows/main/MainWindow.h
        windows/image/ImageWindow.h
        windows/text/TextWindow.h
        windows/project/NewProjectDialog.h
        windows/settings/SettingsWindow.h
)

set(UIS
        windows/main/MainWindow.ui
        windows/image/ImageWindow.ui
        windows/text/TextWindow.ui
        windows/project/NewProjectDialog.ui
        windows/settings/SettingsWindow.ui
)

# ── 4. Icônes selon la plateforme ─────────────────────────────────────────────
set(APP_ICON_SOURCES "")

if(APPLE)
    foreach(_icns "${ICNS_ICON}" "${ICNS_PROJECT}")
        if(EXISTS "${_icns}")
            list(APPEND APP_ICON_SOURCES "${_icns}")
            set_source_files_properties("${_icns}" PROPERTIES
                MACOSX_PACKAGE_LOCATION "Resources")
        endif()
    endforeach()
endif()

if(WIN32)
    if(EXISTS "${ICO_ICON}")
        set(RC_FILE "${CMAKE_BINARY_DIR}/app_icon.rc")
        file(WRITE "${RC_FILE}" "IDI_ICON1 ICON \"${ICO_ICON}\"\n")
        list(APPEND APP_ICON_SOURCES "${RC_FILE}")
    else()
        message(STATUS "ToonTrad.ico sera généré au 1er build")
    endif()
endif()

# ── 5. Exécutable ─────────────────────────────────────────────────────────────
qt_add_executable(ToonTrad
    ${SOURCES}
    ${HEADERS}
    ${UIS}
    ${APP_ICON_SOURCES}
)
add_dependencies(ToonTrad generate_icons download_tessdata)

# ── 6. Bundle macOS ───────────────────────────────────────────────────────────
if(APPLE)
    set_target_properties(ToonTrad PROPERTIES
        MACOSX_BUNDLE                      TRUE
        MACOSX_BUNDLE_BUNDLE_NAME          "ToonTrad"
        MACOSX_BUNDLE_GUI_IDENTIFIER       "com.toontrad.app"
        MACOSX_BUNDLE_BUNDLE_VERSION       "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE            "ToonTrad"
        MACOSX_BUNDLE_INFO_PLIST           "${CMAKE_SOURCE_DIR}/resources/Info.plist.in"
    )
endif()

target_link_libraries(ToonTrad PRIVATE Qt6::Widgets Qt6::Network Qt6::Svg)
target_include_directories(ToonTrad PRIVATE ${CMAKE_SOURCE_DIR})

# ── 7. Post-build : copie python/ et resources/ ───────────────────────────────
if(APPLE)
    set(BUNDLE_CONTENTS "$<TARGET_BUNDLE_CONTENT_DIR:ToonTrad>")

    add_custom_command(TARGET ToonTrad POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/python" "${BUNDLE_CONTENTS}/python"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources" "${BUNDLE_CONTENTS}/resources"
        COMMENT "Copie python/ et resources/ dans le bundle"
    )

    # macdeployqt : uniquement en build local (la CI le gère elle-même)
    if(NOT DEFINED ENV{CI})
        find_program(MACDEPLOYQT macdeployqt HINTS "${Qt6_DIR}/../../../bin")
        if(MACDEPLOYQT)
            add_custom_command(TARGET ToonTrad POST_BUILD
                COMMAND ${MACDEPLOYQT}
                    "$<TARGET_BUNDLE_DIR:ToonTrad>"
                    -always-overwrite
                COMMENT "macdeployqt (local) — intègre Qt dans le bundle"
            )
        else()
            message(WARNING "macdeployqt introuvable")
        endif()
    else()
        message(STATUS "CI détecté — macdeployqt géré par GitHub Actions")
    endif()

else()

# ── 8. Création du venv Python avec les dépendances ──────────────────────────
set(VENV_DIR "${CMAKE_BINARY_DIR}/python_venv")
set(VENV_STAMP "${CMAKE_BINARY_DIR}/venv_installed.stamp")

add_custom_command(
        OUTPUT "${VENV_STAMP}"
        COMMAND ${PYTHON_EXEC} -m venv "${VENV_DIR}"
        COMMAND "${VENV_DIR}/bin/pip" install --upgrade pip --quiet
        COMMAND "${VENV_DIR}/bin/pip" install
        opencv-python-headless
        pytesseract
        pillow
        --quiet
        COMMAND ${CMAKE_COMMAND} -E touch "${VENV_STAMP}"
        DEPENDS "${CMAKE_SOURCE_DIR}/python/requirements.txt"
        COMMENT "Création du venv Python + installation des dépendances..."
        VERBATIM
)

add_custom_target(python_venv ALL DEPENDS "${VENV_STAMP}")
add_dependencies(ToonTrad python_venv)

if(APPLE)
    add_custom_command(TARGET ToonTrad POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/python" "${BUNDLE_CONTENTS}/python"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources" "${BUNDLE_CONTENTS}/resources"
            # Ajoute cette ligne :
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${VENV_DIR}" "${BUNDLE_CONTENTS}/python_venv"
            COMMENT "Copie python/, resources/ et venv dans le bundle"
    )
endif()

    # ── Windows / Linux ───────────────────────────────────────────────────────
    add_custom_command(TARGET ToonTrad POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/python"
            "$<TARGET_FILE_DIR:ToonTrad>/python"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:ToonTrad>/resources"
        COMMENT "Copie python/ et resources/ dans le répertoire de build"
    )

    # windeployqt en local uniquement
    if(WIN32 AND NOT DEFINED ENV{CI})
        find_program(WINDEPLOYQT windeployqt HINTS "${Qt6_DIR}/../../../bin")
        if(WINDEPLOYQT)
            add_custom_command(TARGET ToonTrad POST_BUILD
                COMMAND ${WINDEPLOYQT}
                    --no-translations --no-system-d3d-compiler
                    "$<TARGET_FILE:ToonTrad>"
                COMMENT "windeployqt (local) — copie les DLLs Qt"
            )
        endif()
    endif()
endif()

# ═════════════════════════════════════════════════════════════════════════════
#  INSTALL — règles utilisées par CPack
# ═════════════════════════════════════════════════════════════════════════════
if(APPLE)
    install(TARGETS ToonTrad
        BUNDLE  DESTINATION .
        RUNTIME DESTINATION bin
    )
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/python"
        DESTINATION "ToonTrad.app/Contents/Resources"
    )
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources"
        DESTINATION "ToonTrad.app/Contents/Resources"
    )
else()
    install(TARGETS ToonTrad RUNTIME DESTINATION .)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/python"    DESTINATION .)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources" DESTINATION .)
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

if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
endif()

# ── macOS : DMG drag-and-drop ─────────────────────────────────────────────────
if(APPLE)
    set(CPACK_GENERATOR         "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME   "ToonTrad ${PROJECT_VERSION}")
    set(CPACK_DMG_FORMAT        "UDZO")
    set(CPACK_PACKAGE_FILE_NAME "ToonTrad-${PROJECT_VERSION}-Darwin")
endif()

# ── Windows : NSIS installateur ───────────────────────────────────────────────
if(WIN32)
    set(CPACK_GENERATOR              "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME      "ToonTrad ${PROJECT_VERSION}")
    set(CPACK_NSIS_PACKAGE_NAME      "ToonTrad")
    set(CPACK_NSIS_INSTALL_ROOT      "$PROGRAMFILES64")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH       OFF)
    set(CPACK_PACKAGE_FILE_NAME      "ToonTrad-${PROJECT_VERSION}-win64")

    if(EXISTS "${ICO_ICON}")
        set(CPACK_NSIS_MUI_ICON    "${ICO_ICON}")
        set(CPACK_NSIS_MUI_UNIICON "${ICO_ICON}")
    endif()

    # Raccourci bureau + menu Démarrer
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortcut '$DESKTOP\\\\ToonTrad.lnk' '$INSTDIR\\\\ToonTrad.exe' '' '$INSTDIR\\\\ToonTrad.exe' 0
         CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\ToonTrad.lnk' '$INSTDIR\\\\ToonTrad.exe' '' '$INSTDIR\\\\ToonTrad.exe' 0"
    )
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$DESKTOP\\\\ToonTrad.lnk'
         Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\ToonTrad.lnk'"
    )

    # Association fichier .ttproject → double-clic ouvre ToonTrad
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
        WriteRegStr HKCR '.ttproject' '' 'ToonTrad.Project'
        WriteRegStr HKCR 'ToonTrad.Project' '' 'ToonTrad Project'
        WriteRegStr HKCR 'ToonTrad.Project\\\\DefaultIcon' '' '$INSTDIR\\\\ToonTrad.exe,0'
        WriteRegStr HKCR 'ToonTrad.Project\\\\shell\\\\open\\\\command' '' '\\\"$INSTDIR\\\\ToonTrad.exe\\\" \\\"%1\\\"'
        System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
    ")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
        DeleteRegKey HKCR '.ttproject'
        DeleteRegKey HKCR 'ToonTrad.Project'
    ")
endif()

include(CPack)

add_custom_target(package_dmg ALL
        COMMAND ${CMAKE_CPACK_COMMAND} -G DragNDrop
        DEPENDS ToonTrad
        COMMENT "Génération du DMG avec CPack"
)