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
#include "StandardLib/Objects.h"
#include "SupportLib/PNGWriter.h"
#include "WindowLib/Canvas.h"
#include "ProceduralLib/SolidMaterials.h"
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
		SDialogLook					sLook;
		STestSculptPage				sTest;
		SComponentInputEvent		sEvent;
		SInt32Vec2					sSize;
		uint32						uiBackground;

		pWindow = SculptTestWindow(pFactory, TEST_SCULPT_PAGE_WIDTH, TEST_SCULPT_PAGE_HEIGHT, TestSculptPageTick, &sTest);
		AssertTrue(SculptTestLook(pWindow, &sLook, pFontColours, pTheme));

		pPage = OMalloc<CSculptPage>(pWindow, &sLook, pFontColours, false);
		pView = pPage->GetView();
		AssertInt(0, pPage->NumSpheres());

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
		AssertInt(TEST_SCULPT_PAGE_WIDTH, sSize.x);
		AssertTrue((sSize.y > TEST_SCULPT_PAGE_HEIGHT / 4) && (sSize.y < TEST_SCULPT_PAGE_HEIGHT * 3 / 4));  //Under the two lines of text, over the node editor.
		AssertTrue(pPage->GetNodes()->GetEditor()->GetPosition().y > pView->GetPosition().y + sSize.y);

		pImage = pView->GetDraw()->GetImage();
		AssertTrue(pImage.IsNotNull());
		uiBackground = SculptTestViewPixel(pImage, 0, 0);
		AssertInt(uiBackground, SculptTestViewPixel(pImage, sSize.x - 1, sSize.y - 1));
		AssertTrue(SculptTestViewPixel(pImage, sSize.x / 2, sSize.y / 2) != uiBackground);

		//A click on the view drops another; a press that turns into a drag doesn't.
		sEvent = TestSculptPageEvent(40, 30);
		pView->PressAction(&sEvent);
		pView->ReleaseAction(&sEvent);
		AssertInt(2, pPage->NumSpheres());

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
//
//////////////////////////////////////////////////////////////////////////
void TestSculptPage(void)
{
	BeginTests();

	TestSculptPageSpheres();

	TestStatistics();
}

