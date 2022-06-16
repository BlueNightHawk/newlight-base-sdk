#include "cl_dll.h"
#include "parsemsg.h"
#include "cvardef.h"

#include <iostream>
#include <string> // for string class

#include "PlatformHeaders.h"
#include "hud.h"
#include "subtitles.h"
#include "cpp_aux.h"
#include "fs_aux.h"
#include <map>
#include <fstream>
#include <regex>

extern SDL_Window *window;
cvar_t* subtitles_language;
cvar_t* print_subtitles;
cvar_t* subtitles_font_scale;
cvar_t* subtitles_log_candidates;

using namespace aux;

std::map<std::string, SubtitleOutput> subtitlesToDraw;
std::map<std::string, SubtitleColor> colors;
std::map<std::string, std::vector<Subtitle>> subtitleMap;

void Subtitles_Init() {
	gEngfuncs.pfnHookUserMsg( "OnSound", Subtitles_OnSound );
	gEngfuncs.pfnHookUserMsg( "SubtClear", Subtitles_SubtClear );
	gEngfuncs.pfnHookUserMsg( "SubtRemove", Subtitles_SubtRemove );

	print_subtitles = gEngfuncs.pfnRegisterVariable("subtitles", "2", FCVAR_ARCHIVE);
	subtitles_font_scale = gEngfuncs.pfnRegisterVariable("subtitles_font_scale", "1", FCVAR_ARCHIVE);
	subtitles_language = gEngfuncs.pfnRegisterVariable("subtitles_language", "en", FCVAR_ARCHIVE);
	subtitles_log_candidates = gEngfuncs.pfnRegisterVariable("subtitles_log_candidates", "0", FCVAR_ARCHIVE);

	auto resourceDirectory = FS_ResolveModPath( "resource" );
	auto subtitleFiles = FS_GetAllFileNamesByWildcard( "resource\\subtitles_*.txt" );

	std::regex rgx( "subtitles_(\\w+)\\.txt" );
	for ( auto &sub : subtitleFiles ) {
		std::smatch match;
		std::regex_search( sub, match, rgx );

		if ( match.size() > 1 ) {
			Subtitles_ParseSubtitles(
				FS_ResolveModPath( std::string( "resource\\" ) + sub ),
				match.str( 1 )
			);
		}
	}
}

void Subtitles_ParseSubtitles( const std::string &filePath, const std::string &language ) {

	std::ifstream inp( filePath );
	if ( !inp.is_open( ) ) {
		gEngfuncs.Con_DPrintf( "%s SUBTITLE PARSER ERROR: failed to read file\n", filePath.c_str() );
		return;
	}

	std::string line;
	int lineCount = 0;
	while ( std::getline( inp, line ) ) {
		lineCount++;

		line = str::trim( line );
		if ( line.size() == 0 ) {
			continue;
		}

		if ( line.find( "SUBTITLE" ) != 0 && line.find( "COLOR" ) != 0 ) {
			continue;
		}

		std::vector<std::string> parts = str::split( line, '|' );

		if ( parts.at( 0 ) == "SUBTITLE" ) {
			if ( parts.size() < 6 ) {
				gEngfuncs.Con_DPrintf( "%s SUBTITLE PARSER ERROR ON Line %d: insufficient subtitle data\n", filePath.c_str(), lineCount );
				continue;
			}
			std::string subtitleKey = str::toUppercase( language + "_" + parts.at( 1 ) );
			std::string colorKey = parts.at( 2 );
			std::string text = parts.at( 5 );
			float delay, duration;
			try {
				delay = std::stof( parts.at( 3 ) );
				duration = std::stof( parts.at( 4 ) );

				subtitleMap[subtitleKey].push_back( {
					delay,
					duration,
					colorKey,
					text
				} );
			} catch ( std::invalid_argument e ) {
				gEngfuncs.Con_DPrintf( "%s SUBTITLE PARSER ERROR ON Line %d: delay or duration is not a number\n", filePath.c_str(), lineCount );
			}

		} else if ( parts.at( 0 ) == "COLOR" ) {
			if ( parts.size() < 5 ) {
				gEngfuncs.Con_DPrintf( "%s SUBTITLE PARSER ERROR ON Line %d: insufficient color data\n", filePath.c_str(), lineCount );
				continue;
			}
			std::string colorKey = str::toUppercase( parts.at( 1 ) );
			float r, g, b;
			try {
				r = std::stof( parts.at( 2 ) );
				g = std::stof( parts.at( 3 ) );
				b = std::stof( parts.at( 4 ) );

				colors[colorKey] = { r, g, b };
			} catch ( std::invalid_argument e ) {
				gEngfuncs.Con_DPrintf( "%s SUBTITLE PARSER ERROR ON Line %d: some of color data is not a number\n", filePath.c_str(), lineCount );
			}

		}
	}
}

