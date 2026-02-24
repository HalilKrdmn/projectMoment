#pragma once

#include "gui/core/BaseScreen.h"

#include <memory>

// ─────────────────────────────────────────────────────────────────────────────
class GeneralSettingsState;
class RecordingSettingsState;
class OBSSettingsState;
class NativeRecordingSettingsState;

enum class SettingsSection {
    GENERAL,
    RECORDING,
    OBS,
    NATIVE_RECORDING
};
// ──────────────────────────────────────────────────────────────────────────

class SettingsScreen : public BaseScreen {
public:
    explicit SettingsScreen(MainWindow* manager);

    void Draw() override;

private:
    void DrawSidebar();
    void DrawContent() const;

    void SelectSection(SettingsSection section);

    bool IsCurrentDirty() const;
    void SaveCurrent() const;

    // States
    std::unique_ptr<GeneralSettingsState>         m_generalState;
    std::unique_ptr<RecordingSettingsState>       m_recordingState;
    std::unique_ptr<OBSSettingsState>             m_obsState;
    std::unique_ptr<NativeRecordingSettingsState> m_nativeState;

    SettingsSection m_currentSection = SettingsSection::GENERAL;

    // Sidebar animation
    float m_selectorY       = 0.0f;
    float m_selectorTargetY = 0.0f;
    float m_itemsStartY     = 0.0f;
    bool  m_selectorReady   = false;

    static constexpr float SIDEBAR_WIDTH = 220.0f;
    static constexpr float ITEM_HEIGHT   = 44.0f;
    static constexpr float ANIM_SPEED    = 14.0f;
};