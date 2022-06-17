#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "Exports.h"

#include "com_model.h"

typedef struct
{
	short type;
	short texFormat;
	int width;
	int height;
} msprite_t;

model_s* TRI_pModel = nullptr;

void TriHud_Init()
{
}

void TriHUD_VidInit()
{
	TRI_pModel = nullptr;
}

void TRI_GetSpriteParms(model_t* pSprite, int* frameWidth, int* frameHeight)
{
	if (!pSprite || pSprite->type != mod_sprite)
		return;

	msprite_t* pSpr = (msprite_t*)pSprite->cache.data;

	*frameWidth = pSpr->width;
	*frameHeight = pSpr->height;
}
	
void TRI_SprAdjustSize(int* x, int* y, int* w, int* h, bool changepos)
{
	float xscale, yscale;

	if (!x && !y && !w && !h)
		return;

	float yfactor = (float)ScreenWidth / (float)ScreenHeight;

	xscale = ((float)ScreenWidth / 1480.0f);
	yscale = ((float)ScreenHeight / 1480.0f) * yfactor;
	
	if (!changepos)
	{
		xscale *= 1.5f;
		yscale *= 1.5f;
	}

	if (x)
	{
		bool swap_x = 0;
		if (*x > ScreenWidth / 2)
			swap_x = 1;
		if (swap_x)
			*x = (float)ScreenWidth - *x;
		*x *= xscale;
		if (swap_x)
			*x = (float)ScreenWidth - *x;
	}

	if (y)
	{
		bool swap_y = 0;
		if (*y > ScreenHeight / 2)
			swap_y = 1;
		if (swap_y)
			*y = (float)ScreenHeight - *y;
		*y *= yscale;
		if (swap_y)
			*y = (float)ScreenHeight - *y;
	}

	if (w)
		*w *= xscale;
	if (h)
		*h *= yscale;
}

void TRI_SprDrawStretchPic(model_t* pModel, int frame, float x, float y, float w, float h, float s1, float t1, float s2, float t2)
{
	gEngfuncs.pTriAPI->SpriteTexture(pModel, frame);

	gEngfuncs.pTriAPI->Begin(TRI_QUADS);

	gEngfuncs.pTriAPI->TexCoord2f(s1, t1);
	gEngfuncs.pTriAPI->Vertex3f(x, y, 0);

	gEngfuncs.pTriAPI->TexCoord2f(s2, t1);
	gEngfuncs.pTriAPI->Vertex3f(x + w, y, 0);

	gEngfuncs.pTriAPI->TexCoord2f(s2, t2);
	gEngfuncs.pTriAPI->Vertex3f(x + w, y + h, 0);

	gEngfuncs.pTriAPI->TexCoord2f(s1, t2);
	gEngfuncs.pTriAPI->Vertex3f(x, y + h, 0);

	gEngfuncs.pTriAPI->End();
}

void TRI_SprDrawGeneric(int frame, int x, int y, const Rect* prc, bool changepos)
{
	float s1, s2, t1, t2;
	int w, h;

	TRI_GetSpriteParms(TRI_pModel, &w, &h);

	if (prc)
	{
		Rect rc = *prc;

		if (rc.left <= 0 || rc.left >= w)
			rc.left = 0;
		if (rc.top <= 0 || rc.top >= h)
			rc.top = 0;
		if (rc.right <= 0 || rc.right > w)
			rc.right = w;
		if (rc.bottom <= 0 || rc.bottom > h)
			rc.bottom = h;

		s1 = (float)rc.left / w;
		t1 = (float)rc.top / h;
		s2 = (float)rc.right / w;
		t2 = (float)rc.bottom / h;
		w = rc.right - rc.left;
		h = rc.bottom - rc.top;
	}
	else
	{
		s1 = t1 = 0.0f;
		s2 = t2 = 1.0f;
	}

	if (!changepos)
	{
		TRI_SprAdjustSize(&x, &y, &w, &h, changepos);
		x = (ScreenWidth - w) / 2;
		y = (ScreenHeight - h) / 2;
		x -= (gHUD.crossspr.xofs * 10.0f) - (gHUD.crossspr.cxofs * 10.0F);
		y -= (gHUD.crossspr.yofs * 10.0f) + (gHUD.crossspr.cyofs * 10.0F);
	}
	else
		TRI_SprAdjustSize(&x, &y, &w, &h);

	TRI_SprDrawStretchPic(TRI_pModel, frame, x, y, w, h, s1, t1, s2, t2);
}

void TRI_SprDrawAdditive(int frame, int x, int y, const Rect* prc, bool changepos)
{
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);

	TRI_SprDrawGeneric(frame, x, y, prc, changepos);

	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

void TRI_SprSet(HSPRITE spr, int r, int g, int b, int a)
{
	TRI_pModel = (model_s*)gEngfuncs.GetSpritePointer(spr);
	gEngfuncs.pTriAPI->Color4ub(r, g, b, a);
}

void TRI_FillRGBA(int x, int y, int width, int height, int r, int g, int b, int a)
{
	TRI_SprAdjustSize(&x, &y, &width, &height);
	gEngfuncs.pfnFillRGBA(x, y, width, height, r, g, b, a);
}

void SetCrosshair(HSPRITE hspr, Rect rc, int r, int g, int b)
{
	gHUD.crossspr.spr = hspr;
	gHUD.crossspr.rc = rc;
	gHUD.crossspr.r = r;
	gHUD.crossspr.g = g;
	gHUD.crossspr.b = b;
}

void DrawCrosshair()
{
	if (gHUD.crossspr.spr != 0)
	{
		static float oldtime = 0;
		float flTime = gEngfuncs.GetClientTime();

		int y = ScreenHeight;
		int x = ScreenWidth;

		int a = (int)(CVAR_GET_FLOAT("crosshair") * 255);

		if (oldtime != flTime)
		{
			SPR_Set(gHUD.crossspr.spr, 128, 128, 128, a);
			SPR_DrawAdditive(0, x, y, &gHUD.crossspr.rc, false);
		}
		oldtime = flTime;
	}
}


