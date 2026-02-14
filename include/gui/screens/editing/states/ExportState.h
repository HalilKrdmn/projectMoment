#pragma once

class EditingScreen;

class ExportState {
public:
    ExportState() = default;

    void Draw(EditingScreen* parent);

private:
    int m_quality = 75;
};