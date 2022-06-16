#ifndef SUBTITLES_H
#define SUBTITLES_H

#include "SDL2\SDL.h"
#include "SDL2\SDL_opengl.h"
#include "..\imgui\imgui.h"
#include "..\imgui\imgui_impl_sdl.h"
#include <vector>

struct SubtitleOutput {
	float delay;
	float fadedelay;
	float duration;
	int ignoreLongDistances;
	Vector color;
	Vector pos;
	std::string text;
	float alpha;
	std::string key;
};

struct SubtitleColor {
	float r;
	float g;
	float b;
};

struct Subtitle {
	float delay;
	float duration;
	std::string colorKey;
	std::string text;
};

void Subtitles_Init();
void Subtitles_ParseSubtitles( const std::string &filePath, const std::string &language );
void Subtitles_Draw();
bool Subtitle_IsFarAwayFromPlayer( const SubtitleOutput &subtitle );
void Subtitles_Push( const std::string &key, int ignoreLongDistances, const Vector &pos );
void Subtitles_Push( const std::string &key, const std::string &text, float duration, const Vector &color, const Vector &pos, float delay = 0.0f, int ignoreLongDistances = 0 );
int Subtitles_OnSound( const char *pszName, int iSize, void *pbuf );
int Subtitles_SubtClear( const char *pszName, int iSize, void *pbuf );
int Subtitles_SubtRemove( const char *pszName, int iSize, void *pbuf );
const std::vector<Subtitle> Subtitles_GetByKey( const std::string &key );
const SubtitleColor Subtitles_GetSubtitleColorByKey( const std::string &key );

#endif // SUBTITLES_H