std::string GetSubtitleKeyWithLanguage( const std::string &key ) {
	std::string language_to_use = std::string( subtitles_language->string );
	auto subtitleKey = str::toUppercase( language_to_use + "_" + key ) ;

	if ( subtitleMap.count( subtitleKey ) ) {
		return subtitleKey;
	} else {
		return "EN_" + key;
	}
}

bool Subtitle_IsFarAwayFromPlayer( const SubtitleOutput &subtitle ) {
	if ( subtitle.ignoreLongDistances ) {
		return false;
	}

	Vector playerOrigin = gEngfuncs.GetLocalPlayer()->origin;
	float distance = ( subtitle.pos - playerOrigin ).Length();

	return distance >= 768;
}

void Subtitles_Draw() {
	int ScreenWidth, ScreenHeight;
	int iNumSubtitles = 0;
	bool bIncreaseAlpha = true;

	float FrameWidth = 0;
	float FrameHeight = 0;
	static float L_Width = 0.0f, L_Height = 0.0f;
	static float l_WindowAlpha = 0.0f;

	std::string lastkey = "null";

	if (subtitlesToDraw.size() == 0)
	{
		L_Width = 0;
		L_Height = 0;
		return;
	}
	SDL_GetWindowSize(window, &ScreenWidth, &ScreenHeight);

	bool willDrawAtLeastOneSubtitle = false;

	float time = gEngfuncs.GetClientTime();
	float fontScale = gEngfuncs.pfnGetCvarFloat( "subtitles_font_scale" );
	if ( fontScale < 1.0f ) {
		fontScale = 1.0f;
	}
	if ( fontScale > 2.0f ) {
		fontScale = 2.0f;
	}

	float tempwidth = 0;
	auto i = subtitlesToDraw.begin();
	while ( i != subtitlesToDraw.end() ) {
		auto &subtitle = i->second;
		
		if ( time >= subtitle.duration ) {
			i = subtitlesToDraw.erase( i );
			continue;
		}

		if ( time < subtitle.delay ) {
			i++;
			continue;
		}
		
		if (Subtitle_IsFarAwayFromPlayer(subtitle))
		{
			i++;
			continue;
		}

		ImVec2 textSize = ImGui::CalcTextSize( subtitle.text.c_str() );
		if (textSize.x > tempwidth)
		{
			tempwidth = textSize.x * fontScale;
		}
		FrameHeight += textSize.y * fontScale;

		if (tempwidth > FrameWidth)
			FrameWidth = tempwidth;

		willDrawAtLeastOneSubtitle = true;
		i++;
		if (stricmp(lastkey.c_str(), subtitle.key.c_str()))
		{
			iNumSubtitles++;
		}

		lastkey = subtitle.key;
	}

	if ( !willDrawAtLeastOneSubtitle ) {
		L_Width = 0;
		L_Height = 0;
		return;
	}

	if (L_Width <= 0.01f || L_Height <= 0.01f)
	{
		L_Width = FrameWidth + 14;
		L_Height = (FrameHeight + 15 + (1.25 * iNumSubtitles));
	}

	L_Width = std::lerp(L_Width, FrameWidth + 14, (float)gHUD.m_flTimeDelta * 20.0f);

	if (iNumSubtitles > 2)
	{
		L_Height = std::lerp(L_Height, (float)(FrameHeight + 20 + (1.85 * iNumSubtitles)), (float)gHUD.m_flTimeDelta * 20.0f);
	}
	else
	{
		L_Height = std::lerp(L_Height, (float)(FrameHeight + 15 + (1.25 * iNumSubtitles)), (float)gHUD.m_flTimeDelta * 20.0f);
	}

	ImGui::SetNextWindowPos(ImVec2(ScreenWidth / 2.0f - L_Width / 2.0f, ScreenHeight / 1.3f), 0);
	ImGui::SetNextWindowSize(ImVec2(L_Width + 0.1, L_Height + 0.1));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, l_WindowAlpha);	
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0, 0.0, 0.0, 0.5f));
	ImGui::Begin( "Subtitles", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
	for ( auto &pair : subtitlesToDraw ) {
		auto &subtitle = pair.second;
		if ( time < subtitle.delay ) {
			continue;
		}

		if (Subtitle_IsFarAwayFromPlayer(subtitle))
			continue;

		if (time > subtitle.fadedelay)
		{
			if (iNumSubtitles == 1)
			{
				if (bIncreaseAlpha)
				{
					l_WindowAlpha = std::lerp(l_WindowAlpha, 0.0f, (float)gHUD.m_flTimeDelta * 15.0f);
					bIncreaseAlpha = false;
				}
			}
			subtitle.alpha = std::lerp(subtitle.alpha, 0.0f, (float)gHUD.m_flTimeDelta * 15.0f);

			if (subtitle.alpha <= 0.012f)
			{
				subtitle.duration = time;
			}
		}
		else
		{
			if (bIncreaseAlpha)
			{
				l_WindowAlpha = std::lerp(l_WindowAlpha, 1.0f, (float)gHUD.m_flTimeDelta * 15.0f);
				bIncreaseAlpha = false;
			}
			subtitle.alpha = std::lerp(subtitle.alpha, 1.0f, (float)gHUD.m_flTimeDelta * 15.0f);
		}

		ImVec2 textSize = ImGui::CalcTextSize( subtitle.text.c_str() );
		ImGui::SetCursorPosX( ImGui::GetWindowWidth() / 2.0f - textSize.x / 2.0f );	
		ImGui::TextColored( ImVec4( subtitle.color[0], subtitle.color[1], subtitle.color[2], subtitle.alpha ), "%s", subtitle.text.c_str() );
	}
	ImGui::SetWindowFontScale( fontScale );
	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
}

