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
#include "ProceduralLib/SolidMaterials.h"
#include "SculptMaterials.h"


typedef Ptr<CSolidTexture> (*SculptMaterialFunc)(uint32 uiSeed);


struct SSculptMaterial
{
	const char*			szName;
	SculptMaterialFunc	fMake;
};


SSculptMaterial gasSculptMaterials[] =
{
	{ "Marble",	MakeSolidMarble },
	{ "Wood",	MakeSolidWood },
	{ "Stone",	MakeSolidStone },
	{ "Rust",	MakeSolidRust },
	{ "Bricks",	MakeSolidBricks },
	{ "Tiles",	MakeSolidTiles },
};


size NumSculptMaterials(void) { return sizeof(gasSculptMaterials) / sizeof(SSculptMaterial); }
const char* GetSculptMaterialName(size uiMaterial) { return (uiMaterial < NumSculptMaterials()) ? gasSculptMaterials[uiMaterial].szName : NULL; }


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSolidTexture> MakeSculptMaterial(size uiMaterial, uint32 uiSeed)
{
	if (uiMaterial >= NumSculptMaterials())
	{
		return NULL;
	}
	return gasSculptMaterials[uiMaterial].fMake(uiSeed);
}

