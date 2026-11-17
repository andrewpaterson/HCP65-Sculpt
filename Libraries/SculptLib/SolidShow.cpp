/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela SculptLib

Codaphela SculptLib is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela SculptLib is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Codaphela SculptLib.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#include <string.h>
#include "BaseLib/StringHelper.h"
#include "StandardLib/Class.h"
#include "SolidShow.h"


SSolidNodeShow gasSolidNodeShow[] =
{
	{ "CSolidNoise",	"Noise Texture",		SNK_Texture },
	{ "CSolidCells",	"Voronoi Texture",		SNK_Texture },
	{ "CSolidGradient",	"Gradient Texture",		SNK_Texture },
	{ "CSolidBricks",	"Brick Texture",		SNK_Texture },
	{ "CSolidChecker",	"Checker Texture",		SNK_Texture },
	{ "CSolidSlope",	"Slope",				SNK_Texture },
	{ "CSolidCavity",	"Cavity",				SNK_Texture },
	{ "CSolidRamp",		"Color Ramp",			SNK_Converter },
	{ "CSolidAdjust",	"Hue/Saturation/Value",	SNK_Colour },
	{ "CSolidMerge",	"Mix",					SNK_Colour },
	{ "CSolidMask",		"Mix by Mask",			SNK_Colour },
	{ "CSolidWarp",		"Warp",					SNK_Vector },
};


//Blender's header colours, near enough.
ARGB32 gauiSolidNodeKindColours[SNK_NumKinds] =
{
	Set32BitColour((uint8)121, (uint8)70, (uint8)29),
	Set32BitColour((uint8)110, (uint8)110, (uint8)29),
	Set32BitColour((uint8)36, (uint8)98, (uint8)131),
	Set32BitColour((uint8)60, (uint8)60, (uint8)131),
	Set32BitColour((uint8)123, (uint8)47, (uint8)58),
};


const char* gaszSolidNodeKindNames[SNK_NumKinds] = { "Texture", "Color", "Converter", "Vector", "Output" };


