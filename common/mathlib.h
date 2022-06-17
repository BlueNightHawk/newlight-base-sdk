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
// mathlib.h

#pragma once

#include <cmath>

typedef float vec_t;

#include "vector.h"

typedef vec_t vec4_t[4]; // x,y,z,w
typedef vec_t vec5_t[5];
typedef vec_t matrix3x4[3][4];

typedef short vec_s_t;
typedef vec_s_t vec3s_t[3];
typedef vec_s_t vec4s_t[4]; // x,y,z,w
typedef vec_s_t vec5s_t[5];

typedef int fixed4_t;
typedef int fixed8_t;
typedef int fixed16_t;

#ifndef M_PI
#define M_PI (float)3.14159265358979323846
#endif

#ifndef M_PI2
#define M_PI2 (float)6.28318530717958647692
#endif

#define M_PI_F ((float)(M_PI))
#define M_PI2_F ((float)(M_PI2))

#define RAD2DEG(x) ((float)(x) * (float)(180.f / M_PI))
#define DEG2RAD(x) ((float)(x) * (float)(M_PI / 180.f))

struct mplane_s;

constexpr Vector vec3_origin(0, 0, 0);
constexpr Vector g_vecZero(0, 0, 0);
extern int nanmask;

#define IS_NAN(x) (((*(int*)&x) & nanmask) == nanmask)

#define VectorSubtract(a, b, c)   \
	{                             \
		(c)[0] = (a)[0] - (b)[0]; \
		(c)[1] = (a)[1] - (b)[1]; \
		(c)[2] = (a)[2] - (b)[2]; \
	}
#define VectorAdd(a, b, c)        \
	{                             \
		(c)[0] = (a)[0] + (b)[0]; \
		(c)[1] = (a)[1] + (b)[1]; \
		(c)[2] = (a)[2] + (b)[2]; \
	}
#define VectorCopy(a, b) \
	{                    \
		(b)[0] = (a)[0]; \
		(b)[1] = (a)[1]; \
		(b)[2] = (a)[2]; \
	}
inline void VectorClear(float* a)
{
	a[0] = 0.0;
	a[1] = 0.0;
	a[2] = 0.0;
}

void VectorMA(const float* veca, float scale, const float* vecb, float* vecc);

bool VectorCompare(const float* v1, const float* v2);
float Length(const float* v);
void CrossProduct(const float* v1, const float* v2, float* cross);
float VectorNormalize(float* v); // returns vector length
void VectorInverse(float* v);
void VectorScale(const float* in, float scale, float* out);
int Q_log2(int val);

void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3]);
void R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4]);

void FloorDivMod(double numer, double denom, int* quotient,
	int* rem);
fixed16_t Invert24To16(fixed16_t val);
int GreatestCommonDivisor(int i1, int i2);

void AngleVectors(const Vector& angles, Vector* forward, Vector* right, Vector* up);
void AngleVectorsTranspose(const Vector& angles, Vector* forward, Vector* right, Vector* up);
#define AngleIVectors AngleVectorsTranspose

void AngleMatrix(const float* angles, float (*matrix)[4]);
void AngleIMatrix(const Vector& angles, float (*matrix)[4]);
void VectorTransform(const float* in1, float in2[3][4], float* out);

void NormalizeAngles(float* angles);
void InterpolateAngles(float* start, float* end, float* output, float frac);
float AngleBetweenVectors(const float* v1, const float* v2);


void VectorMatrix(const Vector& forward, Vector& right, Vector& up);
void VectorAngles(const float* forward, float* angles);

int InvertMatrix(const float* m, float* out);

int BoxOnPlaneSide(const Vector& emins, const Vector& emaxs, struct mplane_s* plane);
float anglemod(float a);

/*
=================
SinCos
=================
*/
inline void SinCos(float radians, float* sine, float* cosine)
{
	_asm
	{
		fld	dword ptr [radians]
		fsincos

		mov edx, dword ptr [cosine]
		mov eax, dword ptr [sine]

		fstp dword ptr [edx]
		fstp dword ptr [eax]
	}
}

void Matrix3x4_VectorTransform(const matrix3x4 in, const float v[3], float out[3]);
void Matrix3x4_VectorITransform(const matrix3x4 in, const float v[3], float out[3]);
void Matrix3x4_VectorRotate(const matrix3x4 in, const float v[3], float out[3]);
void Matrix3x4_VectorIRotate(const matrix3x4 in, const float v[3], float out[3]);
void Matrix3x4_ConcatTransforms(matrix3x4 out, const matrix3x4 in1, const matrix3x4 in2);
void Matrix3x4_SetOrigin(matrix3x4 out, float x, float y, float z);
void Matrix3x4_OriginFromMatrix(const matrix3x4 in, float* out);
void Matrix3x4_AnglesFromMatrix(const matrix3x4 in, Vector &out);
void Matrix3x4_FromOriginQuat(matrix3x4 out, const vec4_t quaternion, const Vector origin);
void Matrix3x4_CreateFromEntity(matrix3x4 out, const Vector angles, const Vector origin, float scale);
void Matrix3x4_TransformPositivePlane(const matrix3x4 in, const Vector normal, float d, Vector out, float* dist);
void Matrix3x4_Invert_Simple(matrix3x4 out, const matrix3x4 in1);
void Matrix3x4_Transpose(matrix3x4 out, const matrix3x4 in1);




#define BOX_ON_PLANE_SIDE(emins, emaxs, p)                                                                 \
	(((p)->type < 3) ? (                                                                                   \
						   ((p)->dist <= (emins)[(p)->type]) ? 1                                           \
															 : (                                           \
																   ((p)->dist >= (emaxs)[(p)->type]) ? 2   \
																									 : 3)) \
					 : BoxOnPlaneSide((emins), (emaxs), (p)))
