/*
matrixlib.c - internal matrixlib
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "in_defs.h"
#include "mathlib.h"

const matrix3x4 matrix3x4_identity =
{
{ 1, 0, 0, 0 },	// PITCH	[forward], org[0]
{ 0, 1, 0, 0 },	// YAW	[right]  , org[1]
{ 0, 0, 1, 0 },	// ROLL	[up]     , org[2]
};

/*
========================================================================

		Matrix3x4 operations

========================================================================
*/
void Matrix3x4_VectorTransform( const matrix3x4 in, const float v[3], float out[3] )
{
	out[0] = v[0] * in[0][0] + v[1] * in[0][1] + v[2] * in[0][2] + in[0][3];
	out[1] = v[0] * in[1][0] + v[1] * in[1][1] + v[2] * in[1][2] + in[1][3];
	out[2] = v[0] * in[2][0] + v[1] * in[2][1] + v[2] * in[2][2] + in[2][3];
}

void Matrix3x4_VectorITransform( const matrix3x4 in, const float v[3], float out[3] )
{
	Vector	dir;

	dir[0] = v[0] - in[0][3];
	dir[1] = v[1] - in[1][3];
	dir[2] = v[2] - in[2][3];

	out[0] = dir[0] * in[0][0] + dir[1] * in[1][0] + dir[2] * in[2][0];
	out[1] = dir[0] * in[0][1] + dir[1] * in[1][1] + dir[2] * in[2][1];
	out[2] = dir[0] * in[0][2] + dir[1] * in[1][2] + dir[2] * in[2][2];
}

void Matrix3x4_VectorRotate( const matrix3x4 in, const float v[3], float out[3] )
{
	out[0] = v[0] * in[0][0] + v[1] * in[0][1] + v[2] * in[0][2];
	out[1] = v[0] * in[1][0] + v[1] * in[1][1] + v[2] * in[1][2];
	out[2] = v[0] * in[2][0] + v[1] * in[2][1] + v[2] * in[2][2];
}

void Matrix3x4_VectorIRotate( const matrix3x4 in, const float v[3], float out[3] )
{
	out[0] = v[0] * in[0][0] + v[1] * in[1][0] + v[2] * in[2][0];
	out[1] = v[0] * in[0][1] + v[1] * in[1][1] + v[2] * in[2][1];
	out[2] = v[0] * in[0][2] + v[1] * in[1][2] + v[2] * in[2][2];
}

void Matrix3x4_ConcatTransforms( matrix3x4 out, const matrix3x4 in1, const matrix3x4 in2 )
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] + in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] + in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] + in1[2][2] * in2[2][3] + in1[2][3];
}

void Matrix3x4_SetOrigin( matrix3x4 out, float x, float y, float z )
{
	out[0][3] = x;
	out[1][3] = y;
	out[2][3] = z;
}

void Matrix3x4_OriginFromMatrix( const matrix3x4 in, float *out )
{
	out[0] = in[0][3];
	out[1] = in[1][3];
	out[2] = in[2][3];
}

void Matrix3x4_AnglesFromMatrix( const matrix3x4 in, Vector &out )
{
	float xyDist = sqrt( in[0][0] * in[0][0] + in[1][0] * in[1][0] );

	if( xyDist > 0.001f )
	{
		// enough here to get angles?
		out[0] = RAD2DEG( atan2( -in[2][0], xyDist ));
		out[1] = RAD2DEG( atan2( in[1][0], in[0][0] ));
		out[2] = RAD2DEG( atan2( in[2][1], in[2][2] ));
	}
	else
	{
		// forward is mostly Z, gimbal lock
		out[0] = RAD2DEG( atan2( -in[2][0], xyDist ));
		out[1] = RAD2DEG( atan2( -in[0][1], in[1][1] ));
		out[2] = 0.0f;
	}
}

void Matrix3x4_FromOriginQuat( matrix3x4 out, const vec4_t quaternion, const Vector origin )
{
	out[0][0] = 1.0f - 2.0f * quaternion[1] * quaternion[1] - 2.0f * quaternion[2] * quaternion[2];
	out[1][0] = 2.0f * quaternion[0] * quaternion[1] + 2.0f * quaternion[3] * quaternion[2];
	out[2][0] = 2.0f * quaternion[0] * quaternion[2] - 2.0f * quaternion[3] * quaternion[1];

	out[0][1] = 2.0f * quaternion[0] * quaternion[1] - 2.0f * quaternion[3] * quaternion[2];
	out[1][1] = 1.0f - 2.0f * quaternion[0] * quaternion[0] - 2.0f * quaternion[2] * quaternion[2];
	out[2][1] = 2.0f * quaternion[1] * quaternion[2] + 2.0f * quaternion[3] * quaternion[0];

	out[0][2] = 2.0f * quaternion[0] * quaternion[2] + 2.0f * quaternion[3] * quaternion[1];
	out[1][2] = 2.0f * quaternion[1] * quaternion[2] - 2.0f * quaternion[3] * quaternion[0];
	out[2][2] = 1.0f - 2.0f * quaternion[0] * quaternion[0] - 2.0f * quaternion[1] * quaternion[1];

	out[0][3] = origin[0];
	out[1][3] = origin[1];
	out[2][3] = origin[2];
}

