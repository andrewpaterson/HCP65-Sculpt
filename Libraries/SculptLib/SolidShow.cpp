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
#include "StandardLib/Objects.h"
#include "StandardLib/PointerField.h"
#include "StandardLib/UnmanagedField.h"
#include "SupportLib/SolidChecker.h"
#include "ProceduralLib/SolidAdjust.h"
#include "ProceduralLib/SolidBricks.h"
#include "ProceduralLib/SolidCells.h"
#include "ProceduralLib/SolidGradient.h"
#include "ProceduralLib/SolidMask.h"
#include "ProceduralLib/SolidMerge.h"
#include "ProceduralLib/SolidNoise.h"
#include "ProceduralLib/SolidRamp.h"
#include "ProceduralLib/SolidSlope.h"
#include "ProceduralLib/SolidWarp.h"
#include "SolidShow.h"


//////////////////////////////////////////////////////////////////////////
//
// A new texture of each class, as a node added in the editor starts:
// nothing linked to it, and settings that show something.
//
//////////////////////////////////////////////////////////////////////////
static Ptr<CSolidTexture> SolidMakeNoise(void) { return OMalloc<CSolidNoise>(FN_Sum, 4, 1); }
static Ptr<CSolidTexture> SolidMakeCells(void) { return OMalloc<CSolidCells>(SC_Nearest, 1); }
static Ptr<CSolidTexture> SolidMakeGradient(void) { return OMalloc<CSolidGradient>(SG_Plane, SR_Once); }
static Ptr<CSolidTexture> SolidMakeChecker(void) { return OMalloc<CSolidChecker>(0.25f, Set32BitColour((uint8)230, (uint8)230, (uint8)230), Set32BitColour((uint8)40, (uint8)40, (uint8)40)); }
static Ptr<CSolidTexture> SolidMakeSlope(void) { return OMalloc<CSolidSlope>(); }
static Ptr<CSolidTexture> SolidMakeAdjust(void) { return OMalloc<CSolidAdjust>(Ptr<CSolidTexture>()); }
static Ptr<CSolidTexture> SolidMakeMerge(void) { return OMalloc<CSolidMerge>(SM_Multiply, Ptr<CSolidTexture>(), Ptr<CSolidTexture>()); }
static Ptr<CSolidTexture> SolidMakeMask(void) { return OMalloc<CSolidMask>(Ptr<CSolidTexture>(), Ptr<CSolidTexture>(), Ptr<CSolidTexture>()); }
static Ptr<CSolidTexture> SolidMakeWarp(void) { return OMalloc<CSolidWarp>(Ptr<CSolidTexture>(), Ptr<CSolidTexture>(), 0.5f); }


static Ptr<CSolidTexture> SolidMakeBricks(void)
{
	SFloat32Vec3	sSize;

	sSize.Init(0.3f, 0.15f, 0.1f);
	return OMalloc<CSolidBricks>(SB_Brick, &sSize, 0.1f, 0.5f, 1);
}


//Black at 0 to white at 1.
static Ptr<CSolidTexture> SolidMakeRamp(void)
{
	Ptr<CSolidRamp>	pRamp;

	pRamp = OMalloc<CSolidRamp>(Ptr<CSolidTexture>());
	pRamp->AddKey(0.0f, Set32BitColour((uint8)0, (uint8)0, (uint8)0));
	pRamp->AddKey(1.0f, Set32BitColour((uint8)255, (uint8)255, (uint8)255));
	return pRamp;
}


