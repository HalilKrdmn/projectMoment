#include "gui/core/MainWindow.h"

#include "gui/IconsCustom.h"
#include "gui/Theme.h"
#include "gui/screens/main/MainScreen.h"
#include "gui/screens/settings/SettingsScreen.h"
#include "gui/screens/editing/EditingScreen.h"

#include <cstdio>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "glad/glad.h"

static void error_callback([[maybe_unused]] int error, const char* description) {
    fprintf(stderr, "GLFW Error: %s\n", description);
}

MainWindow::MainWindow(const int width, const int height, const char* title) 
    : window(nullptr)
      , m_currentState(ApplicationState::MAIN) {
    
    // GLFW error callback
    glfwSetErrorCallback(error_callback);

    // GLFW initialize
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return;
    }

    // OpenGL version hints (3.3 Core)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);
    
    // VSYNC enable
    glfwSwapInterval(1);

    // Initialize GLAD
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Apply theme
    Theme::Apply();

    // Add fonts
    io.Fonts->AddFontDefault();

    ImFontConfig config;
    config.MergeMode = true;
    config.PixelSnapH = true;
    config.GlyphMinAdvanceX = 32.0f;
    config.GlyphOffset.y = 11.0f;
    config.GlyphOffset.x = -7.0f;

    static constexpr ImWchar icon_ranges[] = { ICON_MIN_CUS, ICON_MAX_CUS, 0 };
    io.Fonts->AddFontFromFileTTF("../include/gui/IconsCustom.ttf", 32.0f, &config, icon_ranges);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Create first screen
    SetApplicationState(ApplicationState::MAIN);

    printf("MainWindow initialized successfully\n");
}

MainWindow::~MainWindow() {
    // Screen cleanup
    if (m_mainScreen) delete m_mainScreen;
    if (m_editingScreen) delete m_editingScreen;

    // ImGui cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // GLFW cleanup
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
    
    printf("[MainWindow] MainWindow destroyed\n");
}

int MainWindow::Run() const {
    if (window == nullptr) {
        fprintf(stderr, "[MainWindow] Window is null, cannot run\n");
        return 1;
    }

    printf("[MainWindow] Starting main loop...\n");

    constexpr auto& bg = Theme::BG_CONTENT;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll events
        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Clear screen
        glClearColor(bg.x, bg.y, bg.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw current screen
        if (m_currentScreen != nullptr) {
            m_currentScreen->Draw();
        }

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers
        glfwSwapBuffers(window);
    }

    printf("[MainWindow] Main loop ended\n");
    return 0;
}

void MainWindow::SetApplicationState(const ApplicationState newState) {
    m_currentState = newState;

    switch (newState) {
        case ApplicationState::MAIN:
            if (!m_mainScreen)
                m_mainScreen = new MainScreen(this);
            m_currentScreen = m_mainScreen;
            break;

        case ApplicationState::SETTINGS:
            if (!m_settingsScreen)
                m_settingsScreen = new SettingsScreen(this);
            m_currentScreen = m_settingsScreen;
            break;

        case ApplicationState::EDITING:
            if (!m_editingScreen)
                m_editingScreen = new EditingScreen(this);
            m_currentScreen = m_editingScreen;
            break;

        default:
            if (!m_mainScreen)
                m_mainScreen = new MainScreen(this);
            m_currentScreen = m_mainScreen;
            break;
    }

    printf("Application state changed to: %d\n", static_cast<int>(newState));
}

void MainWindow::SwitchToEditingScreen(const VideoInfo& video) {
    // Store selected video
    m_selectedVideo = video;

    printf("[MainWindow] Switching to EDITING screen with video: %s\n", video.name.c_str());

    // Switch state
    SetApplicationState(ApplicationState::EDITING);
}