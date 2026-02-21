#pragma once

class OBSSettingsState {
public:
    OBSSettingsState();
    void Draw();

    bool IsDirty() const { return m_dirty; }
    void Save() { SaveChanges(); SyncOriginals(); m_dirty = false; }

private:
    void SaveChanges() const;
    void SyncOriginals();
    void CheckDirty();

    // ───────────────────────────────────
    char m_host[128] = {};
    int  m_port      = 4455;
    // ───────────────────────────────────
    char m_origHost[128] = {};
    int  m_origPort      = 4455;
    // ───────────────────────────────────

    bool m_dirty = false;
};