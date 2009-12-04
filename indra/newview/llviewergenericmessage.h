/** 
 * @file llviewergenericmessage.h
 * @brief Handle processing of "generic messages" which contain short lists of strings.
 * @author James Cook
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#include "lluuid.h"
#ifndef LLVIEWERGENERICMESSAGE_H
#define LLVIEWERGENERICMESSAGE_H

class LLUUID;
class LLDispatcher;

typedef struct {
	
	F32	red;
	F32	green;
	F32	blue;
	
} Color3;

typedef struct {
	
	F32	X;
	F32	Y;
	F32	Z;
	
} Vector3;

typedef struct {
	
	F32	X;
	F32	Y;
	
} Vector2;

typedef struct {
	
	F32	red;
	F32	green;
	F32	blue;
	F32 alpha;
}  Color4;

typedef struct {

	Color3 waterColor;
	F32 waterFogDensityExponent;
	F32 underwaterFogModifier;
	Vector3 reflectionWaveletScale;
	F32 fresnelScale;
	F32 fresnelOffset;
	F32 refractScaleAbove;
	F32 refractScaleBelow;
	F32 blurMultiplier;
	Vector2 littleWaveDirection;
	Vector2 bigWaveDirection;
	LLUUID normalMapTexture;
	Color4 horizon;
	F32 hazeHorizon;
	Color4 blueDensity;
	F32 hazeDensity;
	F32 densityMultiplier;
	F32 distanceMultiplier;
	Color4 sunMoonColor;
	F32 sunMoonPosiiton;
	Color4 ambient;
	F32 eastAngle;
	F32 sunGlowFocus;
	F32 sunGlowSize;
	F32 sceneGamma;
	F32 starBrightness;
	Color4 cloudColor;
	Vector3 cloudXYDensity;
	F32 cloudCoverage;
	F32 cloudScale;
	Vector3 cloudDetailXYDensity;
	F32 cloudScrollX;
	F32 cloudScrollY;
	unsigned short maxAltitude;
	char cloudScrollXLock;
	char cloudScrollYLock;
	char drawClassicClouds;
	

} Meta7WindlightPacket;

void send_generic_message(const std::string& method,
						  const std::vector<std::string>& strings,
						  const LLUUID& invoice = LLUUID::null);

void process_generic_message(LLMessageSystem* msg, void**);


extern LLDispatcher gGenericDispatcher;

#endif
