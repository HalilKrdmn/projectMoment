#pragma once

#include "imgui.h"

struct Theme {
    // ─── Shared Top Bar Constants  ──────────────────────────────────────────
    static constexpr float TOPBAR_H       = 52.0f;
    static constexpr float TOPBAR_BTN_W   = 32.0f;
    static constexpr float TOPBAR_BTN_H   = 32.0f;
    static constexpr float TOPBAR_BTN_PAD = 10.0f;
    static constexpr float REC_BTN_W      = 155.0f;
    static constexpr float CLIP_BTN_W     = 120.0f;

    // ── Colors ──────────────────────────────────────────────────────────────
    static constexpr ImVec4 BG_DARK        = { 0.08f, 0.08f, 0.10f, 1.0f };
    static constexpr ImVec4 BG_CONTENT     = { 0.11f, 0.11f, 0.13f, 1.0f };
    static constexpr ImVec4 ACCENT         = { 0.20f, 0.50f, 0.90f, 1.0f };
    static constexpr ImVec4 ACCENT_HOVER   = { 0.25f, 0.58f, 1.00f, 1.0f };
    static constexpr ImVec4 ACCENT_ACTIVE  = { 0.15f, 0.40f, 0.75f, 1.0f };
    static constexpr ImVec4 DANGER         = { 0.72f, 0.18f, 0.18f, 1.0f };
    static constexpr ImVec4 DANGER_HOVER   = { 0.85f, 0.22f, 0.22f, 1.0f };
    static constexpr ImVec4 SUCCESS        = { 0.20f, 0.75f, 0.30f, 1.0f };
    static constexpr ImVec4 TEXT_PRIMARY   = { 0.90f, 0.90f, 0.95f, 1.0f };
    static constexpr ImVec4 TEXT_MUTED     = { 0.50f, 0.50f, 0.56f, 1.0f };
    static constexpr ImVec4 SEPARATOR      = { 0.22f, 0.22f, 0.28f, 1.0f };
    static constexpr ImVec4 BTN_NEUTRAL    = { 0.16f, 0.16f, 0.20f, 1.0f };
    static constexpr ImVec4 BTN_HOVER      = { 0.22f, 0.22f, 0.28f, 1.0f };
    static constexpr ImVec4 BTN_ACTIVE     = { 0.12f, 0.12f, 0.16f, 1.0f };

    static constexpr ImU32  SEPARATOR_LINE = IM_COL32(55,  55,  70,  200);
    static constexpr ImU32  SELECTOR_BG    = IM_COL32(40,  90,  180, 100);

    // ─── Timeline ────────────────────────────────────────────────────────────
    // Track backgrounds
    static constexpr ImU32  TL_CLIP_BG         = IM_COL32( 14,  20,  38, 255);  // Clip row
    static constexpr ImU32  TL_EMPTY_BG         = IM_COL32( 14,  14,  28, 255); // Filler row
    static constexpr ImU32  TL_SILENT_BG        = IM_COL32( 12,  12,  20, 255); // Silent / no stream
    static constexpr ImU32  TL_INPUT_BG         = IM_COL32( 36,  18,   6, 255); // Input (orange tint)
    static constexpr ImU32  TL_OUTPUT_BG        = IM_COL32(  6,  18,  38, 255); // Output (blue tint)
    static constexpr ImU32  TL_UNKNOWN_BG       = IM_COL32( 20,  10,  36, 255); // Unknown (purple tint)

    // Waveform colors
    static constexpr ImU32  TL_WAVE_INPUT       = IM_COL32(255, 155,  55, 255); // Orange — mic
    static constexpr ImU32  TL_WAVE_OUTPUT      = IM_COL32( 65, 175, 255, 255); // Blue   — loopback
    static constexpr ImU32  TL_WAVE_UNKNOWN     = IM_COL32(110,  55, 185, 255); // Dark purple

    // Track borders
    static constexpr ImU32  TL_BORDER           = IM_COL32( 45,  45,  75, 200);
    static constexpr ImU32  TL_CLIP_BORDER      = IM_COL32( 50,  60, 100, 255);
    static constexpr ImU32  TL_EMPTY_BORDER     = IM_COL32( 50,  50,  80, 200);

    // Labels
    static constexpr ImU32  TL_LABEL            = IM_COL32(200, 200, 215, 255);
    static constexpr ImU32  TL_LABEL_SILENT     = IM_COL32( 70,  70,  90, 200);
    static constexpr ImU32  TL_LABEL_CLIP       = IM_COL32(200, 200, 220, 255);
    static constexpr ImU32  TL_LABEL_LOADING    = IM_COL32( 90,  90, 110, 180);

