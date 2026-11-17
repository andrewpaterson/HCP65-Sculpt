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
#include "SupportLib/SolidChecker.h"
#include "SupportLib/Sphere.h"
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

	mpStatus = OMalloc<CLabel>(pWindow, pFontColours->GetFont(psLook->pcTheme->GetColour(TC_Text)), "No spheres.");
	mpStatus->SetAlignment(CSH_Left, CSV_Center);
	mpHelp = OMalloc<CLabel>(pWindow, pFontColours->GetFont(psLook->pcTheme->GetColour(TC_Icon)), "Click to drop a sphere on the plane of the screen.  Middle drag to turn, with Shift to pan, wheel to zoom.  R renderer, Home view, Delete clears.");
	mpHelp->SetAlignment(CSH_Left, CSV_Center);

	mpView = OMalloc<CSceneView>(pWindow, psLook->pcTheme, &mpStatus, SVB_MiddleTurns);
	mpView->GetDraw()->UseVulkan(bVulkan);
	mpView->GetClickEvent()->Add(this, &CSculptPage::Clicked);
	mpView->AddInputAction(IAF_KeyDown, "Delete", this, &CSculptPage::DeleteAction);
	mpView->AddInputAction(IAF_KeyDown, "X", this, &CSculptPage::DeleteAction);

	AddComponent(mpStatus, 0, 0, 1, 1, 1, 0);
	AddComponent(mpHelp, 0, 1, 1, 1, 1, 0);
	AddComponent(mpView, 0, 2, 1, 1, 1, 1);

	MakeScene();

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
	M_Pointer(mpScene);
	M_Pointer(mpEditor);
	M_Pointer(mpShapeEditor);
	M_Pointer(mpShapeNormals);
	M_Pointer(mpUnwrap);
	M_Pointer(mpBake);
	M_Pointer(mpShape);
	U_Int32(miSpheres);
	U_Bool(mbInScene);
}


//////////////////////////////////////////////////////////////////////////
//
// The view may outlive the page (something else may hold it), so it's
// no longer told of clicks.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::Free(void)
{
	if (mpView.IsNotNull())
	{
		mpView->GetClickEvent()->Remove(this);
	}
	CGridContainer::Free();
}


//////////////////////////////////////////////////////////////////////////
//
// A camera ten from the origin along z, looking back along -z, so the
// plane of the screen starts as z = 0, and an editor whose stack meshes
// the spheres, gives them the shape's normals (so they're smooth),
// unwraps them and bakes a checker onto them.  The editor's instance
// goes in with the first sphere: until then there's nothing to draw.
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

	mpEditor = OMalloc<CMeshEditor>(OMalloc<CMesh>());
	mpShapeEditor = mpEditor->AddModifier<CMeshShapeEditor>(Ptr<CShape>(), SCULPT_SMALLEST_CELL, SCULPT_LARGEST_CELL);
	mpShapeNormals = mpEditor->AddModifier<CMeshShapeNormals>(Ptr<CShape>());
	mpUnwrap = mpEditor->AddModifier<CMeshUnwrap>(SCULPT_TEXELS_PER_UNIT);
	mpBake = mpEditor->AddModifier<CMeshBake>(OMalloc<CSolidChecker>(SCULPT_CHECKER_SIZE, Set32BitColour((uint8)230, (uint8)220, (uint8)200), Set32BitColour((uint8)60, (uint8)90, (uint8)150)));

	//A checker averages itself over a texel exactly and has no height, so one sample a texel is all of it: the spheres are
	//baked again at every click.
	mpBake->miSamples = 1;
	mpBake->mfRelief = 0.0f;

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
// The editor's stack meshes mpShape again (Touch moves its modifier
// counter, which the render scene makes a new mesh for), its instance is
// in the scene while there are spheres, and the view is drawn again.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::Remesh(void)
{
	CChars	szName;

	mpShapeEditor->mpShape = mpShape;
	mpShapeNormals->mpShape = mpShape;
	mpEditor->mcModifiers.Touch();
	if ((miSpheres > 0) && !mbInScene)
	{
		mpScene->mapInstances.Add(SculptInstance(mpEditor));
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