void Subtitles_Push( const std::string &key, const std::string &text, float duration, const Vector &color, const Vector &pos, float delay, int ignoreLongDistances ) {
	if ( subtitlesToDraw.count( key ) ) {
		return;
	}
	
	int ScreenWidth;
	SDL_GetWindowSize( window, &ScreenWidth, NULL );

	std::vector<std::string> lines = str::wordWrap( text.c_str(), ScreenWidth / 2.0f, []( const std::string &str ) -> float {
		return ImGui::CalcTextSize( str.c_str() ).x;
	} );

	for ( size_t i = 0 ; i < lines.size() ; i++ ) {
		std::string actualKey = key + "_" + std::to_string( i );

		float startTime = gEngfuncs.GetClientTime() + delay;

		actualKey = GetSubtitleKeyWithLanguage( actualKey );

		subtitlesToDraw[actualKey] = {
			startTime,
			startTime + duration,
			startTime + duration + 0.5f,
			ignoreLongDistances,
			color,
			pos,
			lines.at( i ),
			0.0f,
			key
		};
	}
}

const std::vector<Subtitle> Subtitles_GetByKey( const std::string &key ) {
	std::string actualKey = str::toUppercase( key );

	return subtitleMap.count( actualKey ) ?
		subtitleMap[actualKey] :
		std::vector<Subtitle>();
}

const SubtitleColor Subtitles_GetSubtitleColorByKey( const std::string &key ) {
	SubtitleColor defaultColor = {
		1.0,
		1.0,
		1.0
	};
	return colors.count( key ) ? colors[key] : defaultColor;
}

void Subtitles_Push( const std::string &key, int ignoreLongDistances, const Vector &pos ) {

	float print_subtitles_cvar = gEngfuncs.pfnGetCvarFloat( "subtitles" );
	if ( print_subtitles_cvar <= 0.0f ) {
		return;
	}

	std::string actualKey = GetSubtitleKeyWithLanguage( key );

	auto subtitles = Subtitles_GetByKey( actualKey );

	for ( size_t i = 0 ; i < subtitles.size() ; i++ ) {
		auto &subtitle = subtitles.at( i );
		auto color = Subtitles_GetSubtitleColorByKey( subtitle.colorKey );

		if ( subtitle.colorKey != "PAYNE" && print_subtitles_cvar < 2.0f ) {
			continue;
		}

		Subtitles_Push(
			actualKey + std::to_string( i ),
			subtitle.text,
			subtitle.duration,
			Vector( color.r, color.g, color.b ),
			pos,
			subtitle.delay,
			ignoreLongDistances
		);
	}
}

int Subtitles_OnSound( const char *pszName,  int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );

	std::string key = READ_STRING();
	int ignoreLongDistances = READ_BYTE();
	float x = READ_COORD();
	float y = READ_COORD();
	float z = READ_COORD();

	if ( gEngfuncs.pfnGetCvarFloat( "subtitles_log_candidates" ) >= 1.0f ) {
		gEngfuncs.Con_DPrintf( "RECEIVED SUBTITLE CANDIDATE: %s\n", key.c_str() );
	}

	// Dumb exception for Grunt cutscene, because player is actually far away from sound event
	if ( key.find( "!HG_DRAG" ) == 0 ) {
		ignoreLongDistances = true;
	}

	Subtitles_Push( key, ignoreLongDistances, Vector( x, y, z ) );

	return 1;
}

int Subtitles_SubtClear( const char *pszName,  int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );
	subtitlesToDraw.clear();

	return 1;
}

int Subtitles_SubtRemove( const char *pszName,  int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );
	std::string key = READ_STRING();

	auto i = subtitlesToDraw.begin();
	while ( i != subtitlesToDraw.end() ) {
		auto subtitleKey = GetSubtitleKeyWithLanguage( key );

		if ( subtitleKey.find( key ) == 0 ) {
			i = subtitlesToDraw.erase( i );
			continue;
		}

		i++;
	}

	return 1;
}