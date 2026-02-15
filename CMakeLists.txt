cmake_minimum_required(VERSION 4.0)
project(projectMoment VERSION 0.0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(vendor)

set(CORE_SOURCES
        src/main.cpp
        src/core/Config.cpp
        include/core/Config.h
        src/core/CoreServices.cpp
        include/core/CoreServices.h
        include/core/ProjectPaths.h
        include/data/VideoInfo.h
)

set(LIBRARY_SOURCES
        src/core/library/VideoDatabase.cpp
        include/core/library/VideoDatabase.h
        src/core/library/VideoLibrary.cpp
        include/core/library/VideoLibrary.h
        src/core/library/LibraryLoader.cpp
        include/core/library/LibraryLoader.h
)

set(MEDIA_SOURCES
        src/core/media/AudioAnalyzer.cpp
        include/core/media/AudioAnalyzer.h
        src/core/import/VideoImportService.cpp
        include/core/import/VideoImportService.h
        src/core/media/MetadataEmbedder.cpp
        include/core/media/MetadataEmbedder.h
        src/core/media/ThumbnailService.cpp
        include/core/media/ThumbnailService.h
        src/core/media/VideoPlayer.cpp
        include/core/media/VideoPlayer.h
)

set(GUI_CORE_SOURCES
        src/gui/core/MainWindow.cpp
        include/gui/core/MainWindow.h
        include/gui/core/BaseScreen.h
        src/gui/widgets/FolderBrowser.cpp
        include/gui/widgets/FolderBrowser.h
        src/gui/utils/FormatUtils.cpp
        include/gui/utils/FormatUtils.h
        src/gui/utils/ThumbnailLoader.cpp
        include/gui/utils/ThumbnailLoader.h
)

set(GUI_MAIN_SCREENS_SOURCES
        src/gui/screens/main/MainScreen.cpp
        include/gui/screens/main/MainScreen.h

        src/gui/screens/main/states/EmptyFolderState.cpp
        include/gui/screens/main/states/EmptyFolderState.h
        src/gui/screens/main/states/LoadingState.cpp
        include/gui/screens/main/states/LoadingState.h
        src/gui/screens/main/states/VideoListState.cpp
        include/gui/screens/main/states/VideoListState.h
        src/gui/screens/main/states/WelcomeState.cpp
        include/gui/screens/main/states/WelcomeState.h
)

set(GUI_EDITING_SCREENS_SOURCES
        src/gui/screens/editing/EditingScreen.cpp
        include/gui/screens/editing/EditingScreen.h

        src/gui/screens/editing/ExportWidget.cpp
        include/gui/screens/editing/ExportWidget.h

        src/gui/screens/editing/states/VideoEditState.cpp
        include/gui/screens/editing/states/VideoEditState.h
)

add_executable(projectMoment
        ${CORE_SOURCES}
        ${LIBRARY_SOURCES}
        ${MEDIA_SOURCES}
        ${GUI_CORE_SOURCES}
        ${GUI_MAIN_SCREENS_SOURCES}
        ${GUI_EDITING_SCREENS_SOURCES}
)

target_include_directories(projectMoment PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(SQLite3)
find_package(PkgConfig REQUIRED)

pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil libswscale)

include_directories(
        ${CMAKE_SOURCE_DIR}/vendor/stb
)

target_link_libraries(projectMoment PRIVATE
        ImGui
        glad
        glfw
        OpenGL::GL
        PkgConfig::FFMPEG
        swresample
)

if(TARGET SQLite::SQLite3)
    target_link_libraries(projectMoment PRIVATE SQLite::SQLite3)
else()
    target_link_libraries(projectMoment PRIVATE ${SQLite3_LIBRARIES})
    target_include_directories(projectMoment PRIVATE ${SQLite3_INCLUDE_DIRS})
endif()