#pragma once

#include "gui/core/BaseScreen.h"

#include <memory>


struct VideoInfo;

class VideoEditState;
class ExportWidget;

enum class EditingScreenState {
    VIDEO_EDIT
};


class EditingScreen : public BaseScreen {
public:
    explicit EditingScreen(MainWindow* manager);

    void Draw() override;

    void ChangeState(const EditingScreenState newState) {m_currentState = newState;}
    const char *GetCurrentWindowName() const;

    mutable bool m_showExportWidget = false;
private:
    EditingScreenState m_currentState = EditingScreenState::VIDEO_EDIT;

    std::unique_ptr<VideoEditState> m_videoEditState;
    std::unique_ptr<ExportWidget> m_exportWidget;
};