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
#include <stdlib.h>
#include <string.h>
#include "BaseLib/FileUtil.h"
#include "StandardLib/Objects.h"
#include "SupportLib/PNGWriter.h"
#include "WindowLib/Canvas.h"
#include "ProceduralLib/SolidChannel.h"
#include "ProceduralLib/SolidMaterialFile.h"
#include "ProceduralLib/SolidMaterials.h"
#include "SculptLib/SculptMaterials.h"
#include "SculptLib/SculptPage.h"
#include "SculptTestWindow.h"
#include "TestLib/Assert.h"


#define TEST_SCULPT_PAGE_WIDTH	1000
#define TEST_SCULPT_PAGE_HEIGHT	700


//What the page test's tick works on.
struct STestSculptPage
{
	CSculptPage*	pcPage;  //Held by the test.
	bool			bDropped;
};


//////////////////////////////////////////////////////////////////////////
//
// The first tick is before anything's drawn; at the third a sphere is
// dropped in the middle of the view, and two ticks on it's been meshed,
// baked and drawn.
//
//////////////////////////////////////////////////////////////////////////
static bool TestSculptPageTick(Ptr<CWindow> pWindow, int32 iTick, void* pvData)
{
	STestSculptPage*	psTest;
	SInt32Vec2			sSize;

	psTest = (STestSculptPage*)pvData;
	if (iTick == 3)
	{
		sSize = psTest->pcPage->GetView()->GetActualSize();
		psTest->bDropped = psTest->pcPage->DropSphere(sSize.x / 2, sSize.y / 2);
	}
	return iTick < 5;
}


//////////////////////////////////////////////////////////////////////////
//
// An event at x, y.
//
//////////////////////////////////////////////////////////////////////////
static SComponentInputEvent TestSculptPageEvent(int32 x, int32 y)
{
	SComponentInputEvent	sEvent;

	memset(&sEvent, 0, sizeof(sEvent));
	sEvent.x = x;
	sEvent.y = y;
	return sEvent;
}


