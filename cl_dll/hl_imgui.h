#ifndef HL_IMGUI_H
#define HL_IMGUI_H

#include "SDL2\SDL.h"
#include "SDL2\SDL_opengl.h"
#include "..\imgui\imgui.h"
#include "..\imgui\imgui_impl_sdl.h"

const ImVec4 BGColor = ImVec4(0.29910269192f, 0.35227650382f, 0.26586905948f, 1.0f);
const ImVec4 DarkBGColor = ImVec4(0.23529411764f, 0.277124183f, 0.20915032679f, 1.0f);
const ImVec4 HoverBGColor = ImVec4(0.22058823529f, 0.25980392156f, 0.19607843137f, 1.0f);
const ImVec4 DarkerBGColor = ImVec4(0.22058823529f, 0.25980392156f, 0.19607843137f, 1.0f);
const ImVec4 HighlightColor = ImVec4(0.58431372549f, 0.53f, 0.19215686274f, 1.0f);
const ImVec4 SelectedColor = ImVec4(0.78431372549f, 0.73f, 0.09215686274f, 1.0f);

#define MAX_CHAPTERS 32

const int GAME_MODE_WINDOW_WIDTH = 895;
const int GAME_MODE_WINDOW_HEIGHT = 340;

void HL_ImGUI_Init();
void HL_ImGUI_Deinit();
void HL_ImGUI_Draw();
int HL_ImGUI_ProcessEvent( void *data, SDL_Event* event );

void ChapterSelectGUI_Init();
void ChapterSelectGUI_Draw();
void __CmdFunc_ChapterSelectGUI_ToggleMenu();

#endif // HL_IMGUI_H