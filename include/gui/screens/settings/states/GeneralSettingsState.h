#pragma once

#include "gui/widgets/FolderBrowser.h"

class GeneralSettingsState {
public:
    GeneralSettingsState();
    void Draw();

    bool IsDirty() const { return m_dirty; }
    void Save()          { SaveChanges(); SyncOriginals(); m_dirty = false; }

private:
    void LoadFromConfig();
    void SaveChanges() const;
    void SyncOriginals();
    void CheckDirty();

    // ───────────────────────────────────
    char m_libraryPath[512] = {};
    bool m_autoStartBuffer  = false;
    bool m_startMinimized   = false;
    // ───────────────────────────────────
    char m_origLibraryPath[512] = {};
    bool m_origAutoStartBuffer  = false;
    bool m_origStartMinimized   = false;
    // ───────────────────────────────────

    FolderBrowser m_folderBrowser;

    bool m_dirty = false;
};