//////////////////////////////////////////////////////////////////////////
//
// A sphere can't be dropped before the view's drawn (there's no plane
// of the screen yet).  Dropped in the middle it's drawn there, with the
// background at the view's corners.  A click on the view drops another,
// and Delete takes them away.  The page's material is marble, its graph
// in the node editor under the view, until it's given another.
//
//////////////////////////////////////////////////////////////////////////
void TestSculptPageSpheres(void)
{
	ObjectsInit();
	{
		Ptr<CWinRefWindowFactory>	pFactory;
		Ptr<CWindow>				pWindow;
		Ptr<CFontColours>			pFontColours;
		Ptr<CWindowTheme>			pTheme;
		Ptr<CSculptPage>			pPage;
		Ptr<CSceneView>				pView;
		Ptr<CImage>					pImage;
		SFloat32Vec3				sLight;
		SDialogLook					sLook;
		STestSculptPage				sTest;
		SComponentInputEvent		sEvent;
		SInt32Vec2					sSize;
		uint32						uiBackground;
		size						uiNodes;
		size						i;

		pWindow = SculptTestWindow(pFactory, TEST_SCULPT_PAGE_WIDTH, TEST_SCULPT_PAGE_HEIGHT, TestSculptPageTick, &sTest);
		AssertTrue(SculptTestLook(pWindow, &sLook, pFontColours, pTheme));

		pPage = OMalloc<CSculptPage>(pWindow, &sLook, pFontColours, false);
		pView = pPage->GetView();
		AssertInt(0, pPage->NumSpheres());

		//It starts with a sphere, baked with relief by a CMeshBake's own settings.
		AssertInt(SS_Sphere, pPage->GetPanel()->GetShape());
		AssertTrue(pPage->GetShape().IsNotNull());
		AssertInt(SSH_Baked, pPage->GetPanel()->GetShow());
		AssertInt(MESH_BAKE_SAMPLES, pPage->GetBake()->miSamples);
		AssertFloat(1.0f, pPage->GetBake()->mfRelief, 6);
		AssertTrue(pPage->GetBake()->mpTexture == pPage->GetNodes()->GetGraph()->GetSurface());
		AssertFalse(pPage->DropSphere(10, 10));  //Nothing's dropped on a sphere.

		//A box, and a sphere and a box, are shapes; a channel shown is a grey of it, with no relief; the colour alone
		//is the material with no relief.
		pPage->GetPanel()->SetShape(SS_Box);
		pPage->PanelChanged();
		AssertTrue(pPage->GetShape().IsNotNull());
		pPage->GetPanel()->SetShape(SS_Both);
		pPage->GetPanel()->SetShow(SSH_Height);
		pPage->PanelChanged();
		AssertTrue(pPage->GetShape().IsNotNull());
		AssertNotNull(dynamic_cast<CSolidChannel*>(&pPage->GetBake()->mpTexture));
		AssertInt(SCH_Height, Ptr<CSolidChannel>(pPage->GetBake()->mpTexture)->meChannel);
		AssertTrue(Ptr<CSolidChannel>(pPage->GetBake()->mpTexture)->mpInput == pPage->GetNodes()->GetGraph()->GetSurface());
		AssertFloat(0.0f, pPage->GetBake()->mfRelief, 6);
		pPage->GetPanel()->SetShow(SSH_Colour);
		pPage->PanelChanged();
		AssertTrue(pPage->GetBake()->mpTexture == pPage->GetNodes()->GetGraph()->GetSurface());
		AssertFloat(0.0f, pPage->GetBake()->mfRelief, 6);

		//The bake's settings are the panel's; the light is made a unit, and none leaves it as it was.
		sLight.Init(0, 0, 4);
		pPage->GetPanel()->SetBake(2, 0.5f, &sLight);
		pPage->GetPanel()->SetShow(SSH_Baked);
		pPage->PanelChanged();
		AssertInt(2, pPage->GetBake()->miSamples);
		AssertFloat(0.5f, pPage->GetBake()->mfRelief, 6);
		AssertFloat3(SFloat32Vec3(0, 0, 1), &pPage->GetBake()->msLight, 6);
		sLight.Init(0, 0, 0);
		pPage->GetPanel()->SetBake(1, 0.5f, &sLight);
		pPage->PanelChanged();
		AssertFloat3(SFloat32Vec3(0, 0, 1), &pPage->GetBake()->msLight, 6);

		//Sculpted, with nothing dropped, is no shape.
		pPage->GetPanel()->SetShape(SS_Sculpted);
		pPage->PanelChanged();
		AssertTrue(pPage->GetShape().IsNull());

		//It starts with marble: five nodes.
		AssertSize(5, pPage->GetNodes()->GetEditor()->NumNodes());
		AssertSize(4, pPage->GetNodes()->GetEditor()->NumLinks());
		AssertFalse(pPage->DropSphere(10, 10));
		AssertInt(0, pPage->NumSpheres());

		sTest.pcPage = &pPage;
		sTest.bDropped = false;
		pWindow->SetContainer(pPage);
		pWindow->Show();

		AssertTrue(sTest.bDropped);
		AssertInt(1, pPage->NumSpheres());

		//SCULPT_PAGE_PICTURE=file saves the window as it now is, to look at.
		if (getenv("SCULPT_PAGE_PICTURE") != NULL)
		{
			SavePNG(pWindow->GetCanvas()->GetImageOrNull(), getenv("SCULPT_PAGE_PICTURE"));
		}
		sSize = pView->GetActualSize();
		AssertInt(TEST_SCULPT_PAGE_WIDTH - SCULPT_PANEL_WIDTH, sSize.x);
		AssertTrue((sSize.y > TEST_SCULPT_PAGE_HEIGHT / 4) && (sSize.y < TEST_SCULPT_PAGE_HEIGHT * 3 / 4));  //Under the two lines of text, over the node editor.
		AssertTrue(pPage->GetNodes()->GetEditor()->GetPosition().y > pView->GetPosition().y + sSize.y);

		pImage = pView->GetDraw()->GetImage();
		AssertTrue(pImage.IsNotNull());
		uiBackground = SculptTestViewPixel(pImage, 0, 0);
		AssertInt(uiBackground, SculptTestViewPixel(pImage, sSize.x - 1, sSize.y - 1));
		AssertTrue(SculptTestViewPixel(pImage, sSize.x / 2, sSize.y / 2) != uiBackground);

		//The sphere was baked coarsely, the whole bake to come; with it done, nothing is.
		pPage->BakeFine();
		AssertFalse(pPage->IsCoarse());
		pPage->BakeFine();
		AssertFalse(pPage->IsCoarse());

		//A click on the view drops another; a press that turns into a drag doesn't.
		sEvent = TestSculptPageEvent(40, 30);
		pView->PressAction(&sEvent);
		pView->ReleaseAction(&sEvent);
		AssertInt(2, pPage->NumSpheres());
		AssertTrue(pPage->IsCoarse());

		pView->PressAction(&sEvent);
		sEvent = TestSculptPageEvent(80, 30);
		pView->MovementAction(&sEvent);
		pView->ReleaseAction(&sEvent);
		AssertInt(2, pPage->NumSpheres());

		pPage->DeleteAction(&sEvent);
		AssertInt(0, pPage->NumSpheres());

		//Another material: its graph is in the editor.
		pPage->SetMaterial(MakeSolidWood(1));
		AssertTrue(pPage->GetNodes()->GetGraph()->GetNode(1)->mpTexture == pPage->GetNodes()->GetGraph()->GetSurface());
		AssertString("Material Output", pPage->GetNodes()->GetEditor()->GetNode(0)->GetTitle());
		AssertTrue(pPage->IsCoarse());

		//Each of the materials there are to start from is a graph of more than its output; one there isn't is none.
		AssertSize(6, NumSculptMaterials());
		AssertString("Marble", GetSculptMaterialName(0));
		AssertNull((void*)GetSculptMaterialName(6));
		AssertTrue(MakeSculptMaterial(6, 1).IsNull());
		for (i = 0; i < NumSculptMaterials(); i++)
		{
			pPage->SetMaterial(MakeSculptMaterial(i, 1));
			AssertTrue(pPage->GetNodes()->GetEditor()->NumNodes() > 2);
			AssertFalse(pPage->GetNodes()->CanUndo());
		}

		//Weathered on a sphere it's a larger graph round the same material; with no shape nothing is.
		pPage->GetPanel()->SetShape(SS_Sphere);
		pPage->PanelChanged();
		pPage->SetMaterial(MakeSculptMaterial(0, 1));
		uiNodes = pPage->GetNodes()->GetEditor()->NumNodes();
		pPage->WeatherMaterial();
		AssertTrue(pPage->GetNodes()->GetEditor()->NumNodes() > uiNodes + 3);
		uiNodes = pPage->GetNodes()->GetEditor()->NumNodes();
		pPage->GetPanel()->SetShape(SS_Sculpted);
		pPage->PanelChanged();
		pPage->WeatherMaterial();
		AssertSize(uiNodes, pPage->GetNodes()->GetEditor()->NumNodes());

		//A setting changed in a node bakes coarsely again.
		pPage->BakeFine();
		pPage->GetNodes()->FieldChanged();
		AssertTrue(pPage->IsCoarse());

		pImage = NULL;
		pView = NULL;
		pPage = NULL;
		sLook.Kill();
		pTheme = NULL;
		pFontColours = NULL;
		SculptTestWindowDone(pWindow, pFactory);
	}
	AssertTrue(ORoot()->RemoveAll());
	ObjectsFlush();
	ObjectsKill();
}


