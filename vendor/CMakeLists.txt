#Glad
add_library(glad STATIC
        glad/src/glad.c
)

set_source_files_properties(glad/src/glad.c PROPERTIES LANGUAGE C)

target_include_directories(glad PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/glad/include
)
#TomlPlusPlus
add_library(tomlplusplus INTERFACE)
target_include_directories(tomlplusplus INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/tomlplusplus)

#ImGui and ImGui-Filebrowser
add_library(ImGui STATIC
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp

        imgui/backends/imgui_impl_glfw.cpp
        imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(ImGui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui-filebrowser
)