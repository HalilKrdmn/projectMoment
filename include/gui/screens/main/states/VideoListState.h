#pragma once

#include "gui/core/MainWindow.h"

class MainScreen;

class VideoListState {
public:
    VideoListState() = default;
    
    void Draw(MainScreen* parent);

    static void LoadThumbnails(MainScreen* parent);
    
private:
    bool m_thumbnailsLoaded = false;

    static void DrawHeader(MainScreen* parent);
    static void DrawVideoGrid(MainScreen* parent);

    static MainWindow* GetMainWindow(const MainScreen* parent);
};