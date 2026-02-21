#pragma once

class RecordingSettingsState {
public:
    RecordingSettingsState();
    void Draw();

    bool IsDirty() const { return m_dirty; }
    void Save() { SaveChanges(); SyncOriginals(); m_dirty = false; }

private:
    void SaveChanges() const;
    void SyncOriginals();
    void CheckDirty();

    // ───────────────────────────────────
    int  m_modeIndex              = 0;
    char m_hotkeyRecordToggle[64] = {};
    char m_hotkeySaveClip[64]     = {};
    char m_hotkeyToggleMic[64]    = {};
    // ───────────────────────────────────
    int  m_origModeIndex              = 0;
    char m_origHotkeyRecordToggle[64] = {};
    char m_origHotkeySaveClip[64]     = {};
    char m_origHotkeyToggleMic[64]    = {};
    // ───────────────────────────────────

    bool m_dirty = false;
};