cmake_minimum_required(VERSION 3.16)
project(ToonTrad VERSION 1.0 LANGUAGES CXX)

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
set(SVG_ICON     "${CMAKE_SOURCE_DIR}/resources/toontrad-icon-red-double-t.svg")
set(ICO_ICON     "${CMAKE_SOURCE_DIR}/resources/ToonTrad.ico")
set(ICNS_ICON    "${CMAKE_SOURCE_DIR}/resources/ToonTrad.icns")
set(ICNS_PROJECT "${CMAKE_SOURCE_DIR}/resources/ttproject.icns")

add_custom_command(
    OUTPUT "${ICO_ICON}" "${ICNS_ICON}" "${ICNS_PROJECT}"
    COMMAND ${PYTHON_EXEC}
        "${CMAKE_SOURCE_DIR}/scripts/generate_icons.py"
        "${SVG_ICON}"
        "${ICO_ICON}"
        "${ICNS_ICON}"
        "${ICNS_PROJECT}"
    DEPENDS "${SVG_ICON}" "${CMAKE_SOURCE_DIR}/scripts/generate_icons.py"
    COMMENT "Génération des icônes .ico / .icns depuis le SVG..."
)
add_custom_target(generate_icons
    DEPENDS "${ICO_ICON}" "${ICNS_ICON}" "${ICNS_PROJECT}"
)

# ── 2. Téléchargement tessdata ────────────────────────────────────────────────
set(TESSDATA_DIR "${CMAKE_SOURCE_DIR}/resources/tessdata")
add_custom_target(download_tessdata ALL
    COMMAND ${PYTHON_EXEC}
        "${CMAKE_SOURCE_DIR}/scripts/download_tessdata.py"
        "${TESSDATA_DIR}"
    COMMENT "Vérification/téléchargement des modèles Tesseract..."
    VERBATIM
)

# ── 3. Sources ────────────────────────────────────────────────────────────────
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
        message(STATUS "ToonTrad.ico pas encore généré — sera créé au 1er build")
    endif()
endif()

# ── 5. Exécutable ─────────────────────────────────────────────────────────────
qt_add_executable(ToonTrad
    ${SOURCES}
    ${APP_ICON_SOURCES}
)
add_dependencies(ToonTrad generate_icons download_tessdata)

# ── 6. Bundle macOS ───────────────────────────────────────────────────────────
if(APPLE)
    set_target_properties(ToonTrad PROPERTIES
        MACOSX_BUNDLE                      TRUE
        MACOSX_BUNDLE_BUNDLE_NAME          "ToonTrad"
        MACOSX_BUNDLE_BUNDLE_VERSION       "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_GUI_IDENTIFIER       "com.toontrad.app"
        MACOSX_BUNDLE_ICON_FILE            "ToonTrad"
        MACOSX_BUNDLE_INFO_PLIST           "${CMAKE_SOURCE_DIR}/resources/Info.plist.in"
    )
endif()

target_link_libraries(ToonTrad PRIVATE Qt6::Widgets Qt6::Network Qt6::Svg)
target_include_directories(ToonTrad PRIVATE ${CMAKE_SOURCE_DIR})