//A field is found by its class and its name: several classes have a meKind, each its own.
SSolidFieldShow gasSolidFieldShow[] =
{
	{ "CSolidTexture",	"msOrigin",		NULL,			SFS_Hidden,	0, 0, NULL },
	{ "CSolidTexture",	"masAxes",		NULL,			SFS_Hidden,	0, 0, NULL },
	{ "CSolidCavity",	"mpShape",		NULL,			SFS_Hidden,	0, 0, NULL },
	{ "CSolidRamp",		"masKeys",		NULL,			SFS_Hidden,	0, 0, NULL },
	{ "CSolidFlat",		"msSample",		NULL,			SFS_Hidden,	0, 0, NULL },
	{ "CSolidRamp",		"mpInput",		"Fac",			SFS_Plain,	0, 0, NULL },
	{ "CSolidMask",		"mpMask",		"Fac",			SFS_Plain,	0, 0, NULL },
	{ "CSolidWarp",		"mpWarp",		"By",			SFS_Plain,	0, 0, NULL },
	{ "CSolidNoise",	"meKind",		"Kind",			SFS_Enum,	0, 0, "Sum|Turbulence|Ridged" },
	{ "CSolidGradient",	"meFrom",		"From",			SFS_Enum,	0, 0, "Plane|Axis|Point" },
	{ "CSolidGradient",	"meRepeat",		"Repeat",		SFS_Enum,	0, 0, "Once|Saw|Wave" },
	{ "CSolidMerge",	"meMerge",		"Blend",		SFS_Enum,	0, 0, "Add|Multiply|Lesser|Greater|Difference" },
	{ "CSolidCells",	"meKind",		"Kind",			SFS_Enum,	0, 0, "Nearest|Edge|Cell" },
	{ "CSolidBricks",	"meKind",		"Kind",			SFS_Enum,	0, 0, "Brick|Inside" },
	{ "CSolidNoise",	"muiOctaves",	"Detail",		SFS_Range,	1, 12, NULL },
	{ "CSolidBricks",	"mfMortar",		"Mortar",		SFS_Range,	0, 1, NULL },
	{ "CSolidBricks",	"mfOffset",		"Offset",		SFS_Range,	0, 1, NULL },
	{ "CSolidAdjust",	"mfHue",		"Hue",			SFS_Range,	-0.5, 0.5, NULL },
	{ "CSolidAdjust",	"mfSaturation",	"Saturation",	SFS_Range,	0, 2, NULL },
	{ "CSolidAdjust",	"mfContrast",	"Contrast",		SFS_Range,	0, 2, NULL },
	{ "CSolidAdjust",	"mfBrightness",	"Brightness",	SFS_Range,	-1, 1, NULL },
};


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
static SSolidNodeShow* FindSolidNodeShow(CSolidTexture* pcTexture)
{
	const char*		szClass;
	size			i;

	szClass = pcTexture->GetClass()->GetName();
	for (i = 0; i < sizeof(gasSolidNodeShow) / sizeof(SSolidNodeShow); i++)
	{
		if (strcmp(gasSolidNodeShow[i].szClass, szClass) == 0)
		{
			return &gasSolidNodeShow[i];
		}
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// What a texture's node is called.
//
//////////////////////////////////////////////////////////////////////////
void SolidNodeTitle(CChars* pszDest, CSolidTexture* pcTexture)
{
	SSolidNodeShow*		psShow;
	const char*			szClass;

	psShow = FindSolidNodeShow(pcTexture);
	if (psShow != NULL)
	{
		pszDest->Append(psShow->szTitle);
		return;
	}
	szClass = pcTexture->GetClass()->GetName();
	pszDest->Append((strncmp(szClass, "CSolid", 6) == 0) ? szClass + 6 : szClass);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
ESolidNodeKind SolidNodeKind(CSolidTexture* pcTexture)
{
	SSolidNodeShow*		psShow;

	psShow = FindSolidNodeShow(pcTexture);
	return (psShow != NULL) ? psShow->eKind : SNK_Texture;
}


const char* SolidNodeKindName(ESolidNodeKind eKind) { return gaszSolidNodeKindNames[eKind]; }
ARGB32 SolidNodeKindColour(ESolidNodeKind eKind) { return gauiSolidNodeKindColours[eKind]; }


//////////////////////////////////////////////////////////////////////////
//
// NULL for a field with nothing said of it.
//
//////////////////////////////////////////////////////////////////////////
SSolidFieldShow* FindSolidFieldShow(CSolidTexture* pcTexture, const char* szField)
{
	SSolidFieldShow*	psShow;
	const char*			szClass;
	size				i;

	szClass = pcTexture->GetClass()->GetName();
	for (i = 0; i < sizeof(gasSolidFieldShow) / sizeof(SSolidFieldShow); i++)
	{
		psShow = &gasSolidFieldShow[i];
		if ((strcmp(psShow->szField, szField) == 0) && ((strcmp(psShow->szClass, szClass) == 0) || (strcmp(psShow->szClass, "CSolidTexture") == 0)))
		{
			return psShow;
		}
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// The field's label: what's said of it, or its name from its first
// capital ("mfScale" is "Scale", "muiSeed" is "Seed").
//
//////////////////////////////////////////////////////////////////////////
void SolidFieldLabel(CChars* pszDest, CSolidTexture* pcTexture, const char* szField)
{
	SSolidFieldShow*	psShow;
	const char*			szName;

	psShow = FindSolidFieldShow(pcTexture, szField);
	if ((psShow != NULL) && (psShow->szLabel != NULL))
	{
		pszDest->Append(psShow->szLabel);
		return;
	}
	szName = szField;
	while ((*szName != '\0') && !((*szName >= 'A') && (*szName <= 'Z')))
	{
		szName++;
	}
	pszDest->Append((*szName != '\0') ? szName : szField);
}

