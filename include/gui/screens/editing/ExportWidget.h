#pragma once


class EditingScreen;


class ExportWidget {
public:
    ExportWidget() = default;
    ~ExportWidget() = default;

    static void Draw(const EditingScreen* parent);
private:
    static void DrawDimBackground();

    int m_quality = 80;
};