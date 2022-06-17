#include "cl_dll.h"
#include <map>
#include <algorithm>
#include "cpp_aux.h"
#include "PlatformHeaders.h"
#include <filesystem>
#include "fs_aux.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "hl_imgui.h"

struct
{
	bool initialised{false};

	// Texture info
	GLuint texture;
	int width;
	int height;

	// Chapter related info
	char title[64];
	char mapname[64];
} ch_info[MAX_CHAPTERS];

extern SDL_Window* window;

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
	// Load from file
	int image_width = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
	if (image_data == NULL)
		return false;

	// Create a OpenGL texture identifier
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

	// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	stbi_image_free(image_data);

	*out_texture = image_texture;
	*out_width = image_width;
	*out_height = image_height;

	return true;
}

const char* ButtonText(const char *in)
{
	static char out[512] = {"\0"};
	strcpy(out, in);

	for (int i = strlen(in); i < 15; i++)
	{
		out[i] = ' ';
	}
	return out;
}

void ChapterSelectGUI_ParseChaptersFile()
{
	char *afile, *pfile;
	char token[128];
	int index = 0;

	afile = pfile = (char *)gEngfuncs.COM_LoadFile("resource/chapters.txt", 5, nullptr);

	if (!afile || !pfile)
		return;

	while (pfile = gEngfuncs.COM_ParseFile(pfile, token))
	{
		if (!stricmp(token, "title"))
		{
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			strcpy(ch_info[index].title, token);
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			strcpy(ch_info[index].mapname, token);
			index++;
		}
	}

	gEngfuncs.COM_FreeFile(afile);
	afile = pfile = nullptr;
}

void ChapterSelectGUI_LoadImageTextures()
{
	const char* chapterfolder = "chaptericons";
	const char* prefix = "ch";
	const char* ext = ".png";
	const char* gamedir = gEngfuncs.pfnGetGameDirectory();
	char fullpath[128];

	int savedchapteronpage = 0;

	for (int i = 0; i < MAX_CHAPTERS; i++)
	{
		sprintf(fullpath, ".\\%s\\%s\\%s%i%s", gamedir, chapterfolder, prefix, i + 1, ext);
		if (!LoadTextureFromFile(fullpath, &ch_info[i].texture, &ch_info[i].width, &ch_info[i].height))
		{
			if (i > savedchapteronpage)
				g_iNumPages++;
			break;
		}
		else
		{
			ch_info[i].initialised = true;
			g_iNumChapters++;
		}

		if (((i + 1) % 3) == 0)
		{
			g_iNumPages++;
			savedchapteronpage = i;
		}
	}
}

void ChapterSelectGUI_DeleteImageTextures()
{
	for (int i = 0; i < g_iNumChapters; i++)
	{
		glDeleteTextures(1, &ch_info[i].texture);
	}
}

int ChapterSelectGUI_WindowFlags()
{
	return ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_HorizontalScrollbar;
}

void __CmdFunc_ChapterSelectGUI_ToggleMenu()
{
	g_bMenuOpen = !g_bMenuOpen;
}

void ChapterSelectGUI_SetMenuState(bool state)
{
	g_bMenuOpen = state;
}

void ChapterSelectGUI_Init()
{
	g_iNumChapters = 0;
	g_iNumPages = -1;
	g_iCurPage = 0;
	g_iSelectedChapter = -1;
	g_bMenuOpen = false;
	g_iDifficulty = 0;

	ChapterSelectGUI_LoadImageTextures();
	ChapterSelectGUI_ParseChaptersFile();
}

void ChapterSelectGUI_DrawChapters()
{
	int start = 0;
	int end = 3;

	start = end * (g_iCurPage);
	end = start + 3;
	if (end > g_iNumChapters)
	{
		end = g_iNumChapters;
		ImGui::Columns(end - start, 0, false);
	}
	else
	{
		ImGui::Columns(3, 0, false);
	}

	for (int i = start; i < end; i++)
	{
		// Give proper column spacing
		if (i == start + 1)
			ImGui::SetColumnOffset(ImGui::GetColumnIndex(), (GAME_MODE_WINDOW_WIDTH - ch_info[i].width) / 2.075);

		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HighlightColor);
		if (i == g_iSelectedChapter)
			ImGui::PushStyleColor(ImGuiCol_Button, SelectedColor);
		else
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

		ImGui::TextColored(SelectedColor, "Chapter %i", i + 1);
		ImGui::Text(ch_info[i].title);

		ImGui::Spacing();

		if (ImGui::ImageButton((void*)(intptr_t)ch_info[i].texture, ImVec2(ch_info[i].width, ch_info[i].height)))
		{
			g_iSelectedChapter = i;
		}

		ImGui::PopStyleColor();
		ImGui::PopStyleColor();

		ImGui::NextColumn();
	}

	ImGui::Columns(1, 0, false);
}

