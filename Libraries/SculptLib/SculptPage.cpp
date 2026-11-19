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
#include "StandardLib/NamedObjectFile.h"
#include "StandardLib/Objects.h"
#include "SupportLib/Camera.h"
#include "SupportLib/Instance.h"
#include "SupportLib/ShapeSmoothUnion.h"
#include "ProceduralLib/SolidMaterialFile.h"
#include "ProceduralLib/SolidMaterials.h"
#include "SculptDocument.h"
#include "SupportLib/Box.h"
#include "SupportLib/ShapeUnion.h"
#include "SupportLib/Sphere.h"
#include "ProceduralLib/SolidChannel.h"
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
	mpHelp = OMalloc<CLabel>(pWindow, pFontColours->GetFont(psLook->pcTheme->GetColour(TC_Icon)), "Middle drag to turn, with Shift to pan, wheel to zoom.  R renderer, Home view.  Sculpted: click to drop a sphere, Delete clears.");
	mpHelp->SetAlignment(CSH_Left, CSV_Center);

	mpView = OMalloc<CSceneView>(pWindow, psLook->pcTheme, &mpStatus, SVB_MiddleTurns);
	mpView->GetDraw()->UseVulkan(bVulkan);
	mpView->GetClickEvent()->Add(this, &CSculptPage::Clicked);
	mpView->AddInputAction(IAF_KeyDown, "Delete", this, &CSculptPage::DeleteAction);
	mpView->AddInputAction(IAF_KeyDown, "X", this, &CSculptPage::DeleteAction);

	mpEditor = OMalloc<CNodeEditor>(pWindow, psLook->pcTheme);
	mpNodes = OMalloc<CSculptNodes>(mpEditor);
	mpNodes->GetChangeEvent()->Add(this, &CSculptPage::MaterialChanged);
	AddInputAction(CIC_Undo, &mpNodes, &CSculptNodes::UndoCommand);  //Wherever in the page the focus is; the editor has its own.
	AddInputAction(CIC_Redo, &mpNodes, &CSculptNodes::RedoCommand);
	mpSplit = OMalloc<CSplitContainer>(pWindow, psLook->pcTheme, false);
	mpSplit->SetAlignment(CSH_Stretch, CSV_Stretch);
	mpSplit->SetComponents(mpView, mpEditor);
	mpSplit->SetShare(SCULPT_VIEW_SHARE);
	mpPanel = OMalloc<CSculptPanel>(pWindow, psLook->pcTheme);
	mpPanel->GetChangeEvent()->Add(this, &CSculptPage::PanelChanged);

	AddComponent(mpStatus, 0, 0, 2, 1, 1, 0);
	AddComponent(mpHelp, 0, 1, 2, 1, 1, 0);
	AddComponent(mpSplit, 0, 2, 1, 1, 1, 1);
	AddComponent(mpPanel, 1, 2, 1, 1, 0, 1);

	mapBakes.Init();
	MakeScene();
	mpPanel->SetBake(GetBake()->miSamples, GetBake()->mfRelief, &GetBake()->msLight);
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
	M_Pointer(mpPanel);
	M_Pointer(mpScene);
	M_Pointer(mpMesh);
	M_Pointer(mpSculpted);
	M_Pointer(mpBaker);
	M_Embedded(mapBakes);
	U_Int32(miSpheres);
	U_Bool(mbInScene);
	U_Bool(mbCoarse);
}


//////////////////////////////////////////////////////////////////////////
//
// The view and the nodes may outlive the page (something else may hold
// them), so it's no longer told of clicks and changes; nor is the whole
// bake still to come.  A baker finishes what it was given without the
// page: it holds the bakes itself.
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
	if (mpPanel.IsNotNull())
	{
		mpPanel->GetChangeEvent()->Remove(this);
	}
	CGridContainer::Free();
}


