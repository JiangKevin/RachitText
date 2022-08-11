#include <stdio.h>
#include <string>
#include <SDL2/SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <SDL2/SDL_syswm.h>
#include "imgui.h"
#include "bgfx-imgui/imgui_impl_bgfx.h"
#include "sdl-imgui/imgui_impl_sdl.h"
#include "imgui-extensions/TextEditor.h"
#include "file-ops.h"

static bgfx::ShaderHandle createShader(
    const std::string& shader, const char* name)
{
    const bgfx::Memory* mem = bgfx::copy(shader.data(), shader.size());
    const bgfx::ShaderHandle handle = bgfx::createShader(mem);
    bgfx::setName(handle, name);
    return handle;
}

SDL_Window* window = NULL;
const int WIDTH = 800;
const int HEIGHT = 600;
int main (int argc, char* args[]) {
    // Initialize SDL systems
    if (SDL_Init( SDL_INIT_VIDEO ) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n",
                SDL_GetError());
    }
    else {
        // Create a window
         window = SDL_CreateWindow("RachitText++",
                    SDL_WINDOWPOS_UNDEFINED,
                    SDL_WINDOWPOS_UNDEFINED,
                    WIDTH, HEIGHT,
                    SDL_WINDOW_SHOWN);
        if(window == NULL) {
            printf("Window could not be created! SDL_Error: %s\n",
                   SDL_GetError());
        }
    }

    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(window, &wmi)) {
        printf(
            "SDL_SysWMinfo could not be retrieved. SDL_Error: %s\n",
            SDL_GetError());
        return 1;
    }
    bgfx::renderFrame(); // single threaded mode

    bgfx::PlatformData pd{};
#if BX_PLATFORM_WINDOWS
    pd.nwh = wmi.info.win.window;
#elif BX_PLATFORM_OSX
    pd.nwh = wmi.info.cocoa.window;
#elif BX_PLATFORM_LINUX
    pd.ndt = wmi.info.x11.display;
    pd.nwh = (void*)(uintptr_t)wmi.info.x11.window;
#endif

    bgfx::Init bgfx_init;
    bgfx_init.type = bgfx::RendererType::Count; // auto choose renderer
    bgfx_init.resolution.width = WIDTH;
    bgfx_init.resolution.height = HEIGHT;
    bgfx_init.resolution.reset = BGFX_RESET_VSYNC;
    bgfx_init.platformData = pd;
    bgfx::init(bgfx_init);

    bgfx::setViewClear(
        0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x6495EDFF, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, WIDTH, HEIGHT);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui_Implbgfx_Init(255);
#if BX_PLATFORM_WINDOWS
    ImGui_ImplSDL2_InitForD3D(window);
#elif BX_PLATFORM_OSX
    ImGui_ImplSDL2_InitForMetal(window);
#elif BX_PLATFORM_LINUX
    ImGui_ImplSDL2_InitForOpenGL(window, nullptr);
#endif

    std::string vshader;
    if (!fileops::read_file("shader/v_simple.bin", vshader)) {
        return 1;
    }

    std::string fshader;
    if (!fileops::read_file("shader/f_simple.bin", fshader)) {
        return 1;
    }

    bgfx::ShaderHandle vsh = createShader(vshader, "vshader");
    bgfx::ShaderHandle fsh = createShader(fshader, "fshader");
    bgfx::ProgramHandle program = bgfx::createProgram(vsh, fsh, true);

    TextEditor editor;
    auto lang = TextEditor::LanguageDefinition::CPlusPlus();
    editor.SetLanguageDefinition(lang);
    static const char* fileToEdit = "main.cpp";
    {
        std::ifstream t(fileToEdit);
        if (t.good())
        {
            std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
            editor.SetText(str);
        }
    }

    // Poll for events and wait till user closes window
    bool quit = false;
    SDL_Event currentEvent;
    while (!quit) {
        while (SDL_PollEvent(&currentEvent) != 0) {
            ImGui_ImplSDL2_ProcessEvent(&currentEvent);
            switch(currentEvent.type) {
                case  SDL_QUIT:
                    quit = true;
                    break;
            }
        }
        ImGui_Implbgfx_NewFrame();
        ImGui_ImplSDL2_NewFrame();

        ImGui::NewFrame();
        
        auto cpos = editor.GetCursorPosition();
        ImGui::SetNextWindowSize(ImVec2(WIDTH, HEIGHT));
        ImGui::SetNextWindowPos(ImVec2(0, 0)); 
        ImGui::Begin("RachitText++", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize);
        ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Save"))
                {
                    auto textToSave = editor.GetText();
                    std::ofstream f(fileToEdit);
                    f << textToSave;
                    f.close();
                    editor.SetText(textToSave);

                }
                if (ImGui::MenuItem("Quit", "Alt-F4"))
                    break;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                bool ro = editor.IsReadOnly();
                if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
                    editor.SetReadOnly(ro);
                ImGui::Separator();

                if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && editor.CanUndo()))
                    editor.Undo();
                if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && editor.CanRedo()))
                    editor.Redo();

                ImGui::Separator();

                if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
                    editor.Copy();
                if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && editor.HasSelection()))
                    editor.Cut();
                if (ImGui::MenuItem("Delete", "Del", nullptr, !ro && editor.HasSelection()))
                    editor.Delete();
                if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
                    editor.Paste();

                ImGui::Separator();

                if (ImGui::MenuItem("Select all", nullptr, nullptr))
                    editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("Dark palette"))
                    editor.SetPalette(TextEditor::GetDarkPalette());
                if (ImGui::MenuItem("Light palette"))
                    editor.SetPalette(TextEditor::GetLightPalette());
                if (ImGui::MenuItem("Retro blue palette"))
                    editor.SetPalette(TextEditor::GetRetroBluePalette());
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(),
            editor.IsOverwrite() ? "Ovr" : "Ins",
            editor.CanUndo() ? "*" : " ",
            editor.GetLanguageDefinition().mName.c_str(), fileToEdit);

        editor.Render("TextEditor");
        ImGui::End();

        ImGui::Render();
        ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());

        bgfx::submit(0, program);

        bgfx::frame();
    }

    ImGui_ImplSDL2_Shutdown();
    ImGui_Implbgfx_Shutdown();

    ImGui::DestroyContext();
    bgfx::shutdown();

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}