# ── 7. Post-build : copie des dépendances ─────────────────────────────────────
if(APPLE)
    set(BUNDLE_CONTENTS "$<TARGET_BUNDLE_CONTENT_DIR:ToonTrad>")

    add_custom_command(TARGET ToonTrad POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/python" "${BUNDLE_CONTENTS}/python"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources" "${BUNDLE_CONTENTS}/resources"
    )

    find_program(MACDEPLOYQT macdeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(MACDEPLOYQT)
        add_custom_command(TARGET ToonTrad POST_BUILD
            COMMAND ${MACDEPLOYQT}
                "$<TARGET_BUNDLE_DIR:ToonTrad>"
                -always-overwrite
            COMMENT "macdeployqt — intègre Qt dans le bundle"
        )
    else()
        message(WARNING "macdeployqt introuvable")
    endif()

else()
    # ── Windows / Linux ───────────────────────────────────────────────────────
    set(STAGING "${CMAKE_BINARY_DIR}/staging")

    if(WIN32)
        find_program(WINDEPLOYQT windeployqt HINTS "${Qt6_DIR}/../../../bin")
    endif()

    if(WIN32 AND WINDEPLOYQT)
        add_custom_command(TARGET ToonTrad POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${STAGING}/bin"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:ToonTrad>" "${STAGING}/bin/"
            COMMAND ${WINDEPLOYQT}
                --no-translations
                --no-system-d3d-compiler
                --dir "${STAGING}/bin"
                "$<TARGET_FILE:ToonTrad>"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/python" "${STAGING}/python"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/resources" "${STAGING}/resources"
            COMMENT "windeployqt + copie dépendances dans staging/"
        )
    else()
        add_custom_command(TARGET ToonTrad POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${STAGING}/bin"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:ToonTrad>" "${STAGING}/bin/"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/python" "${STAGING}/python"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/resources" "${STAGING}/resources"
        )
    endif()
endif()

# ══════════════════════════════════════════════════════════════════════════════
# CPack — Installeur final
# ══════════════════════════════════════════════════════════════════════════════
include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME          "ToonTrad")
set(CPACK_PACKAGE_VENDOR        "ToonTrad")
set(CPACK_PACKAGE_VERSION       "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Outil de scantrad manga/webtoon")

if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
endif()

# ── macOS : DMG drag-and-drop ─────────────────────────────────────────────────
if(APPLE)
    set(CPACK_GENERATOR  "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "ToonTrad")
    set(CPACK_DMG_FORMAT  "UDZO")

    install(TARGETS ToonTrad
        BUNDLE DESTINATION . COMPONENT Runtime
    )
endif()

# ── Windows : installeur NSIS ─────────────────────────────────────────────────
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_PACKAGE_NAME        "ToonTrad")
    set(CPACK_NSIS_DISPLAY_NAME        "ToonTrad")
    set(CPACK_NSIS_HELP_LINK           "https://github.com/trotroni/toontrad")
    set(CPACK_NSIS_URL_INFO_ABOUT      "https://github.com/trotroni/toontrad")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\ToonTrad.exe")

    if(EXISTS "${ICO_ICON}")
        set(CPACK_NSIS_MUI_ICON   "${ICO_ICON}")
        set(CPACK_NSIS_MUI_UNIICON "${ICO_ICON}")
    endif()

    # Raccourci menu Démarrer
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\ToonTrad.lnk' '$INSTDIR\\\\bin\\\\ToonTrad.exe' '' '$INSTDIR\\\\bin\\\\ToonTrad.exe' 0"
    )
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\ToonTrad.lnk'"
    )

    # Association fichier .ttproject → double-clic ouvre ToonTrad
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
        WriteRegStr HKCR '.ttproject' '' 'ToonTrad.Project'
        WriteRegStr HKCR 'ToonTrad.Project' '' 'ToonTrad Project'
        WriteRegStr HKCR 'ToonTrad.Project\\\\DefaultIcon' '' '$INSTDIR\\\\bin\\\\ToonTrad.exe,0'
        WriteRegStr HKCR 'ToonTrad.Project\\\\shell\\\\open\\\\command' '' '\\\"$INSTDIR\\\\bin\\\\ToonTrad.exe\\\" \\\"%1\\\"'
        System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
    ")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
        DeleteRegKey HKCR '.ttproject'
        DeleteRegKey HKCR 'ToonTrad.Project'
    ")

    # Installe tout ce qui est dans staging/ (exe + DLLs Qt + python + resources)
    install(DIRECTORY "${CMAKE_BINARY_DIR}/staging/"
        DESTINATION . COMPONENT Runtime
    )
endif()

include(CPack)