//////////////////////////////////////////////////////////////////////////
//
// A camera ten from the origin along z, looking back along -z, so the
// plane of the screen starts as z = 0, and an editor whose stack meshes
// the shape and bakes the material onto it (CSculptBake::MakeEditor;
// SetMaterial gives it a material).  The editor's instance goes in with
// the first shape: until then there's nothing to draw.
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

	mpMesh = CSculptBake::MakeEditor();

	mpView->GetDraw()->SetScene(mpScene, "Sculpt: no spheres");
}


//////////////////////////////////////////////////////////////////////////
//
// The editor's instance, stood up: the materials are made with z up
// (bricks' courses, wood's grain, moss on what faces up) and the view
// turns about y, so the mesh's z is the view's y and its -y, its front,
// faces the view.
//
//////////////////////////////////////////////////////////////////////////
static Ptr<CInstance> SculptInstance(Ptr<CMeshEditor> pEditor)
{
	Ptr<CInstance>		pInstance;
	Ptr<CInstanceMesh>	pInstanceMesh;
	Ptr<CConnection>	pConnection;

	pConnection = OMalloc<CConnection>();
	pConnection->msWorldMatrix.Identity();
	pConnection->msWorldMatrix.y.Init(0, 0, -1, 0);
	pConnection->msWorldMatrix.z.Init(0, 1, 0, 0);
	pConnection->msLocalMatrix = pConnection->msWorldMatrix;
	pInstanceMesh = OMalloc<CInstanceMesh>(pEditor);
	pInstanceMesh->mapConnections.Add(pConnection);
	pInstance = OMalloc<CInstance>();
	pInstance->mapConnections.Add(pConnection);
	pInstance->mapMeshes.Add(pInstanceMesh);
	return pInstance;
}


//////////////////////////////////////////////////////////////////////////
//
// What the material is looked at on, as the panel has it: a sphere a
// unit in radius, a box, the two met as the gallery has them (the box
// turned about z, so its sides aren't square to the texture's axes), or
// the spheres dropped.  NULL for no spheres yet.  A box's top and bottom
// are kept off the tenths of a unit, where bricks' courses meet: a face
// that lies in the mortar between two courses is all mortar.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CShape> CSculptPage::GetShape(void)
{
	Ptr<CShapeUnion>	pUnion;
	SFloat32Box			sBox;
	SFloat32Vec3		sCentre;
	SFloat32Vec3		asAxes[3];
	ESculptShape		eShape;

	eShape = mpPanel->GetShape();
	if (eShape == SS_Sculpted)
	{
		return mpSculpted;
	}
	if (eShape == SS_Sphere)
	{
		sCentre.Init(0, 0, 0);
		return OMalloc<CSphere>(&sCentre, SCULPT_SPHERE_RADIUS);
	}
	if (eShape == SS_Box)
	{
		sCentre.Init(0, 0, 0);
		asAxes[0].Init(1, 0, 0);
		asAxes[1].Init(0, 1, 0);
		asAxes[2].Init(0, 0, 1);
		sBox.Init(&sCentre, &asAxes[0], 0.8f, &asAxes[1], 0.8f, &asAxes[2], 0.75f);
		return OMalloc<CBox>(&sBox);
	}

	pUnion = OMalloc<CShapeUnion>();
	sCentre.Init(-1.05f, 0.0f, 0.0f);
	pUnion->Add(OMalloc<CSphere>(&sCentre, SCULPT_SPHERE_RADIUS));
	sCentre.Init(1.0f, 0.0f, -0.25f);
	asAxes[0].Init(0.5f, 0.8660254f, 0.0f);
	asAxes[1].Init(-0.8660254f, 0.5f, 0.0f);
	asAxes[2].Init(0, 0, 1);
	sBox.Init(&sCentre, &asAxes[0], 0.8f, &asAxes[1], 0.8f, &asAxes[2], 0.8f);
	pUnion->Add(OMalloc<CBox>(&sBox));
	return pUnion;
}


