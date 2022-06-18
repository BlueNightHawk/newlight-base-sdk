/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// util.cpp
//
// implementation of class-less helper functions
//

#include <cstdio>
#include <cstdlib>

#include "hud.h"
#include "cl_util.h"
#include <string.h>

#include <PlatformHeaders.h>
#include "mathlib.h"

HSPRITE LoadSprite(const char* pszName)
{
	int i;
	char sz[256];

	if (ScreenWidth < 640)
		i = 320;
	else
		i = 640;

	sprintf(sz, pszName, i);

	return SPR_Load(sz);
}

/*
================
Sys_DoubleTime
================
*/
double Sys_DoubleTime(void)
{
	static LARGE_INTEGER g_PerformanceFrequency;
	static LARGE_INTEGER g_ClockStart;
	LARGE_INTEGER CurrentTime;

	if (!g_PerformanceFrequency.QuadPart)
	{
		QueryPerformanceFrequency(&g_PerformanceFrequency);
		QueryPerformanceCounter(&g_ClockStart);
	}
	QueryPerformanceCounter(&CurrentTime);

	return (double)(CurrentTime.QuadPart - g_ClockStart.QuadPart) / (double)(g_PerformanceFrequency.QuadPart);
}

/*
==============
GetFPS
==============
*/
int GetFPS()
{
	float calc;
	double newtime;
	static double nexttime = 0, lasttime = 0;
	static double framerate = 0;
	static int framecount = 0;
	static int minfps = 9999;
	static int maxfps = 0;

	newtime = Sys_DoubleTime();
	if (newtime >= nexttime)
	{
		framerate = framecount / (newtime - lasttime);
		lasttime = newtime;
		nexttime = V_max(nexttime + 1.0, lasttime - 1.0);
		framecount = 0;
	}

	calc = framerate;
	framecount++;

	if (calc >= 1.0f)
	{
		int curfps = (int)(calc + 0.5f);

		if (curfps < minfps)
			minfps = curfps;
		if (curfps > maxfps)
			maxfps = curfps;

		return curfps;
	}

	return 0;
}