void Matrix3x4_CreateFromEntity( matrix3x4 out, const Vector angles, const Vector origin, float scale )
{
	float	angle, sr, sp, sy, cr, cp, cy;

	if( angles[ROLL] )
	{
		angle = angles[YAW] * (M_PI2 / 360.0f);
		SinCos( angle, &sy, &cy );
		angle = angles[PITCH] * (M_PI2 / 360.0f);
		SinCos( angle, &sp, &cp );
		angle = angles[ROLL] * (M_PI2 / 360.0f);
		SinCos( angle, &sr, &cr );

		out[0][0] = (cp*cy) * scale;
		out[0][1] = (sr*sp*cy+cr*-sy) * scale;
		out[0][2] = (cr*sp*cy+-sr*-sy) * scale;
		out[0][3] = origin[0];
		out[1][0] = (cp*sy) * scale;
		out[1][1] = (sr*sp*sy+cr*cy) * scale;
		out[1][2] = (cr*sp*sy+-sr*cy) * scale;
		out[1][3] = origin[1];
		out[2][0] = (-sp) * scale;
		out[2][1] = (sr*cp) * scale;
		out[2][2] = (cr*cp) * scale;
		out[2][3] = origin[2];
	}
	else if( angles[PITCH] )
	{
		angle = angles[YAW] * (M_PI2 / 360.0f);
		SinCos( angle, &sy, &cy );
		angle = angles[PITCH] * (M_PI2 / 360.0f);
		SinCos( angle, &sp, &cp );

		out[0][0] = (cp*cy) * scale;
		out[0][1] = (-sy) * scale;
		out[0][2] = (sp*cy) * scale;
		out[0][3] = origin[0];
		out[1][0] = (cp*sy) * scale;
		out[1][1] = (cy) * scale;
		out[1][2] = (sp*sy) * scale;
		out[1][3] = origin[1];
		out[2][0] = (-sp) * scale;
		out[2][1] = 0.0f;
		out[2][2] = (cp) * scale;
		out[2][3] = origin[2];
	}
	else if( angles[YAW] )
	{
		angle = angles[YAW] * (M_PI2 / 360.0f);
		SinCos( angle, &sy, &cy );

		out[0][0] = (cy) * scale;
		out[0][1] = (-sy) * scale;
		out[0][2] = 0.0f;
		out[0][3] = origin[0];
		out[1][0] = (sy) * scale;
		out[1][1] = (cy) * scale;
		out[1][2] = 0.0f;
		out[1][3] = origin[1];
		out[2][0] = 0.0f;
		out[2][1] = 0.0f;
		out[2][2] = scale;
		out[2][3] = origin[2];
	}
	else
	{
		out[0][0] = scale;
		out[0][1] = 0.0f;
		out[0][2] = 0.0f;
		out[0][3] = origin[0];
		out[1][0] = 0.0f;
		out[1][1] = scale;
		out[1][2] = 0.0f;
		out[1][3] = origin[1];
		out[2][0] = 0.0f;
		out[2][1] = 0.0f;
		out[2][2] = scale;
		out[2][3] = origin[2];
	}
}

void Matrix3x4_TransformPositivePlane( const matrix3x4 in, const Vector normal, float d, Vector out, float *dist )
{
	float	scale = sqrt( in[0][0] * in[0][0] + in[0][1] * in[0][1] + in[0][2] * in[0][2] );
	float	iscale = 1.0f / scale;

	out[0] = (normal[0] * in[0][0] + normal[1] * in[0][1] + normal[2] * in[0][2]) * iscale;
	out[1] = (normal[0] * in[1][0] + normal[1] * in[1][1] + normal[2] * in[1][2]) * iscale;
	out[2] = (normal[0] * in[2][0] + normal[1] * in[2][1] + normal[2] * in[2][2]) * iscale;
	*dist = d * scale + ( out[0] * in[0][3] + out[1] * in[1][3] + out[2] * in[2][3] );
}

void Matrix3x4_Invert_Simple( matrix3x4 out, const matrix3x4 in1 )
{
	// we only support uniform scaling, so assume the first row is enough
	// (note the lack of sqrt here, because we're trying to undo the scaling,
	// this means multiplying by the inverse scale twice - squaring it, which
	// makes the sqrt a waste of time)
	float	scale = 1.0f / (in1[0][0] * in1[0][0] + in1[0][1] * in1[0][1] + in1[0][2] * in1[0][2]);

	// invert the rotation by transposing and multiplying by the squared
	// recipricol of the input matrix scale as described above
	out[0][0] = in1[0][0] * scale;
	out[0][1] = in1[1][0] * scale;
	out[0][2] = in1[2][0] * scale;
	out[1][0] = in1[0][1] * scale;
	out[1][1] = in1[1][1] * scale;
	out[1][2] = in1[2][1] * scale;
	out[2][0] = in1[0][2] * scale;
	out[2][1] = in1[1][2] * scale;
	out[2][2] = in1[2][2] * scale;

	// invert the translate
	out[0][3] = -(in1[0][3] * out[0][0] + in1[1][3] * out[0][1] + in1[2][3] * out[0][2]);
	out[1][3] = -(in1[0][3] * out[1][0] + in1[1][3] * out[1][1] + in1[2][3] * out[1][2]);
	out[2][3] = -(in1[0][3] * out[2][0] + in1[1][3] * out[2][1] + in1[2][3] * out[2][2]);
}

void Matrix3x4_Transpose( matrix3x4 out, const matrix3x4 in1 )
{
	// transpose only rotational component
	out[0][0] = in1[0][0];
	out[0][1] = in1[1][0];
	out[0][2] = in1[2][0];
	out[1][0] = in1[0][1];
	out[1][1] = in1[1][1];
	out[1][2] = in1[2][1];
	out[2][0] = in1[0][2];
	out[2][1] = in1[1][2];
	out[2][2] = in1[2][2];

	// copy origin
	out[0][3] = in1[0][3];
	out[1][3] = in1[1][3];
	out[2][3] = in1[2][3];
}