//////////////////////////////////////////////////////////////////////////
//
// What's baked of pSurface, a material's surface: it, or for one of its
// channels a grey of that channel of it.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSolidTexture> CSculptPage::GetShown(Ptr<CSolidTexture> pSurface)
{
	switch (mpPanel->GetShow())
	{
	case SSH_Height:	return OMalloc<CSolidChannel>(SCH_Height, pSurface);
	case SSH_Shine:		return OMalloc<CSolidChannel>(SCH_Shine, pSurface);
	case SSH_Gloss:		return OMalloc<CSolidChannel>(SCH_Gloss, pSurface);
	case SSH_Glow:		return OMalloc<CSolidChannel>(SCH_Glow, pSurface);
	default:			return pSurface;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// What the page shows as it is now: the panel's shape and the material's
// surface, baked as the panel says at the coarse density or the whole
// one.  Only the baked show has relief, as a channel's greys and the
// colour alone are the texture's own.  A light of no length leaves the
// bake's as it was.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSculptBake> CSculptPage::DescribeBake(bool bCoarse)
{
	SFloat32Vec3	sLight;

	mpPanel->GetLight(&sLight);
	if (sLight.Magnitude() > 0.0f)
	{
		sLight.Normalize();
	}
	else
	{
		sLight = GetBake()->msLight;
	}
	return OMalloc<CSculptBake>(GetShape(), GetShown(mpNodes->GetGraph()->GetSurface()),
			SCULPT_TEXELS_PER_UNIT * (bCoarse ? SCULPT_COARSE_SHARE : 1.0f),
			bCoarse ? 1 : mpPanel->GetSamples(),  //A coarse bake is to see a change as it's made: one sample a texel.
			(mpPanel->GetShow() == SSH_Baked) ? mpPanel->GetRelief() : 0.0f, &sLight, bCoarse);
}


//////////////////////////////////////////////////////////////////////////
//
// What the page shows is baked again, as it is now.  The stack is given
// the bake's settings, and either makes its mesh again itself (Touch
// moves its modifier counter, which the render scene makes a new mesh
// for), or, with a baker, is left showing what it does until the baker's
// thread has made the mesh (ShowBaked): the baker is given the bake with
// a copy of the material, which nothing changes while it's read there.
// With no shape there's nothing to make, and nothing asked for earlier
// is shown.  A coarse bake posts the whole one for SCULPT_FINE_PAUSE from
// now, in place of any already posted.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::Rebake(bool bCoarse)
{
	Ptr<CSculptBake>	pBake;

	pBake = DescribeBake(bCoarse);
	pBake->Apply(&mpMesh);

	mpWindow->CancelPosted(this);
	mbCoarse = bCoarse;
	if (mbCoarse)
	{
		mpWindow->PostAfter(SCULPT_FINE_PAUSE, this, &CSculptPage::BakeFine);
	}

	if (mpBaker.IsNotNull() && pBake->mpShape.IsNotNull())
	{
		pBake->mpTexture = GetShown(mpNodes->GetGraph()->Copy()->GetSurface());
		mapBakes.Add(pBake);
		mpBaker->Bake(pBake);
		mpWindow->PostAfter(SCULPT_BAKED_PAUSE, this, &CSculptPage::ShowBaked);
	}
	else
	{
		mapBakes.RemoveAll();
		mpMesh->mcModifiers.Touch();
		Show(pBake->mpShape.IsNotNull(), bCoarse);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The newest bake the baker has made is shown: its mesh is the stack's
// output, as if the stack had made it, and the view is given it cooked,
// as the baker's thread cooked it.  That bake and those before it are
// done with, as are any the baker dropped.  While there are others still
// to be made this is posted again.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::ShowBaked(void)
{
	Ptr<CSculptBake>	pBake;
	size				uiDone;
	size				i;

	uiDone = 0;
	for (i = mapBakes.NumElements(); (i > 0) && (uiDone == 0); i--)
	{
		pBake = mapBakes.Get(i - 1);
		if (pBake->IsBaked())
		{
			if (pBake->mpMesh.IsNotNull())
			{
				mpMesh->SetOutput(pBake->mpMesh);
				mpView->GetDraw()->SetCooked(mpMesh, pBake->mpCooked);
				Show(true, pBake->mbCoarse);
			}
			uiDone = i;
		}
	}
	for (i = 0; i < uiDone; i++)
	{
		mapBakes.RemoveAt(0);
	}
	for (i = mapBakes.NumElements(); i > 0; i--)
	{
		if (mapBakes.Get(i - 1)->IsDropped())
		{
			mapBakes.RemoveAt(i - 1);
		}
	}

	if (IsBaking())
	{
		mpWindow->PostAfter(SCULPT_BAKED_PAUSE, this, &CSculptPage::ShowBaked);
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CSculptPage::IsBaking(void)
{
	return mapBakes.NumElements() > 0;
}


//////////////////////////////////////////////////////////////////////////
//
// The view is told what it shows now: the editor's instance is in the
// scene while there's a shape, and the status says what of, and whether
// at the coarse density.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::Show(bool bShape, bool bCoarse)
{
	CChars	szName;

	if (bShape && !mbInScene)
	{
		mpScene->mapInstances.Add(SculptInstance(mpMesh));
		mbInScene = true;
	}
	else if (!bShape && mbInScene)
	{
		mpScene->mapInstances.RemoveAll();
		mbInScene = false;
	}

	szName.Init("Sculpt: ");
	if (mpPanel->GetShape() != SS_Sculpted)
	{
		szName.Append("a material");
	}
	else if (miSpheres == 0)
	{
		szName.Append("no spheres");
	}
	else
	{
		szName.Append(miSpheres);
		szName.Append((miSpheres == 1) ? " sphere" : " spheres");
	}
	if (bCoarse)
	{
		szName.Append(", coarse");
	}
	mpView->GetDraw()->SceneChanged(szName.Text());
	szName.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSceneView> CSculptPage::GetView(void) const
{
	return mpView;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSculptNodes> CSculptPage::GetNodes(void) const
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
// The material as it would be after years outside on the shape now
// shown (MakeSolidWeathered): dirt in the shape's hollows, wear along its
// edges and moss on what faces up, round the material there is.  Nothing
// with no shape to weather.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::WeatherMaterial(void)
{
	Ptr<CShape>		pShape;

	pShape = GetShape();
	if (pShape.IsNotNull())
	{
		SetMaterial(MakeSolidWeathered(mpNodes->GetGraph()->GetSurface(), pShape, SCULPT_MATERIAL_SEED));
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The material's graph with its nodes where they are, the spheres
// dropped and the panel's settings, written to szFilename (with the
// object files' extension, given or not).  What's written is the page's
// own objects: nothing is copied.
//
//////////////////////////////////////////////////////////////////////////
bool CSculptPage::Save(const char* szFilename)
{
	Ptr<CSculptDocument>	pDocument;

	pDocument = gcObjects.Malloc<CSculptDocument>(SCULPT_DOCUMENT_NAME);
	if (pDocument.IsNull())
	{
		return false;
	}
	pDocument->Init();
	pDocument->mpGraph = mpNodes->GetGraph();
	pDocument->mpSculpted = mpSculpted;
	pDocument->miSpheres = miSpheres;
	pDocument->miShape = mpPanel->GetShape();
	pDocument->miShow = mpPanel->GetShow();
	pDocument->miSamples = mpPanel->GetSamples();
	pDocument->mfRelief = mpPanel->GetRelief();
	mpPanel->GetLight(&pDocument->msLight);
	return WriteNamedObjectFile(&pDocument, szFilename);
}


//////////////////////////////////////////////////////////////////////////
//
// A Sculpt file read: its graph is shown, its spheres are the sculpted
// shape and its settings the panel's.  A file of a material alone is
// its graph, arranged, and nothing else changes.  False, with nothing
// changed, if the file is neither.
//
//////////////////////////////////////////////////////////////////////////
bool CSculptPage::Open(const char* szFilename)
{
	Ptr<CSculptDocument>	pDocument;
	Ptr<CSolidTexture>		pSurface;

	AddSculptConstructors();
	pDocument = Ptr<CSculptDocument>(dynamic_cast<CSculptDocument*>(ReadNamedObjectFile(szFilename, SCULPT_DOCUMENT_NAME)));
	if (pDocument.IsNull() || pDocument->mpGraph.IsNull())
	{
		pSurface = ReadSolidMaterial(szFilename);
		if (pSurface.IsNull())
		{
			return false;
		}
		SetMaterial(pSurface);
		return true;
	}

	mpSculpted = pDocument->mpSculpted;
	miSpheres = pDocument->miSpheres;
	mpPanel->SetShape((ESculptShape)pDocument->miShape);
	mpPanel->SetShow((ESculptShow)pDocument->miShow);
	mpPanel->SetBake(pDocument->miSamples, pDocument->mfRelief, &pDocument->msLight);
	mpNodes->SetGraph(pDocument->mpGraph);
	Rebake(true);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The material alone, as another program reads it.
//
//////////////////////////////////////////////////////////////////////////
bool CSculptPage::ExportMaterial(const char* szFilename)
{
	return WriteSolidMaterial(mpNodes->GetGraph()->GetSurface(), szFilename);
}


//////////////////////////////////////////////////////////////////////////
//
// The graph's surface, which may be another texture now, is baked again.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::MaterialChanged(void)
{
	Rebake(true);
}


//////////////////////////////////////////////////////////////////////////
//
// The panel's shape, show or bake settings were changed.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::PanelChanged(void)
{
	Rebake(true);
}


Ptr<CSculptPanel> CSculptPage::GetPanel(void) const { return mpPanel; }
Ptr<CMeshBake> CSculptPage::GetBake(void) { return mpMesh->mcModifiers.GetModifier<CMeshBake>(); }
void CSculptPage::SetJobs(CJobs* pcJobs) { GetBake()->SetJobs(pcJobs); }


//////////////////////////////////////////////////////////////////////////
//
// Bakes are pBaker's thread's to make from now; with NULL, the stack's
// own again.  What the page shows is asked for again from whichever it
// is.  Given a baker, the editor's instance leaves the scene until the
// baker has made that, so that nothing asks the stack for a mesh it
// hasn't made, which it would make then and there.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::SetBaker(Ptr<CSculptBaker> pBaker)
{
	mpBaker = pBaker;
	if (mpBaker.IsNotNull() && mbInScene)
	{
		mpScene->mapInstances.RemoveAll();
		mbInScene = false;
	}
	Rebake(mbCoarse);
}


//////////////////////////////////////////////////////////////////////////
//
// The last bake asked for is at the coarse density: the whole one is to
// come.
//
//////////////////////////////////////////////////////////////////////////
bool CSculptPage::IsCoarse(void) const
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
// the plane (the view hasn't been drawn yet), or the shape shown isn't
// the sculpted one.
//
//////////////////////////////////////////////////////////////////////////
bool CSculptPage::DropSphere(int32 x, int32 y)
{
	SFloat32Vec3	sCentre;
	Ptr<CSphere>	pSphere;

	if ((mpPanel->GetShape() != SS_Sculpted) || !mpView->GetDraw()->ScreenToPlane(x, y, &sCentre))
	{
		return false;
	}

	//The point is the view's; the shape is the mesh's, which is stood up in the view (SculptInstance).
	sCentre.Init(sCentre.x, -sCentre.z, sCentre.y);
	pSphere = OMalloc<CSphere>(&sCentre, SCULPT_SPHERE_RADIUS);
	if (mpSculpted.IsNull())
	{
		mpSculpted = pSphere;
	}
	else
	{
		mpSculpted = OMalloc<CShapeSmoothUnion>(mpSculpted, pSphere, SCULPT_BLEND_RADIUS);
	}
	miSpheres++;
	Rebake(true);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptPage::Clear(void)
{
	mpSculpted = NULL;
	miSpheres = 0;
	Rebake(true);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
int32 CSculptPage::NumSpheres(void) const
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