    // Separator between label column and waveform
    static constexpr ImU32  TL_COL_SEP          = IM_COL32( 70,  90, 130, 200);

    // Center line (flat / silent track)
    static constexpr ImU32  TL_CENTER_LINE      = IM_COL32( 50,  50,  72, 160);
    static constexpr ImU32  TL_CLIP_CENTER_LINE = IM_COL32( 60,  60, 100, 120);

    // Selection
    static constexpr ImU32  TL_SEL_DIM          = IM_COL32(  0,   0,   0, 130); // outside selection
    static constexpr ImU32  TL_SEL_DIM_CLIP     = IM_COL32(  0,   0,   0, 150);

    // Playhead
    static constexpr ImU32  TL_PLAYHEAD         = IM_COL32(255, 255,   0, 255); // yellow

    // Selection handles (bracket [ ])
    static constexpr ImU32  TL_HANDLE           = IM_COL32(  0, 225, 225, 255); // cyan

    // ── Apply the Global ImGui style ─────────────────────────────────────────
    static void Apply() {
        ImGuiStyle& s = ImGui::GetStyle();
        s.WindowRounding    = 6.0f;
        s.FrameRounding     = 4.0f;
        s.PopupRounding     = 4.0f;
        s.ScrollbarRounding = 4.0f;
        s.GrabRounding      = 4.0f;
        s.TabRounding       = 4.0f;
        s.WindowBorderSize  = 0.0f;
        s.FrameBorderSize   = 0.0f;
        s.PopupBorderSize   = 1.0f;
        s.ItemSpacing       = { 8.0f,  6.0f };
        s.FramePadding      = { 8.0f,  4.0f };
        s.WindowPadding     = { 12.0f, 12.0f };
        s.ScrollbarSize     = 10.0f;

        ImVec4* c = s.Colors;
        c[ImGuiCol_WindowBg]             = BG_CONTENT;
        c[ImGuiCol_ChildBg]              = BG_CONTENT;
        c[ImGuiCol_PopupBg]              = BG_DARK;
        c[ImGuiCol_FrameBg]              = BTN_NEUTRAL;
        c[ImGuiCol_FrameBgHovered]       = BTN_HOVER;
        c[ImGuiCol_FrameBgActive]        = BTN_ACTIVE;
        c[ImGuiCol_TitleBg]              = BG_DARK;
        c[ImGuiCol_TitleBgActive]        = BG_DARK;
        c[ImGuiCol_Button]               = BTN_NEUTRAL;
        c[ImGuiCol_ButtonHovered]        = BTN_HOVER;
        c[ImGuiCol_ButtonActive]         = BTN_ACTIVE;
        c[ImGuiCol_Header]               = { ACCENT.x, ACCENT.y, ACCENT.z, 0.4f };
        c[ImGuiCol_HeaderHovered]        = { ACCENT.x, ACCENT.y, ACCENT.z, 0.6f };
        c[ImGuiCol_HeaderActive]         = ACCENT;
        c[ImGuiCol_SliderGrab]           = ACCENT;
        c[ImGuiCol_SliderGrabActive]     = ACCENT_ACTIVE;
        c[ImGuiCol_CheckMark]            = ACCENT;
        c[ImGuiCol_Separator]            = SEPARATOR;
        c[ImGuiCol_SeparatorHovered]     = SEPARATOR;
        c[ImGuiCol_SeparatorActive]      = SEPARATOR;
        c[ImGuiCol_Text]                 = TEXT_PRIMARY;
        c[ImGuiCol_TextDisabled]         = TEXT_MUTED;
        c[ImGuiCol_ScrollbarBg]          = BG_DARK;
        c[ImGuiCol_ScrollbarGrab]        = BTN_NEUTRAL;
        c[ImGuiCol_ScrollbarGrabHovered] = BTN_HOVER;
        c[ImGuiCol_ScrollbarGrabActive]  = BTN_ACTIVE;
        c[ImGuiCol_Tab]                  = BTN_NEUTRAL;
        c[ImGuiCol_TabHovered]           = BTN_HOVER;
        c[ImGuiCol_TabActive]            = ACCENT;
        c[ImGuiCol_ResizeGrip]           = BTN_NEUTRAL;
        c[ImGuiCol_ResizeGripHovered]    = ACCENT;
        c[ImGuiCol_ResizeGripActive]     = ACCENT_ACTIVE;
    }
};