//////////////////////////////////////////////////////////////////////////
//
// The red of pTexture at a point.
//
//////////////////////////////////////////////////////////////////////////
static float32 TestSculptPageRed(Ptr<CSolidTexture> pTexture)
{
	SSolidSample	sSample;
	SFloat32Vec3	sPosition;
	SFloat32Vec3	sNormal;

	sPosition.Init(0.3f, -0.2f, 0.7f);
	sNormal.Init(0, 0, 1);
	pTexture->Sample(&sPosition, &sNormal, 0.01f, &sSample);
	return sSample.sColour.x;
}


//////////////////////////////////////////////////////////////////////////
//
// Never ticked: the window is laid out, not shown.
//
//////////////////////////////////////////////////////////////////////////
static bool TestSculptPageNoTick(Ptr<CWindow> pWindow, int32 iTick, void* pvData)
{
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// Saved and opened, the page is as it was: the material, its nodes
// where they were, and the panel's settings.  The material exported is
// what another program reads, and a file of one opens as a graph.  A
// file that's neither changes nothing.
//
//////////////////////////////////////////////////////////////////////////
void TestSculptPageFile(void)
{
	char		szDirectory[] = "Output" _FS_ "TestSculptPageFile";
	char		szFile[] = "Output" _FS_ "TestSculptPageFile" _FS_ "Mine";
	char		szMaterial[] = "Output" _FS_ "TestSculptPageFile" _FS_ "Rust.DRG";
	CFileUtil	cFileUtil;

	AssertTrue(cFileUtil.RemoveDir(szDirectory));
	AssertTrue(cFileUtil.TouchDir(szDirectory));

	ObjectsInit();
	{
		Ptr<CWinRefWindowFactory>	pFactory;
		Ptr<CWindow>				pWindow;
		Ptr<CFontColours>			pFontColours;
		Ptr<CWindowTheme>			pTheme;
		Ptr<CSculptPage>			pPage;
		Ptr<CSculptGraph>			pGraph;
		Ptr<CSolidTexture>			pRead;
		SDialogLook					sLook;
		SFloat32Vec3				sLight;
		CChars						szMaterials;
		float32						fRed;
		size						uiNodes;
		size						i;

		pWindow = SculptTestWindow(pFactory, 1000, 700, TestSculptPageNoTick, NULL);
		AssertTrue(SculptTestLook(pWindow, &sLook, pFontColours, pTheme));
		pPage = OMalloc<CSculptPage>(pWindow, &sLook, pFontColours, false);
		pWindow->SetContainer(pPage);
		pWindow->Layout();

		//Rust on a box, its height shown, a node moved and a link taken off.
		pPage->SetMaterial(MakeSolidRust(4));
		pGraph = pPage->GetNodes()->GetGraph();
		uiNodes = pGraph->NumNodes();
		pGraph->GetNode(2)->Place(-123, 45);
		pGraph->Unlink(&pGraph->GetNode(1), 0);
		fRed = TestSculptPageRed(pGraph->GetSurface());
		sLight.Init(1, 0, 0);
		pPage->GetPanel()->SetShape(SS_Box);
		pPage->GetPanel()->SetShow(SSH_Height);
		pPage->GetPanel()->SetBake(2, 0.25f, &sLight);
		pPage->PanelChanged();

		AssertTrue(pPage->Save(szFile));
		AssertTrue(pPage->ExportMaterial(szMaterial));

		//SCULPT_PAGE_FILE=file saves it there too, to open in Sculpt.
		if (getenv("SCULPT_PAGE_FILE") != NULL)
		{
			AssertTrue(pPage->Save(getenv("SCULPT_PAGE_FILE")));
		}

		//Something else, then the file opened: it's all back, in other objects.
		pPage->SetMaterial(MakeSolidMarble(1));
		pPage->GetPanel()->SetShape(SS_Sphere);
		pPage->GetPanel()->SetShow(SSH_Baked);
		pPage->PanelChanged();
		AssertTrue(pPage->Open(szFile));
		AssertTrue(pPage->GetNodes()->GetGraph() != pGraph);
		pGraph = pPage->GetNodes()->GetGraph();
		AssertSize(uiNodes, pGraph->NumNodes());
		AssertSize(uiNodes, pPage->GetNodes()->GetEditor()->NumNodes());
		AssertInt(-123, pGraph->GetNode(2)->msPosition.x);
		AssertInt(45, pGraph->GetNode(2)->msPosition.y);
		AssertInt(-123, pPage->GetNodes()->GetEditor()->GetNode(2)->GetNodePosition().x);
		AssertTrue(CSculptGraph::IsValue(pGraph->GetInput(&pGraph->GetNode(1), 0)));
		AssertFloat(fRed, TestSculptPageRed(pGraph->GetSurface()), 6);
		AssertInt(SS_Box, pPage->GetPanel()->GetShape());
		AssertInt(SSH_Height, pPage->GetPanel()->GetShow());
		AssertInt(2, pPage->GetBake()->miSamples);
		AssertFloat3(SFloat32Vec3(1, 0, 0), &pPage->GetBake()->msLight, 6);
		AssertFalse(pPage->GetNodes()->CanUndo());

		//The exported material is what another program reads; opened here it's a graph, and the panel's as it was.
		pRead = ReadSolidMaterial(szMaterial);
		AssertTrue(pRead.IsNotNull());
		AssertFloat(fRed, TestSculptPageRed(pRead), 6);
		pRead = NULL;
		pPage->GetPanel()->SetShape(SS_Sphere);
		AssertTrue(pPage->Open(szMaterial));
		AssertTrue(pPage->GetNodes()->GetGraph()->NumNodes() < uiNodes);  //Only what the surface is made of: not the nodes the link taken off left behind.
		AssertTrue(pPage->GetNodes()->GetGraph()->NumNodes() > 1);
		AssertFloat(fRed, TestSculptPageRed(pPage->GetNodes()->GetGraph()->GetSurface()), 6);
		AssertInt(SS_Sphere, pPage->GetPanel()->GetShape());
		AssertTrue(pPage->GetNodes()->GetGraph()->GetNode(2)->msPosition.x != -123);  //Arranged afresh.

		//SCULPT_MATERIAL_FILES=folder saves a Sculpt file of each material on the sphere and box there, to look at in
		//Sculpt.
		if (getenv("SCULPT_MATERIAL_FILES") != NULL)
		{
			pPage->GetPanel()->SetShape(SS_Both);
			pPage->GetPanel()->SetShow(SSH_Baked);
			sLight.Init(MESH_BAKE_LIGHT_X, MESH_BAKE_LIGHT_Y, MESH_BAKE_LIGHT_Z);
			pPage->GetPanel()->SetBake(MESH_BAKE_SAMPLES, 1.0f, &sLight);
			for (i = 0; i < NumSculptMaterials(); i++)
			{
				szMaterials.Init(getenv("SCULPT_MATERIAL_FILES"));
				cFileUtil.AppendToPath(&szMaterials, GetSculptMaterialName(i));
				pPage->SetMaterial(MakeSculptMaterial(i, 1));
				AssertTrue(pPage->Save(szMaterials.Text()));
				szMaterials.Kill();
			}
		}

		//No such file: nothing changes.
		pGraph = pPage->GetNodes()->GetGraph();
		AssertFalse(pPage->Open("Output" _FS_ "TestSculptPageFile" _FS_ "Nothing"));
		AssertTrue(pPage->GetNodes()->GetGraph() == pGraph);

		pGraph = NULL;
		pPage = NULL;
		sLook.Kill();
		pTheme = NULL;
		pFontColours = NULL;
		SculptTestWindowDone(pWindow, pFactory);
	}
	AssertTrue(ORoot()->RemoveAll());
	ObjectsFlush();
	ObjectsKill();

	AssertTrue(cFileUtil.RemoveDir(szDirectory));
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void TestSculptPage(void)
{
	BeginTests();

	TestSculptPageSpheres();
	TestSculptPageFile();

	TestStatistics();
}

