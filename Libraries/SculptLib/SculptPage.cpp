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
#include "BaseLib/Float4x4.h"
#include "StandardLib/ClassDefines.h"
#include "StandardLib/Objects.h"
#include "SupportLib/Camera.h"
#include "SupportLib/Instance.h"
#include "SupportLib/ShapeSmoothUnion.h"
#include "ProceduralLib/SolidMaterials.h"
#include "SupportLib/Sphere.h"
#include "WindowLib/Window.h"
#include "SculptPage.h"


#define SCULPT_CAMERA_DISTANCE	10.0f	//From the plane z = 0, where the view starts.
#define SCULPT_CAMERA_FOV		(3.14159265f / 3.0f)


//////////////////////////////////////////////////////////////////////////
//
// bVulkan false has the view drawn by the rasteriser without a device
// for Vulkan being looked for: the tests.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::Init(Ptr<CWindow> pWindow, SDialogLook* psLook, Ptr<CFontColours> pFontColours, bool bVulkan)
{
	PreInit();

	CGridContainer::Init(pWindow);
	SetAlignment(CSH_Stretch, CSV_Stretch);
	miSpheres = 0;
	mbInScene = false;
	mbCoarse = false;

	mpStatus = OMalloc<CLabel>(pWindow, pFontColours->GetFont(psLook->pcTheme->GetColour(TC_Text)), "No spheres.");
	mpStatus->SetAlignment(CSH_Left, CSV_Center);
	mpHelp = OMalloc<CLabel>(pWindow, pFontColours->GetFont(psLook->pcTheme->GetColour(TC_Icon)), "Click to drop a sphere on the plane of the screen.  Middle drag to turn, with Shift to pan, wheel to zoom.  R renderer, Home view, Delete clears.");
	mpHelp->SetAlignment(CSH_Left, CSV_Center);

	mpView = OMalloc<CSceneView>(pWindow, psLook->pcTheme, &mpStatus, SVB_MiddleTurns);
	mpView->GetDraw()->UseVulkan(bVulkan);
	mpView->GetClickEvent()->Add(this, &CSculptPage::Clicked);
	mpView->AddInputAction(IAF_KeyDown, "Delete", this, &CSculptPage::DeleteAction);
	mpView->AddInputAction(IAF_KeyDown, "X", this, &CSculptPage::DeleteAction);

	mpEditor = OMalloc<CNodeEditor>(pWindow, psLook->pcTheme);
	mpNodes = OMalloc<CSculptNodes>(mpEditor);
	mpNodes->GetChangeEvent()->Add(this, &CSculptPage::MaterialChanged);
	mpSplit = OMalloc<CSplitContainer>(pWindow, psLook->pcTheme, false);
	mpSplit->SetAlignment(CSH_Stretch, CSV_Stretch);
	mpSplit->SetComponents(mpView, mpEditor);
	mpSplit->SetShare(SCULPT_VIEW_SHARE);

	AddComponent(mpStatus, 0, 0, 1, 1, 1, 0);
	AddComponent(mpHelp, 0, 1, 1, 1, 1, 0);
	AddComponent(mpSplit, 0, 2, 1, 1, 1, 1);

	MakeScene();
	SetMaterial(MakeSolidMarble(SCULPT_MATERIAL_SEED));

	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::Class(void)
{
	CGridContainer::Class();

	M_Pointer(mpStatus);
	M_Pointer(mpHelp);
	M_Pointer(mpView);
	M_Pointer(mpSplit);
	M_Pointer(mpEditor);
	M_Pointer(mpNodes);
	M_Pointer(mpScene);
	M_Pointer(mpMesh);
	M_Pointer(mpShapeEditor);
	M_Pointer(mpShapeNormals);
	M_Pointer(mpUnwrap);
	M_Pointer(mpBake);
	M_Pointer(mpShape);
	U_Int32(miSpheres);
	U_Bool(mbInScene);
	U_Bool(mbCoarse);
}


//////////////////////////////////////////////////////////////////////////
//
// The view and the nodes may outlive the page (something else may hold
// them), so it's no longer told of clicks and changes; nor is the whole
// bake still to come.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::Free(void)
{
	if (mpWindow.IsNotNull())
	{
		mpWindow->CancelPosted(this);
	}
	if (mpView.IsNotNull())
	{
		mpView->GetClickEvent()->Remove(this);
	}
	if (mpNodes.IsNotNull())
	{
		mpNodes->GetChangeEvent()->Remove(this);
	}
	CGridContainer::Free();
}


//////////////////////////////////////////////////////////////////////////
//
// A camera ten from the origin along z, looking back along -z, so the
// plane of the screen starts as z = 0, and an editor whose stack meshes
// the spheres, gives them the shape's normals (so they're smooth),
// unwraps them and bakes the material onto them (SetMaterial gives it
// one).  The editor's instance goes in with the first sphere: until
// then there's nothing to draw.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::MakeScene(void)
{
	Ptr<CCamera>	pCamera;

	mpScene = OMalloc<CScene>();

	pCamera = OMalloc<CCamera>();
	pCamera->mfFOV = SCULPT_CAMERA_FOV;
	pCamera->mpConnection = OMalloc<CConnection>();
	Float4x4Translation(&pCamera->mpConnection->msWorldMatrix, 0.0f, 0.0f, SCULPT_CAMERA_DISTANCE);
	pCamera->mpConnection->msLocalMatrix = pCamera->mpConnection->msWorldMatrix;
	mpScene->mapCameras.Add(pCamera);

	mpMesh = OMalloc<CMeshEditor>(OMalloc<CMesh>());
	mpShapeEditor = mpMesh->AddModifier<CMeshShapeEditor>(Ptr<CShape>(), SCULPT_SMALLEST_CELL, SCULPT_LARGEST_CELL);
	mpShapeNormals = mpMesh->AddModifier<CMeshShapeNormals>(Ptr<CShape>());
	mpUnwrap = mpMesh->AddModifier<CMeshUnwrap>(SCULPT_TEXELS_PER_UNIT);
	mpBake = mpMesh->AddModifier<CMeshBake>(Ptr<CSolidTexture>());

	mpView->GetDraw()->SetScene(mpScene, "Sculpt: no spheres");
}


//////////////////////////////////////////////////////////////////////////
//
// The editor's instance, placed where its spheres are.
//
//////////////////////////////////////////////////////////////////////////
static Ptr<CInstance> SculptInstance(Ptr<CMeshEditor> pEditor)
{
	Ptr<CInstance>		pInstance;
	Ptr<CInstanceMesh>	pInstanceMesh;
	Ptr<CConnection>	pConnection;

	pConnection = OMalloc<CConnection>();
	pConnection->msWorldMatrix.Identity();
	pConnection->msLocalMatrix.Identity();
	pInstanceMesh = OMalloc<CInstanceMesh>(pEditor);
	pInstanceMesh->mapConnections.Add(pConnection);
	pInstance = OMalloc<CInstance>();
	pInstance->mapConnections.Add(pConnection);
	pInstance->mapMeshes.Add(pInstanceMesh);
	return pInstance;
}


//////////////////////////////////////////////////////////////////////////
//
// The editor's stack meshes mpShape again, its instance is in the scene
// while there are spheres, and the view is drawn again: coarsely now and
// finely once nothing more changes.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::Remesh(void)
{
	mpShapeEditor->mpShape = mpShape;
	mpShapeNormals->mpShape = mpShape;
	Rebake(true);
}


//////////////////////////////////////////////////////////////////////////
//
// The stack makes its mesh again (Touch moves its modifier counter,
// which the render scene makes a new mesh for), unwrapped and baked at
// the coarse density or the whole one.  A coarse bake posts the whole
// one for SCULPT_FINE_PAUSE from now, in place of any already posted.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::Rebake(bool bCoarse)
{
	CChars	szName;

	mpWindow->CancelPosted(this);
	mbCoarse = bCoarse;
	if (mbCoarse)
	{
		mpWindow->PostAfter(SCULPT_FINE_PAUSE, this, &CSculptPage::BakeFine);
	}
	mpUnwrap->mfTexelsPerUnit = SCULPT_TEXELS_PER_UNIT * (mbCoarse ? SCULPT_COARSE_SHARE : 1.0f);
	mpMesh->mcModifiers.Touch();
	if ((miSpheres > 0) && !mbInScene)
	{
		mpScene->mapInstances.Add(SculptInstance(mpMesh));
		mbInScene = true;
	}
	else if ((miSpheres == 0) && mbInScene)
	{
		mpScene->mapInstances.RemoveAll();
		mbInScene = false;
	}

	szName.Init("Sculpt: ");
	if (miSpheres == 0)
	{
		szName.Append("no spheres");
	}
	else
	{
		szName.Append(miSpheres);
		szName.Append((miSpheres == 1) ? " sphere" : " spheres");
	}
	mpView->GetDraw()->SceneChanged(szName.Text());
	szName.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSceneView> CSculptPage::GetView(void)
{
	return mpView;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSculptNodes> CSculptPage::GetNodes(void)
{
	return mpNodes;
}


//////////////////////////////////////////////////////////////////////////
//
// The material is the one pSurface is the top of: its graph is in the
// node editor and it's baked onto what's made.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::SetMaterial(Ptr<CSolidTexture> pSurface)
{
	mpNodes->SetGraph(OMalloc<CSculptGraph>(pSurface));
	MaterialChanged();
}


//////////////////////////////////////////////////////////////////////////
//
// The graph's surface, which may be another texture now, is baked again.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::MaterialChanged(void)
{
	mpBake->mpTexture = mpNodes->GetGraph()->GetSurface();
	Rebake(true);
}


//////////////////////////////////////////////////////////////////////////
//
// What's shown is baked at the coarse density: the whole one is to come.
//
//////////////////////////////////////////////////////////////////////////
bool CSculptPage::IsCoarse(void)
{
	return mbCoarse;
}


//////////////////////////////////////////////////////////////////////////
//
// Nothing has changed for a while: what's shown is baked at the whole
// density.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::BakeFine(void)
{
	if (mbCoarse)
	{
		Rebake(false);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A sphere where the view's point x, y is on the plane of the screen,
// joined to the others by a smooth union.  False if the point isn't on
// the plane (the view hasn't been drawn yet).
//
//////////////////////////////////////////////////////////////////////////
bool CSculptPage::DropSphere(int32 x, int32 y)
{
	SFloat32Vec3	sCentre;
	Ptr<CSphere>	pSphere;

	if (!mpView->GetDraw()->ScreenToPlane(x, y, &sCentre))
	{
		return false;
	}

	pSphere = OMalloc<CSphere>(&sCentre, SCULPT_SPHERE_RADIUS);
	if (mpShape.IsNull())
	{
		mpShape = pSphere;
	}
	else
	{
		mpShape = OMalloc<CShapeSmoothUnion>(mpShape, pSphere, SCULPT_BLEND_RADIUS);
	}
	miSpheres++;
	Remesh();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::Clear(void)
{
	mpShape = NULL;
	miSpheres = 0;
	Remesh();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int32 CSculptPage::NumSpheres(void)
{
	return miSpheres;
}


//////////////////////////////////////////////////////////////////////////
//
// The view was clicked: a sphere is dropped where.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::Clicked(CSceneView* pcView)
{
	SInt32Vec2	sWhere;

	sWhere = pcView->GetClick();
	DropSphere(sWhere.x, sWhere.y);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::DeleteAction(SComponentInputEvent* psEvent)
{
	Clear();
}

