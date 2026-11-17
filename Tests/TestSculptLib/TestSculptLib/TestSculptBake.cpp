/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela TestSculptLib

Codaphela TestSculptLib is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela TestSculptLib is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Codaphela TestSculptLib.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#include <stdio.h>
#include <stdlib.h>
#include "BaseLib/Timer.h"
#include "StandardLib/Objects.h"
#include "SupportLib/MeshBake.h"
#include "SupportLib/MeshEditor.h"
#include "SupportLib/MeshUnwrap.h"
#include "SupportLib/Sphere.h"
#include "ProceduralLib/MeshShapeEditor.h"
#include "ProceduralLib/MeshShapeNormals.h"
#include "ProceduralLib/SolidMaterials.h"
#include "SculptLib/SculptPage.h"
#include "TestLib/Assert.h"


#define TEST_SCULPT_BAKE_MATERIALS	6


//////////////////////////////////////////////////////////////////////////
//
// The uiIndex'th material and its name.
//
//////////////////////////////////////////////////////////////////////////
static Ptr<CSolidTexture> TestSculptBakeMaterial(size uiIndex, const char** pszName)
{
	static const char*	aszNames[TEST_SCULPT_BAKE_MATERIALS] = { "Marble", "Wood", "Stone", "Rust", "Bricks", "Tiles" };

	*pszName = aszNames[uiIndex];
	switch (uiIndex)
	{
	case 0:		return MakeSolidMarble(1);
	case 1:		return MakeSolidWood(1);
	case 2:		return MakeSolidStone(1);
	case 3:		return MakeSolidRust(1);
	case 4:		return MakeSolidBricks(1);
	default:	return MakeSolidTiles(1);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The milliseconds a sphere a unit in radius takes to be given pTexture
// at fTexelsPerUnit: meshed, unwrapped and baked first with a texture
// (the whole), then baked again with pTexture, the mesh and its UVs as
// they were (the bake alone, which is what a setting changed costs).
//
//////////////////////////////////////////////////////////////////////////
static void TestSculptBakeTimes(Ptr<CSolidTexture> pTexture, float32 fTexelsPerUnit, int32 iSamples, int64* piWhole, int64* piBake)
{
	Ptr<CMeshEditor>		pEditor;
	Ptr<CMeshShapeEditor>	pShape;
	Ptr<CMeshShapeNormals>	pNormals;
	Ptr<CMeshUnwrap>		pUnwrap;
	Ptr<CMeshBake>			pBake;
	Ptr<CMesh>				pUnwrapped;
	Ptr<CSphere>			pSphere;
	SFloat32Vec3			sCentre;
	int64					iStart;

	sCentre.Init(0, 0, 0);
	pSphere = OMalloc<CSphere>(&sCentre, SCULPT_SPHERE_RADIUS);
	pEditor = OMalloc<CMeshEditor>(OMalloc<CMesh>());
	pShape = pEditor->AddModifier<CMeshShapeEditor>(pSphere, SCULPT_SMALLEST_CELL, SCULPT_LARGEST_CELL);
	pNormals = pEditor->AddModifier<CMeshShapeNormals>(pSphere);
	pUnwrap = pEditor->AddModifier<CMeshUnwrap>(fTexelsPerUnit);
	pBake = pEditor->AddModifier<CMeshBake>(pTexture);
	pBake->miSamples = iSamples;

	iStart = GetDebugTickCount();
	pEditor->GetOutput();
	*piWhole = GetDebugTickCount() - iStart;

	pUnwrapped = pUnwrap->Modify(pNormals->Modify(pShape->Modify(pEditor->mpMesh)));
	iStart = GetDebugTickCount();
	pBake->Modify(pUnwrapped);
	*piBake = GetDebugTickCount() - iStart;
}


//////////////////////////////////////////////////////////////////////////
//
// SCULPT_BAKE_REPORT prints what a sphere takes to bake with each
// material at the densities a change could be shown at.  Without it, one
// small bake says the stack works.
//
//////////////////////////////////////////////////////////////////////////
void TestSculptBakeReport(void)
{
	float32				afDensities[4] = { 128.0f, 64.0f, 32.0f, 16.0f };
	const char*			szName;
	Ptr<CSolidTexture>	pMaterial;
	int64				iWhole;
	int64				iBake;
	size				i;
	size				j;

	if (getenv("SCULPT_BAKE_REPORT") == NULL)
	{
		TestSculptBakeTimes(MakeSolidMarble(1), 8.0f, 1, &iWhole, &iBake);
		AssertTrue(iWhole >= 0);
		return;
	}

	printf("A sphere of radius %.1f: milliseconds to mesh, unwrap and bake (whole), and to bake alone (bake)\n", SCULPT_SPHERE_RADIUS);
	printf("%-8s", "");
	for (j = 0; j < 4; j++)
	{
		printf("  %3.0f/unit whole  bake", afDensities[j]);
	}
	printf("\n");
	for (i = 0; i < TEST_SCULPT_BAKE_MATERIALS; i++)
	{
		pMaterial = TestSculptBakeMaterial(i, &szName);
		printf("%-8s", szName);
		for (j = 0; j < 4; j++)
		{
			TestSculptBakeTimes(pMaterial, afDensities[j], MESH_BAKE_SAMPLES, &iWhole, &iBake);
			printf("  %14d %5d", (int)iWhole, (int)iBake);
		}
		printf("\n");
	}
	fflush(stdout);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void TestSculptBake(void)
{
	BeginTests();
	ObjectsInit();

	TestSculptBakeReport();

	ObjectsDiscard();
	ObjectsKill(false);
	TestStatistics();
}