void ChapterSelectGUI_DrawDifficultyComboBox()
{
	const char* items[] = {"Easy", "Medium", "Hard"};
	ImGui::TextColored(SelectedColor, "Difficulty: ");

	ImGui::SameLine();

	ImGui::PushItemWidth(ImGui::CalcTextSize("Medium").x * 1.7f);
	if (ImGui::Combo("", &g_iDifficulty, items, 3, -1))
	{
		gEngfuncs.Cvar_SetValue("skill", g_iDifficulty);
	}
	ImGui::PopItemWidth();
}

void ChapterSelectGUI_DrawButtons()
{
	if (g_iCurPage > 0)
	{
		if (ImGui::Button(ButtonText("Back")) || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow),false))
		{
			g_iCurPage--;
		}
		ImGui::SameLine();
	}

	ImGui::SetCursorPosX((GAME_MODE_WINDOW_WIDTH - ImGui::CalcTextSize(ButtonText("Next")).x) * 0.964);

	if (g_iCurPage < g_iNumPages)
	{
		if (ImGui::Button(ButtonText("Next")) || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow),false))
		{
			g_iCurPage++;
		}
	}
	else
	{
		ImGui::Spacing();
	}

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Spacing();

	ChapterSelectGUI_DrawDifficultyComboBox();
	ImGui::SameLine();

	ImGui::SetCursorPosX((GAME_MODE_WINDOW_WIDTH - ImGui::CalcTextSize(ButtonText("Cancel")).x) * 0.964);
	if (ImGui::Button(ButtonText("Cancel")) || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape), false))
	{
		g_bMenuOpen = false;
	}

	ImGui::SameLine();

	ImGui::SetCursorPosX((GAME_MODE_WINDOW_WIDTH - ImGui::CalcTextSize(ButtonText("Start")).x) * 0.85);
	if (ImGui::Button(ButtonText("Start")) || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter), false))
	{
		if (g_iSelectedChapter > -1)
		{
			char cmd[128];
			sprintf(cmd, "map %s", ch_info[g_iSelectedChapter].mapname);
			gEngfuncs.pfnClientCmd(cmd);
			g_bMenuOpen = false;
		}
	}
}

void ChapterSelectGUI_Draw()
{
	if (g_bMenuOpen == false)
		return;

	if (g_iNumPages < 0)
		return;

	int RENDERED_WIDTH = 0, RENDERED_HEIGHT = 0;
	int WINDOW_POS_X = 0, WINDOW_POS_Y = 0;
	
	int columnofs = 0;

	SDL_GetWindowSize(window, &RENDERED_WIDTH, &RENDERED_HEIGHT);

	WINDOW_POS_X = (RENDERED_WIDTH - GAME_MODE_WINDOW_WIDTH) / 2;
	WINDOW_POS_Y = (RENDERED_HEIGHT - GAME_MODE_WINDOW_HEIGHT) / 2;

	ImGui::SetNextWindowPos(ImVec2(WINDOW_POS_X, WINDOW_POS_Y), ImGuiSetCond_Once);
	ImGui::SetNextWindowSize(ImVec2(GAME_MODE_WINDOW_WIDTH, GAME_MODE_WINDOW_HEIGHT), ImGuiSetCond_Once);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 10));

	ImGui::Begin("    Chapter Select", &g_bMenuOpen, ChapterSelectGUI_WindowFlags());

	ImGui::Separator();
	ImGui::Spacing();

	ChapterSelectGUI_DrawChapters();
	ImGui::NewLine();
	ChapterSelectGUI_DrawButtons();

	ImGui::End();

	ImGui::PopStyleVar();
}