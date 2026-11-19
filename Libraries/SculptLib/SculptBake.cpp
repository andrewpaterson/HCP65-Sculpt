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
#include "BaseLib/Atomic.h"
#include "StandardLib/ClassDefines.h"
#include "StandardLib/Objects.h"
#include "StandardLib/ObjectsLock.h"
#include "SupportLib/MeshUnwrap.h"
#include "ProceduralLib/MeshShapeEditor.h"
#include "ProceduralLib/MeshShapeNormals.h"
#include "SculptBake.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptBake::Init(Ptr<CShape> pShape, Ptr<CSolidTexture> pTexture, float32 fTexelsPerUnit, int iSamples, float32 fRelief, SFloat32Vec3* psLight, bool bCoarse)
{
	PreInit();
	mpShape = pShape;
	mpTexture = pTexture;
	mfTexelsPerUnit = fTexelsPerUnit;
	miSamples = iSamples;
	mfRelief = fRelief;
	msLight = *psLight;
	mbCoarse = bCoarse;
	mpMesh = NULL;
	miState = SBS_Described;
	miStop = 0;
	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptBake::Class(void)
{
	M_Pointer(mpShape);
	M_Pointer(mpTexture);
	U_Float32(mfTexelsPerUnit);
	U_SInt(miSamples);
	U_Float32(mfRelief);
	U_3Float32(msLight);
	U_Bool(mbCoarse);
	M_Pointer(mpMesh);
	U_Int32(miState);
	U_Int32(miStop);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptBake::Free(void)
{
}


//////////////////////////////////////////////////////////////////////////
//
// An editor of an empty mesh whose stack meshes a shape, gives the mesh
// the shape's normals (so it's smooth), unwraps it and bakes a texture
// onto it.  Apply says which shape and texture, and how.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CMeshEditor> CSculptBake::MakeEditor(void)
{
	Ptr<CMeshEditor>	pEditor;

	pEditor = OMalloc<CMeshEditor>(OMalloc<CMesh>());
	pEditor->AddModifier<CMeshShapeEditor>(Ptr<CShape>(), SCULPT_SMALLEST_CELL, SCULPT_LARGEST_CELL);
	pEditor->AddModifier<CMeshShapeNormals>(Ptr<CShape>());
	pEditor->AddModifier<CMeshUnwrap>(1.0f);
	pEditor->AddModifier<CMeshBake>(Ptr<CSolidTexture>());
	return pEditor;
}


//////////////////////////////////////////////////////////////////////////
//
// MakeEditor's modifiers are given what this describes.  The stack isn't
// touched: whether its output is to be made again is the caller's to say.
//
//////////////////////////////////////////////////////////////////////////
void CSculptBake::Apply(CMeshEditor* pcEditor)
{
	Ptr<CMeshBake>	pBake;

	pcEditor->mcModifiers.GetModifier<CMeshShapeEditor>()->mpShape = mpShape;
	pcEditor->mcModifiers.GetModifier<CMeshShapeNormals>()->mpShape = mpShape;
	pcEditor->mcModifiers.GetModifier<CMeshUnwrap>()->mfTexelsPerUnit = mfTexelsPerUnit;
	pBake = pcEditor->mcModifiers.GetModifier<CMeshBake>();
	pBake->mpTexture = mpTexture;
	pBake->miSamples = miSamples;
	pBake->mfRelief = mfRelief;
	pBake->msLight = msLight;
}


//////////////////////////////////////////////////////////////////////////
//
// The mesh is made by a stack of the calling thread's own, which it lets
// go of.  On a worker the stack and what it makes on the way are the
// worker's objects, freed there; the mesh, with its material and images,
// is adopted by the main object system before this, a main object, is
// pointed at it.  Stopped, what the stack made isn't whole and goes with
// the stack.
//
//////////////////////////////////////////////////////////////////////////
void CSculptBake::Make(CJobs* pcJobs)
{
	Ptr<CMeshEditor>	pEditor;
	Ptr<CMeshBake>		pBake;
	Ptr<CMesh>			pMesh;

	pEditor = MakeEditor();
	Apply(&pEditor);
	pBake = pEditor->mcModifiers.GetModifier<CMeshBake>();
	pBake->SetJobs(pcJobs);
	pBake->SetStop(&miStop);
	pMesh = pEditor->GetOutput();
	if (AtomicLoad(&miStop) != 0)
	{
		SetState(SBS_Dropped);
		return;
	}

	if ((ObjectsOwn() != NULL) && !ObjectsAdopt(pMesh))
	{
		pMesh = NULL;  //Adopt has said why.  A main object can't point at a worker's.
	}
	mpMesh = pMesh;
	SetState(SBS_Baked);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptBake::Stop(void)
{
	AtomicStore(&miStop, 1);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
ESculptBakeState CSculptBake::GetState(void)
{
	return (ESculptBakeState)AtomicLoad(&miState);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptBake::SetState(ESculptBakeState eState)
{
	AtomicStore(&miState, (int32)eState);
}


bool CSculptBake::IsBaked(void) { return GetState() == SBS_Baked; }
bool CSculptBake::IsDropped(void) { return GetState() == SBS_Dropped; }
