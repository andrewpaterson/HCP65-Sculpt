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
#include "StandardLib/ClassDefines.h"
#include "StandardLib/Objects.h"
#include "ProceduralLib/SolidMaterialFile.h"
#include "SculptDocument.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptDocument::Init(void)
{
	PreInit();

	miSpheres = 0;
	miShape = 0;
	miShow = 0;
	miSamples = 1;
	mfRelief = 0.0f;
	msLight.Init(0, 0, 1);

	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptDocument::Class(void)
{
	M_Pointer(mpGraph);
	M_Pointer(mpSculpted);
	U_Int32(miSpheres);
	U_Int32(miShape);
	U_Int32(miShow);
	U_Int32(miSamples);
	U_Float32(mfRelief);
	U_3Float32(msLight);
}


void CSculptDocument::Free(void) {}


//////////////////////////////////////////////////////////////////////////
//
// The solid textures and shapes a material may hold, and Sculpt's own.
//
//////////////////////////////////////////////////////////////////////////
void AddSculptConstructors(void)
{
	AddSolidTextureConstructors();
	gcObjects.AddConstructor<CSculptDocument>();
	gcObjects.AddConstructor<CSculptGraph>();
	gcObjects.AddConstructor<CSculptNode>();
}

