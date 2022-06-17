//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
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

void DrawCrosshair()
{
	if (gHUD.crossspr.spr != 0)
	{
		static float oldtime = 0;
		float flTime = gEngfuncs.GetClientTime();

		int y = ScreenHeight;
		int x = ScreenWidth;

		if (oldtime != flTime)
		{
			SPR_Set(gHUD.crossspr.spr, 128, 128, 128);
			SPR_DrawAdditive(0, x, y, &gHUD.crossspr.rc, false);
		}
		oldtime = flTime;
	}
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
