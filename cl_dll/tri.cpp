//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "Exports.h"

#include "particleman.h"
#include "tri.h"
#include "PlatformHeaders.h"
#include <gl/gl.h>

#define GL_TEXTURE_RECTANGLE_NV 0x84F5

#include "r_studioint.h"

void DrawCrosshair();

extern engine_studio_api_t IEngineStudio;

bool CanEnableBloom()
{
	return gEngfuncs.CheckParm("-nofbo", nullptr) != 0;
}

// TEXTURES
unsigned int g_uiScreenTex = 0;
unsigned int g_uiGlowTex = 0;

// FUNCTIONS
bool InitScreenGlow(void);
void RenderScreenGlow(void);
void DrawQuad(int width, int height, int ofsX = 0, int ofsY = 0);

bool ScreenGlow_Init()
{
	if (!CanEnableBloom())
		return false;

	// register the CVARs
	gEngfuncs.pfnRegisterVariable("glow_blur_steps", "4", FCVAR_ARCHIVE);
	gEngfuncs.pfnRegisterVariable("glow_darken_steps", "3", FCVAR_ARCHIVE);
	gEngfuncs.pfnRegisterVariable("glow_strength", "0", FCVAR_ARCHIVE);
	return true;
}

bool ScreenGlow_VidInit()
{
	if (!CanEnableBloom())
		return false;

	unsigned char* pBlankTex = new unsigned char[ScreenWidth * ScreenHeight * 3];
	memset(pBlankTex, 0, ScreenWidth * ScreenHeight * 3);

	// Create the SCREEN-HOLDING TEXTURE
	glGenTextures(1, &g_uiScreenTex);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiScreenTex);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB8, ScreenWidth, ScreenHeight, 0, GL_RGB8, GL_UNSIGNED_BYTE, pBlankTex);

	// Create the BLURRED TEXTURE
	glGenTextures(1, &g_uiGlowTex);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiGlowTex);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB8, ScreenWidth / 2, ScreenHeight / 2, 0, GL_RGB8, GL_UNSIGNED_BYTE, pBlankTex);

	// free the memory
	delete[] pBlankTex;

	return true;
}

void DrawQuad(int width, int height, int ofsX, int ofsY)
{
	glTexCoord2f(ofsX, ofsY);
	glVertex3f(0, 1, -1);
	glTexCoord2f(ofsX, height + ofsY);
	glVertex3f(0, 0, -1);
	glTexCoord2f(width + ofsX, height + ofsY);
	glVertex3f(1, 0, -1);
	glTexCoord2f(width + ofsX, ofsY);
	glVertex3f(1, 1, -1);
}

void RenderScreenGlow(void)
{
	if (!CanEnableBloom())
		return;
	// check to see if (a) we can render it, and (b) we're meant to render it

	if (IEngineStudio.IsHardware() != 1)
		return;

	if ((int)gEngfuncs.pfnGetCvarFloat("glow_blur_steps") == 0 || (int)gEngfuncs.pfnGetCvarFloat("glow_strength") == 0)
		return;

	// enable some OpenGL stuff
	glEnable(GL_TEXTURE_RECTANGLE_NV);
	glColor3f(1, 1, 1);
	glDisable(GL_DEPTH_TEST);


	// STEP 1: Grab the screen and put it into a texture

	glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiScreenTex);
	glCopyTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB, 0, 0, ScreenWidth, ScreenHeight, 0);

	// STEP 2: Set up an orthogonal projection

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, 0.1, 100);

	// STEP 3: Render the current scene to a new, lower-res texture, darkening non-bright areas of the scene
	// by multiplying it with itself a few times.

	glViewport(0, 0, ScreenWidth / 2, ScreenHeight / 2);

	glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiScreenTex);

	glBlendFunc(GL_DST_COLOR, GL_ZERO);

	glDisable(GL_BLEND);

	glBegin(GL_QUADS);
	DrawQuad(ScreenWidth, ScreenHeight);
	glEnd();

	glEnable(GL_BLEND);

	glBegin(GL_QUADS);
	for (int i = 0; i < (int)gEngfuncs.pfnGetCvarFloat("glow_darken_steps"); i++)
		DrawQuad(ScreenWidth, ScreenHeight);
	glEnd();

	glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiGlowTex);
	glCopyTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB, 0, 0, ScreenWidth / 2, ScreenHeight / 2, 0);
	float blurAlpha = 1 / (gEngfuncs.pfnGetCvarFloat("glow_blur_steps") * 2 + 1);

	glColor4f(1, 1, 1, blurAlpha);

	glBlendFunc(GL_SRC_ALPHA, GL_ZERO);

	glBegin(GL_QUADS);
	DrawQuad(ScreenWidth / 2, ScreenHeight / 2);
	glEnd();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glBegin(GL_QUADS);
	for (int i = 1; i <= (int)gEngfuncs.pfnGetCvarFloat("glow_blur_steps"); i++)
	{
		DrawQuad(ScreenWidth / 2, ScreenHeight / 2, -i, 0);
		DrawQuad(ScreenWidth / 2, ScreenHeight / 2, i, 0);
	}
	glEnd();

	glCopyTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB, 0, 0, ScreenWidth / 2, ScreenHeight / 2, 0);

	// STEP 5: Blur the horizontally blurred image in the vertical direction.

	glBlendFunc(GL_SRC_ALPHA, GL_ZERO);

	glBegin(GL_QUADS);
	DrawQuad(ScreenWidth / 2, ScreenHeight / 2);
	glEnd();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glBegin(GL_QUADS);
	for (int i = 1; i <= (int)gEngfuncs.pfnGetCvarFloat("glow_blur_steps"); i++)
	{
		DrawQuad(ScreenWidth / 2, ScreenHeight / 2, 0, -i);
		DrawQuad(ScreenWidth / 2, ScreenHeight / 2, 0, i);
	}
	glEnd();

	glCopyTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB, 0, 0, ScreenWidth / 2, ScreenHeight / 2, 0);

	// STEP 6: Combine the blur with the original image.
	glViewport(0, 0, ScreenWidth, ScreenHeight);

	glDisable(GL_BLEND);

	glBegin(GL_QUADS);
	DrawQuad(ScreenWidth / 2, ScreenHeight / 2);
	glEnd();

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	glBegin(GL_QUADS);
	for (int i = 1; i < (int)gEngfuncs.pfnGetCvarFloat("glow_strength"); i++)
	{
		DrawQuad(ScreenWidth / 2, ScreenHeight / 2);
	}
	glEnd();

	glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiScreenTex);

	glBegin(GL_QUADS);
	DrawQuad(ScreenWidth, ScreenHeight);
	glEnd();

	// STEP 7: Restore the original projection and modelview matrices and disable rectangular textures.

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glDisable(GL_TEXTURE_RECTANGLE_NV);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}

extern IParticleMan* g_pParticleMan;

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles()
{
	//	RecClDrawNormalTriangles();

	gHUD.m_Spectator.DrawOverview();
}


/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void DLLEXPORT HUD_DrawTransparentTriangles()
{
	//	RecClDrawTransparentTriangles();

	if (g_pParticleMan)
		g_pParticleMan->Update();
}
/*
=================
HUD_DrawOrthoTriangles
Orthogonal Triangles -- (relative to resolution,
smackdab on the screen) add them here
=================
*/
void HUD_DrawOrthoTriangles()
{
	DrawCrosshair();
}