//A cavity is of a shape, which a node has no way to give it: it's shown, not added.
SSolidNodeShow gasSolidNodeShow[] =
{
	{ "CSolidNoise",	"Noise Texture",		SNK_Texture,	SolidMakeNoise },
	{ "CSolidCells",	"Voronoi Texture",		SNK_Texture,	SolidMakeCells },
	{ "CSolidGradient",	"Gradient Texture",		SNK_Texture,	SolidMakeGradient },
	{ "CSolidBricks",	"Brick Texture",		SNK_Texture,	SolidMakeBricks },
	{ "CSolidChecker",	"Checker Texture",		SNK_Texture,	SolidMakeChecker },
	{ "CSolidSlope",	"Slope",				SNK_Texture,	SolidMakeSlope },
	{ "CSolidCavity",	"Cavity",				SNK_Texture,	NULL },
	{ "CSolidRamp",		"Color Ramp",			SNK_Converter,	SolidMakeRamp },
	{ "CSolidAdjust",	"Hue/Saturation/Value",	SNK_Colour,		SolidMakeAdjust },
	{ "CSolidMerge",	"Mix",					SNK_Colour,		SolidMakeMerge },
	{ "CSolidMask",		"Mix by Mask",			SNK_Colour,		SolidMakeMask },
	{ "CSolidWarp",		"Warp",					SNK_Vector,		SolidMakeWarp },
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
	{ "CSolidRamp",		"mpInput",		"Fac",			SFS_Range,	0, 1, NULL },
	{ "CSolidMask",		"mpMask",		"Fac",			SFS_Range,	0, 1, NULL },
	{ "CSolidWarp",		"mpWarp",		"By",			SFS_Range,	0, 1, NULL },
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


size NumSolidNodeShows(void) { return sizeof(gasSolidNodeShow) / sizeof(SSolidNodeShow); }
SSolidNodeShow* GetSolidNodeShow(size uiIndex) { return &gasSolidNodeShow[uiIndex]; }
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


//////////////////////////////////////////////////////////////////////////
//
// A field whose bytes are all it is, so that copying them copies it: a
// number, a tick, an enum, a vector.  A structure, an array of keys or a
// string may own memory.
//
//////////////////////////////////////////////////////////////////////////
static bool SolidFieldIsPlain(CUnmanagedField* pcField)
{
	switch (pcField->GetType())
	{
	case PT_float32:
	case PT_float64:
	case PT_int8:
	case PT_int16:
	case PT_int32:
	case PT_int64:
	case PT_uint8:
	case PT_uint16:
	case PT_uint32:
	case PT_uint64:
	case PT_bool:
	case PT_Enum:
	case PT_M3float32:
		return true;
	default:
		return false;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Another texture as pcTexture is: its class's new texture given
// pcTexture's plain fields, its inputs (which the two then share) and, of
// a ramp, its keys.  NULL for a class that isn't made new (SSolidNodeShow).
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSolidTexture> SolidCopy(CSolidTexture* pcTexture)
{
	Ptr<CSolidTexture>	pCopy;
	SSolidNodeShow*		psShow;
	CArrayVoidPtr*		papcFields;
	CUnmanagedField*	pcField;
	CPointerField*		pcPointer;
	CSolidRamp*			pcRamp;
	size				i;

	psShow = FindSolidNodeShow(pcTexture);
	if ((psShow == NULL) || (psShow->fMake == NULL))
	{
		return NULL;
	}
	pCopy = psShow->fMake();

	papcFields = pcTexture->GetClass()->GetUnmanagedFields();
	for (i = 0; i < papcFields->NumElements(); i++)
	{
		pcField = (CUnmanagedField*)papcFields->GetPtr(i);
		if (SolidFieldIsPlain(pcField))
		{
			memcpy(pcField->GetData(&pCopy), pcField->GetData(pcTexture), pcField->GetSizeOf() * pcField->GetLength());
		}
	}
	papcFields = pcTexture->GetClass()->GetPointerFields();
	for (i = 0; i < papcFields->NumElements(); i++)
	{
		pcPointer = (CPointerField*)papcFields->GetPtr(i);
		*pcPointer->GetPointer(&pCopy) = *pcPointer->GetPointer(pcTexture);
	}

	pcRamp = dynamic_cast<CSolidRamp*>(pcTexture);
	if (pcRamp != NULL)
	{
		Ptr<CSolidRamp>(pCopy)->masKeys.ReInit();
		Ptr<CSolidRamp>(pCopy)->masKeys.Copy(&pcRamp->masKeys);
	}
	return pCopy;
}

