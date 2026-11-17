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
#include <string.h>
#include "StandardLib/Objects.h"
#include "ProceduralLib/SolidAdjust.h"
#include "ProceduralLib/SolidBricks.h"
#include "ProceduralLib/SolidFlat.h"
#include "ProceduralLib/SolidMerge.h"
#include "ProceduralLib/SolidNoise.h"
#include "ProceduralLib/SolidSlope.h"
#include "ProceduralLib/SolidWarp.h"
#include "ProceduralLib/SolidMaterials.h"
#include "ProceduralLib/SolidRamp.h"
#include "SculptLib/SculptNodes.h"
#include "SculptLib/SolidShow.h"
#include "SculptTestWindow.h"
#include "TestLib/Assert.h"


//Counts the nodes' changes.
struct STestSculptNodesChanges
{
	int32	iChanges;

	void Changed(CSculptNodes* pcNodes)
	{
		iChanges++;
	}
};


//////////////////////////////////////////////////////////////////////////
//
// Never ticked: the window is laid out, not shown.
//
//////////////////////////////////////////////////////////////////////////
static bool TestSculptNodesTick(Ptr<CWindow> pWindow, int32 iTick, void* pvData)
{
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// The left button pressed at sFrom, moved to sTo and let go, in the
// window.
//
//////////////////////////////////////////////////////////////////////////
static void TestSculptNodesDrag(Ptr<CWindow> pWindow, SInt32Vec2 sFrom, SInt32Vec2 sTo)
{
	pWindow->PointerMoved(sFrom.x, sFrom.y);
	pWindow->InputCommand(CIC_ActivateComponent, true);
	pWindow->PointerMoved(sTo.x, sTo.y);
	pWindow->Layout();
	pWindow->InputCommand(CIC_ExecuteComponent, true);
	pWindow->Layout();
}


//////////////////////////////////////////////////////////////////////////
//
// Marble in a node editor: a box for each node, titled as Blender's are,
// linked as the textures are, arranged in columns with the output on the
// right and framed.  A link dragged in the editor is the graph's, a link
// that would make a texture its own input isn't made, and a node moved
// is where its box is.
//
//////////////////////////////////////////////////////////////////////////
void TestSculptNodesMarble(void)
{
	ObjectsInit();
	{
		Ptr<CWinRefWindowFactory>	pFactory;
		Ptr<CWindow>				pWindow;
		Ptr<CFontColours>			pFontColours;
		Ptr<CWindowTheme>			pTheme;
		Ptr<CNodeEditor>			pEditor;
		Ptr<CSculptNodes>			pNodes;
		Ptr<CSculptGraph>			pGraph;
		Ptr<CNodeBox>				pOutput;
		Ptr<CNodeBox>				pRamp;
		Ptr<CNodeBox>				pWarp;
		Ptr<CNodeBox>				pGradient;
		Ptr<CNodeBox>				pNoise;
		Ptr<CSolidRamp>				pRampTexture;
		Ptr<CSculptValue>			pValue;
		SDialogLook					sLook;
		STestSculptNodesChanges		sChanges;
		SInt32Vec2					sFrom;

		pWindow = SculptTestWindow(pFactory, 1000, 400, TestSculptNodesTick, NULL);
		AssertTrue(SculptTestLook(pWindow, &sLook, pFontColours, pTheme));
		pEditor = OMalloc<CNodeEditor>(pWindow, pTheme);
		pWindow->SetContainer(pEditor);
		pNodes = OMalloc<CSculptNodes>(pEditor);
		sChanges.iChanges = 0;
		pNodes->GetChangeEvent()->Add(&sChanges, &STestSculptNodesChanges::Changed);

		pGraph = OMalloc<CSculptGraph>(MakeSolidMarble(3));
		pNodes->SetGraph(pGraph);
		pWindow->Layout();
		AssertInt(0, sChanges.iChanges);

		AssertSize(5, pEditor->NumNodes());
		AssertSize(4, pEditor->NumLinks());
		pOutput = pEditor->GetNode(0);
		pRamp = pEditor->GetNode(1);
		pWarp = pEditor->GetNode(2);
		pGradient = pEditor->GetNode(3);
		pNoise = pEditor->GetNode(4);
		AssertString("Material Output", pOutput->GetTitle());
		AssertString("Color Ramp", pRamp->GetTitle());
		AssertString("Warp", pWarp->GetTitle());
		AssertString("Gradient Texture", pGradient->GetTitle());
		AssertString("Noise Texture", pNoise->GetTitle());
		AssertTrue(CSculptNodes::GetNode(&pRamp) == &pGraph->GetNode(1));
		AssertTrue(pNodes->FindBox(&pGraph->GetNode(4)) == pNoise);

		//The output has an input and no output; a texture with no inputs has only its output.
		AssertSize(0, pOutput->NumSockets(false));
		AssertSize(1, pOutput->NumSockets(true));
		AssertSize(1, pWarp->NumSockets(false));
		AssertSize(2, pWarp->NumSockets(true));
		AssertSize(0, pNoise->NumSockets(true));
		AssertTrue(pEditor->FindLinkTo(&pWarp, 1)->pcFrom == &pNoise);

		//Columns a box and a gap apart, the output furthest right; the two at one depth one under the other.
		AssertInt(0, pOutput->GetNodePosition().x);
		AssertInt(-(NODE_BOX_WIDTH + SCULPT_NODES_COLUMN_GAP), pRamp->GetNodePosition().x);
		AssertInt(-3 * (NODE_BOX_WIDTH + SCULPT_NODES_COLUMN_GAP), pGradient->GetNodePosition().x);
		AssertInt(pGradient->GetNodePosition().x, pNoise->GetNodePosition().x);
		AssertInt(0, pGradient->GetNodePosition().y);
		AssertInt(pGradient->GetActualSize().y + SCULPT_NODES_ROW_GAP, pNoise->GetNodePosition().y);
		AssertTrue(pGraph->GetNode(4)->mbPlaced);

		//Framed: as far from the left as from the right.
		AssertTrue(pGradient->GetPosition().x > 0);
		AssertInt(pGradient->GetPosition().x, 1000 - (pOutput->GetPosition().x + NODE_BOX_WIDTH));

		//The ramp reads the warp, so its output to the warp's input would make it its own input: not made.
		TestSculptNodesDrag(pWindow, pRamp->GetSocketPosition(false, 0), pWarp->GetSocketPosition(true, 0));
		AssertTrue(pEditor->FindLinkTo(&pWarp, 0)->pcFrom == &pGradient);
		AssertSize(4, pEditor->NumLinks());
		AssertInt(0, sChanges.iChanges);

		//The noise's output dragged to the ramp's input: the ramp reads the noise.
		pRampTexture = pGraph->GetNode(1)->mpTexture;
		TestSculptNodesDrag(pWindow, pNoise->GetSocketPosition(false, 0), pRamp->GetSocketPosition(true, 0));
		AssertTrue(pRampTexture->mpInput == pGraph->GetNode(4)->mpTexture);
		AssertSize(4, pEditor->NumLinks());
		AssertInt(2, sChanges.iChanges);  //The warp's link taken off, and the noise's put on.

		//The ramp's link taken off by its end: its input has its own value.
		TestSculptNodesDrag(pWindow, pRamp->GetSocketPosition(true, 0), SInt32Vec2(500, 380));
		AssertTrue(CSculptGraph::IsValue(pRampTexture->mpInput));
		AssertSize(3, pEditor->NumLinks());
		AssertInt(3, sChanges.iChanges);

		//Its own value is a factor, shown as a number in the input's row: the middle grey an unlinked input starts with.
		pValue = pNodes->FindValue(&pGraph->GetNode(1), 0);
		AssertTrue(pValue->GetColour().IsNull());
		AssertDouble(0.5, pValue->GetNumber()->GetValue(), 6);
		AssertFalse(pRamp->IsInputLinked(0));
		pValue->GetNumber()->SetValue(0.75);
		pValue->NumberChanged(&pValue->GetNumber());
		AssertFloat(0.75f, pValue->GetFlat()->msSample.Value(), 6);
		AssertFloat(0.0f, pValue->GetFlat()->msSample.fHeight, 6);
		AssertInt(4, sChanges.iChanges);
		sChanges.iChanges = 3;

		//The output's surface is a colour; while a node's linked to it, it has no flat texture to show.
		pValue = pNodes->FindValue(&pGraph->GetNode(0), 0);
		AssertTrue(pValue->GetColour().IsNotNull());
		AssertTrue(pValue->GetFlat().IsNull());
		AssertTrue(pOutput->IsInputLinked(0));

		//A node dragged by its title is where its box is.
		sFrom = pWarp->GetPosition();
		sFrom.Init(sFrom.x + NODE_BOX_WIDTH / 2, sFrom.y + pWarp->GetHeaderHeight() / 2);
		TestSculptNodesDrag(pWindow, sFrom, SInt32Vec2(sFrom.x + 25, sFrom.y + 40));
		AssertInt(-2 * (NODE_BOX_WIDTH + SCULPT_NODES_COLUMN_GAP) + 25, pGraph->GetNode(2)->msPosition.x);
		AssertInt(40, pGraph->GetNode(2)->msPosition.y);
		AssertInt(3, sChanges.iChanges);  //Moving a node bakes nothing again.

		//Another graph takes the first's place.
		pNodes->SetGraph(OMalloc<CSculptGraph>(MakeSolidRust(2)));
		pWindow->Layout();
		AssertTrue(pEditor->NumNodes() > 5);
		AssertString("Material Output", pEditor->GetNode(0)->GetTitle());

		pNodes->GetChangeEvent()->Remove(&sChanges);
		pValue = NULL;
		pRampTexture = NULL;
		pOutput = NULL;
		pRamp = NULL;
		pWarp = NULL;
		pGradient = NULL;
		pNoise = NULL;
		pGraph = NULL;
		pNodes = NULL;
		pEditor = NULL;
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
// The settings of a graph's nodes named szName ("mfAmount" say), in the
// nodes' order: the uiIndex'th.
//
//////////////////////////////////////////////////////////////////////////
static Ptr<CSculptField> TestSculptNodesField(Ptr<CSculptNodes> pNodes, const char* szName, size uiIndex)
{
	Ptr<CSculptField>	pField;
	size				i;

	for (i = 0; i < pNodes->NumFields(); i++)
	{
		pField = pNodes->GetField(i);
		if (strcmp(pField->GetFieldName(), szName) == 0)
		{
			if (uiIndex == 0)
			{
				return pField;
			}
			uiIndex--;
		}
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// A node's settings are its texture's fields: a float, an integer, a
// size, a bool, an enum and a vector's three floats each read and set
// as what it is; hidden fields aren't there.  A setting changed by its
// component is put in the texture, the texture is told (a slope's
// direction is a unit again) and the change event called.
//
//////////////////////////////////////////////////////////////////////////
void TestSculptNodesSettings(void)
{
	ObjectsInit();
	{
		Ptr<CWinRefWindowFactory>	pFactory;
		Ptr<CWindow>				pWindow;
		Ptr<CFontColours>			pFontColours;
		Ptr<CWindowTheme>			pTheme;
		Ptr<CNodeEditor>			pEditor;
		Ptr<CSculptNodes>			pNodes;
		Ptr<CSolidAdjust>			pAdjust;
		Ptr<CSolidBricks>			pBricks;
		Ptr<CSolidNoise>			pNoise;
		Ptr<CSolidSlope>			pSlope;
		Ptr<CSculptField>			pField;
		Ptr<CNumberField>			pNumber;
		Ptr<CCheckBox>				pTick;
		Ptr<CComboBox>				pChoice;
		Ptr<CSolidMerge>			pMerge;
		SDialogLook					sLook;
		STestSculptNodesChanges		sChanges;
		Ptr<CSculptValue>			pValue;
		SSolidSample				sSample;
		SFloat32Vec3				sSize;

		pWindow = SculptTestWindow(pFactory, 1000, 600, TestSculptNodesTick, NULL);
		AssertTrue(SculptTestLook(pWindow, &sLook, pFontColours, pTheme));
		pEditor = OMalloc<CNodeEditor>(pWindow, pTheme);
		pWindow->SetContainer(pEditor);
		pNodes = OMalloc<CSculptNodes>(pEditor);
		sChanges.iChanges = 0;
		pNodes->GetChangeEvent()->Add(&sChanges, &STestSculptNodesChanges::Changed);

		//An adjust of a merge of bricks, a noise and... a slope warped by the noise.
		sSize.Init(0.3f, 0.15f, 0.1f);
		pBricks = OMalloc<CSolidBricks>(SB_Brick, &sSize, 0.1f, 0.5f, 9);
		pNoise = OMalloc<CSolidNoise>(FN_Turbulence, 6, 11);
		pSlope = OMalloc<CSolidSlope>();
		pMerge = OMalloc<CSolidMerge>(SM_Add, pBricks, OMalloc<CSolidWarp>(pSlope, pNoise, 0.25f));
		pAdjust = OMalloc<CSolidAdjust>(pMerge);
		pNodes->SetGraph(OMalloc<CSculptGraph>(pAdjust));
		pWindow->Layout();
		AssertSize(7, pEditor->NumNodes());

		//Every texture has a scale; the placement's origin and axes aren't settings.
		AssertTrue(TestSculptNodesField(pNodes, "mfScale", 5).IsNotNull());
		AssertTrue(TestSculptNodesField(pNodes, "mfScale", 6).IsNull());
		AssertTrue(TestSculptNodesField(pNodes, "msOrigin", 0).IsNull());
		AssertTrue(TestSculptNodesField(pNodes, "masAxes", 0).IsNull());

		//An enum is chosen from its names: each class's kind has its own.
		pField = TestSculptNodesField(pNodes, "meKind", 0);
		pChoice = pField->MakeChoice(pWindow, pTheme);
		AssertSize(2, pChoice->NumItems());
		AssertString("Brick", pChoice->GetSelectedText());
		pField = TestSculptNodesField(pNodes, "meKind", 1);
		pChoice = pField->MakeChoice(pWindow, pTheme);
		AssertSize(3, pChoice->NumItems());
		AssertString("Turbulence", pChoice->GetSelectedText());
		pChoice->Select(2);
		pField->Chosen(&pChoice);
		AssertInt(FN_Ridged, pNoise->meKind);
		AssertSize(6, pNoise->muiOctaves);  //Its neighbour is as it was.
		AssertInt(1, sChanges.iChanges);
		sChanges.iChanges = 0;
		pField = TestSculptNodesField(pNodes, "meMerge", 0);
		AssertDouble((float64)SM_Add, pField->Get(), 6);
		pField->Set((float64)SM_Difference);
		AssertInt(SM_Difference, pMerge->meMerge);

		//A float.
		pField = TestSculptNodesField(pNodes, "mfMortar", 0);
		AssertDouble(0.1, pField->Get(), 6);
		pField->Set(0.2);
		AssertFloat(0.2f, pBricks->mfMortar, 6);

		//A 32 bit integer and a size.
		pField = TestSculptNodesField(pNodes, "muiSeed", 0);
		AssertDouble(9.0, pField->Get(), 6);
		pField->Set(123456.0);
		AssertInt(123456, pBricks->muiSeed);
		pField = TestSculptNodesField(pNodes, "muiOctaves", 0);
		AssertDouble(6.0, pField->Get(), 6);
		pField->Set(3.0);
		AssertSize(3, pNoise->muiOctaves);
		AssertInt(11, pNoise->muiSeed);  //Its neighbour is as it was.

		//A bool.
		pField = TestSculptNodesField(pNodes, "mbInvert", 0);
		AssertDouble(0.0, pField->Get(), 6);
		pField->Set(1.0);
		AssertTrue(pAdjust->mbInvert);

		//A vector is three settings.
		AssertTrue(TestSculptNodesField(pNodes, "msSize", 3).IsNull());
		pField = TestSculptNodesField(pNodes, "msSize", 1);
		AssertDouble(0.15, pField->Get(), 6);
		pField->Set(0.5);
		AssertFloat3(SFloat32Vec3(0.3f, 0.5f, 0.1f), &pBricks->msSize, 6);

		//Set, the texture is told its fields changed: a slope's direction is made a unit.
		pField = TestSculptNodesField(pNodes, "msDirection", 2);
		pField->Set(4.0);
		AssertFloat3(SFloat32Vec3(0, 0, 1), &pSlope->msDirection, 6);
		pField = TestSculptNodesField(pNodes, "msDirection", 0);
		pField->Set(1.0);
		AssertFloat3(SFloat32Vec3(0.70710678f, 0, 0.70710678f), &pSlope->msDirection, 5);
		AssertInt(0, sChanges.iChanges);

		//Changed by its component, the texture has the number and the change event is called.  The component is
		//whole for an integer and kept to the range said of the field.
		pField = TestSculptNodesField(pNodes, "mfAmount", 0);
		pNumber = pField->MakeNumber(pWindow, pTheme, "Amount");
		AssertDouble(0.25, pNumber->GetValue(), 6);
		pNumber->SetValue(0.75);
		pField->NumberChanged(&pNumber);
		AssertDouble(0.75, pField->Get(), 6);
		AssertInt(1, sChanges.iChanges);

		pField = TestSculptNodesField(pNodes, "muiOctaves", 0);
		pNumber = pField->MakeNumber(pWindow, pTheme, "Detail");
		pNumber->SetValue(40.4);
		AssertDouble(12.0, pNumber->GetValue(), 6);
		pField->NumberChanged(&pNumber);
		AssertSize(12, pNoise->muiOctaves);
		AssertInt(2, sChanges.iChanges);

		pField = TestSculptNodesField(pNodes, "mbInvert", 0);
		pTick = pField->MakeTick(pWindow, pTheme, "Invert");
		AssertTrue(pTick->IsTicked());
		pTick->SetTicked(false);
		pField->Ticked(&pTick);
		AssertFalse(pAdjust->mbInvert);
		AssertInt(3, sChanges.iChanges);

		//Another graph: the first's settings are gone.
		pNodes->SetGraph(OMalloc<CSculptGraph>(OMalloc<CSolidSlope>()));
		AssertSize(4, pNodes->NumFields());  //A scale and a direction.

		//A merge of two flat textures: each is its input's own colour, changed with a colour field.
		sSample.Init(Set32BitColour((uint8)255, (uint8)128, (uint8)0));
		pMerge = OMalloc<CSolidMerge>(SM_Add, OMalloc<CSolidFlat>(&sSample), Ptr<CSolidTexture>());
		pNodes->SetGraph(OMalloc<CSculptGraph>(pMerge));
		AssertSize(2, pEditor->NumNodes());
		AssertSize(1, pEditor->NumLinks());
		pValue = pNodes->FindValue(&pNodes->GetGraph()->GetNode(1), 0);
		AssertTrue(pValue->GetNumber().IsNull());
		AssertFloat(1.0f, pValue->GetColour()->GetColour()[0], 6);
		AssertFloat(128.0f / 255.0f, pValue->GetColour()->GetColour()[1], 6);
		sChanges.iChanges = 0;
		pValue->GetColour()->SetColour(0.25f, 0.5f, 0.75f);
		pValue->ColourChanged(&pValue->GetColour());
		AssertTrue(pMerge->mpFirst == pValue->GetFlat());
		AssertFloat(0.25f, pValue->GetFlat()->msSample.sColour.x, 6);
		AssertFloat(0.75f, pValue->GetFlat()->msSample.sColour.z, 6);
		AssertFloat(1.0f, pValue->GetFlat()->msSample.sColour.w, 6);
		AssertInt(1, sChanges.iChanges);
		pValue = pNodes->FindValue(&pNodes->GetGraph()->GetNode(1), 1);
		AssertFloat(0.0f, pValue->GetColour()->GetColour()[0], 6);  //Nothing was linked: black.
		pValue = NULL;

		pNodes->GetChangeEvent()->Remove(&sChanges);
		pChoice = NULL;
		pMerge = NULL;
		pTick = NULL;
		pNumber = NULL;
		pField = NULL;
		pSlope = NULL;
		pNoise = NULL;
		pBricks = NULL;
		pAdjust = NULL;
		pNodes = NULL;
		pEditor = NULL;
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
// A ramp's node has a bar of its keys.  A stop moved, recoloured, added
// or removed is the ramp's key: moved past another its key is too, and
// keeps its height and shine; a stop added is the ramp's sample there;
// the chosen key's other numbers are changed under the bar.
//
//////////////////////////////////////////////////////////////////////////
void TestSculptNodesRamp(void)
{
	ObjectsInit();
	{
		Ptr<CWinRefWindowFactory>	pFactory;
		Ptr<CWindow>				pWindow;
		Ptr<CFontColours>			pFontColours;
		Ptr<CWindowTheme>			pTheme;
		Ptr<CNodeEditor>			pEditor;
		Ptr<CSculptNodes>			pNodes;
		Ptr<CSolidRamp>				pRamp;
		Ptr<CSculptRamp>			pKeys;
		Ptr<CColourRampBar>			pBar;
		SDialogLook					sLook;
		STestSculptNodesChanges		sChanges;
		SSolidSample				sSample;

		pWindow = SculptTestWindow(pFactory, 800, 600, TestSculptNodesTick, NULL);
		AssertTrue(SculptTestLook(pWindow, &sLook, pFontColours, pTheme));
		pEditor = OMalloc<CNodeEditor>(pWindow, pTheme);
		pWindow->SetContainer(pEditor);
		pNodes = OMalloc<CSculptNodes>(pEditor);
		sChanges.iChanges = 0;
		pNodes->GetChangeEvent()->Add(&sChanges, &STestSculptNodesChanges::Changed);

		//Black and flat at 0, red and raised at 0.5, white and shiny at 1.
		pRamp = OMalloc<CSolidRamp>(Ptr<CSolidTexture>());
		sSample.Init();
		pRamp->AddKey(0.0f, &sSample);
		sSample.Init(Set32BitColour((uint8)255, (uint8)0, (uint8)0));
		sSample.fHeight = 0.2f;
		pRamp->AddKey(0.5f, &sSample);
		sSample.Init(Set32BitColour((uint8)255, (uint8)255, (uint8)255));
		sSample.fShine = 0.8f;
		pRamp->AddKey(1.0f, &sSample);
		pNodes->SetGraph(OMalloc<CSculptGraph>(pRamp));
		pWindow->Layout();

		AssertTrue(pNodes->FindRamp(NULL).IsNull());
		pKeys = pNodes->FindRamp(&pRamp);
		pBar = pKeys->GetBar();
		AssertSize(3, pBar->NumStops());
		AssertFloat(0.5f, pBar->GetStop(1)->fPlace, 6);
		AssertFloat(1.0f, pBar->GetStop(1)->afColour[0], 6);
		AssertFloat(0.0f, pBar->GetStop(1)->afColour[1], 6);
		AssertInt(0, pBar->GetChosen());
		AssertDouble(0.0, pKeys->GetExtra(0)->GetValue(), 6);

		//The red key chosen: its height shows.  Changed, it's the key's.
		pBar->Choose(1);
		AssertDouble(0.2, pKeys->GetExtra(0)->GetValue(), 6);
		pKeys->GetExtra(0)->SetValue(0.3);
		pKeys->ExtraChanged(&pKeys->GetExtra(0));
		AssertFloat(0.3f, pRamp->masKeys.Get(1)->sSample.fHeight, 6);
		pKeys->GetExtra(3)->SetValue(0.6);
		pKeys->ExtraChanged(&pKeys->GetExtra(3));
		AssertFloat(0.6f, pRamp->masKeys.Get(1)->sSample.fGlow, 6);
		AssertInt(2, sChanges.iChanges);

		//Recoloured green, it keeps its height.
		pBar->GetColour()->SetColour(0.0f, 1.0f, 0.0f);
		pBar->ColourChanged(&pBar->GetColour());
		AssertFloat(0.0f, pRamp->masKeys.Get(1)->sSample.sColour.x, 6);
		AssertFloat(1.0f, pRamp->masKeys.Get(1)->sSample.sColour.y, 6);
		AssertFloat(0.3f, pRamp->masKeys.Get(1)->sSample.fHeight, 6);
		AssertInt(3, sChanges.iChanges);

		//Moved to 0.25 it's still the second key; moved past the white one it's the last, with its height.
		pBar->GetPlace()->SetValue(0.25);
		pBar->PlaceChanged(&pBar->GetPlace());
		AssertFloat(0.25f, pRamp->masKeys.Get(1)->fValue, 6);
		pRamp->masKeys.Get(2)->fValue = 0.75f;  //The white key, by hand,
		pNodes->SetGraph(OMalloc<CSculptGraph>(pRamp));  //and its node made again.
		pKeys = pNodes->FindRamp(&pRamp);
		pBar = pKeys->GetBar();
		pBar->Choose(1);
		sChanges.iChanges = 0;
		pBar->GetPlace()->SetValue(0.9);
		pBar->PlaceChanged(&pBar->GetPlace());
		AssertSize(3, pRamp->masKeys.NumElements());
		AssertFloat(0.75f, pRamp->masKeys.Get(1)->fValue, 6);
		AssertFloat(0.8f, pRamp->masKeys.Get(1)->sSample.fShine, 6);
		AssertFloat(0.9f, pRamp->masKeys.Get(2)->fValue, 6);
		AssertFloat(0.3f, pRamp->masKeys.Get(2)->sSample.fHeight, 6);
		AssertFloat(1.0f, pRamp->masKeys.Get(2)->sSample.sColour.y, 6);
		AssertInt(2, pBar->GetChosen());
		AssertDouble(0.3, pKeys->GetExtra(0)->GetValue(), 6);
		AssertInt(1, sChanges.iChanges);

		//A stop added between the white key and the green: half way, the ramp's sample there, height and all.
		pBar->Choose(1);
		pBar->AddClicked(NULL);
		AssertSize(4, pRamp->masKeys.NumElements());
		AssertFloat(0.825f, pRamp->masKeys.Get(2)->fValue, 5);
		AssertFloat(0.15f, pRamp->masKeys.Get(2)->sSample.fHeight, 5);
		AssertFloat(0.4f, pRamp->masKeys.Get(2)->sSample.fShine, 5);
		AssertFloat(0.5f, pRamp->masKeys.Get(2)->sSample.sColour.x, 5);
		AssertDouble(0.15, pKeys->GetExtra(0)->GetValue(), 5);

		//Removed again, the keys either side are as they were.
		pBar->RemoveClicked(NULL);
		AssertSize(3, pRamp->masKeys.NumElements());
		AssertFloat(0.75f, pRamp->masKeys.Get(1)->fValue, 6);
		AssertFloat(0.9f, pRamp->masKeys.Get(2)->fValue, 6);
		AssertFloat(0.3f, pRamp->masKeys.Get(2)->sSample.fHeight, 6);
		AssertInt(3, sChanges.iChanges);

		pNodes->GetChangeEvent()->Remove(&sChanges);
		pBar = NULL;
		pKeys = NULL;
		pRamp = NULL;
		pNodes = NULL;
		pEditor = NULL;
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
// The class of SolidShow's table titled szTitle.
//
//////////////////////////////////////////////////////////////////////////
static size TestSculptNodesShow(const char* szTitle)
{
	size	i;

	for (i = 0; i < NumSolidNodeShows(); i++)
	{
		if (strcmp(GetSolidNodeShow(i)->szTitle, szTitle) == 0)
		{
			return i;
		}
	}
	return NumSolidNodeShows();
}


//////////////////////////////////////////////////////////////////////////
//
// A node added is a new texture linked to nothing, selected, in the
// middle of the editor, and bakes nothing until it's linked.  Selected
// nodes are copied a little right and down, the copy with the original's
// settings, inputs and keys, and its own value.  Deleted, what they fed
// has its own value again; the output stays.
//
//////////////////////////////////////////////////////////////////////////
void TestSculptNodesAddCopyRemove(void)
{
	ObjectsInit();
	{
		Ptr<CWinRefWindowFactory>	pFactory;
		Ptr<CWindow>				pWindow;
		Ptr<CFontColours>			pFontColours;
		Ptr<CWindowTheme>			pTheme;
		Ptr<CNodeEditor>			pEditor;
		Ptr<CSculptNodes>			pNodes;
		Ptr<CSculptGraph>			pGraph;
		Ptr<CSculptNode>			pNode;
		Ptr<CSculptNode>			pCopy;
		Ptr<CSolidNoise>			pNoise;
		Ptr<CSolidRamp>				pRamp;
		Ptr<CSolidRamp>				pRampCopy;
		Ptr<CSolidWarp>				pWarp;
		Ptr<CMenuContainer>			pMenu;
		SDialogLook					sLook;
		STestSculptNodesChanges		sChanges;
		SInt32Vec2					sPan;

		pWindow = SculptTestWindow(pFactory, 1000, 600, TestSculptNodesTick, NULL);
		AssertTrue(SculptTestLook(pWindow, &sLook, pFontColours, pTheme));
		pEditor = OMalloc<CNodeEditor>(pWindow, pTheme);
		pWindow->SetContainer(pEditor);
		pNodes = OMalloc<CSculptNodes>(pEditor);
		sChanges.iChanges = 0;
		pNodes->GetChangeEvent()->Add(&sChanges, &STestSculptNodesChanges::Changed);
		pGraph = OMalloc<CSculptGraph>(MakeSolidMarble(3));
		pNodes->SetGraph(pGraph);
		pWindow->Layout();
		sPan = pEditor->GetPan();

		//The Add menu: the four kinds, each with its classes under it.
		pMenu = pNodes->MakeAddMenu();
		AssertSize(4, pMenu->NumItems());
		pMenu = NULL;

		//A noise added: in the middle of the editor, selected alone, linked to nothing, nothing baked, the view unmoved.
		pNode = pNodes->AddNodeOf(TestSculptNodesShow("Noise Texture"));
		pWindow->Layout();
		AssertSize(6, pGraph->NumNodes());
		AssertSize(6, pEditor->NumNodes());
		AssertSize(4, pEditor->NumLinks());
		pNoise = pNode->mpTexture;
		AssertTrue(pNoise.IsNotNull());
		AssertSize(4, pNoise->muiOctaves);
		AssertTrue(pNode->mbPlaced);
		AssertInt(500, pNodes->FindBox(&pNode)->GetPosition().x + NODE_BOX_WIDTH / 2);
		AssertInt(300, pNodes->FindBox(&pNode)->GetPosition().y);
		AssertTrue(pNodes->FindBox(&pNode)->IsSelected());
		AssertSize(1, pEditor->NumSelected());
		AssertInt(0, sChanges.iChanges);
		AssertInt(sPan.x, pEditor->GetPan().x);
		AssertSize(0, pGraph->GetDepth(&pNode));

		//A class that isn't made new, and one there isn't, add nothing.
		AssertTrue(pNodes->AddNodeOf(TestSculptNodesShow("Cavity")).IsNull());
		AssertTrue(pNodes->AddNodeOf(NumSolidNodeShows()).IsNull());
		AssertSize(6, pGraph->NumNodes());

		//Shift+A at a place: what's then added goes there.
		pNodes->AddNode(-40, 25);
		pWindow->CloseAllPopups();
		pNode = pNodes->AddNodeOf(TestSculptNodesShow("Mix"));
		AssertInt(-40, pNode->msPosition.x);
		AssertInt(25, pNode->msPosition.y);
		AssertSize(2, pGraph->NumInputs(&pNode));
		AssertTrue(CSculptGraph::IsValue(pGraph->GetInput(&pNode, 0)));
		AssertSize(7, pEditor->NumNodes());

		//The ramp and the warp copied: each a new texture as the old, fed by what fed it; the copies are the selected ones.
		pRamp = pGraph->GetNode(1)->mpTexture;
		pWarp = pGraph->GetNode(2)->mpTexture;
		pEditor->SelectNone();
		pEditor->Select(pNodes->FindBox(&pGraph->GetNode(1)), true);
		pEditor->Select(pNodes->FindBox(&pGraph->GetNode(2)), true);
		pEditor->Select(pEditor->GetNode(0), true);  //The output, which isn't copied.
		pNodes->DuplicateSelected();
		pWindow->Layout();
		AssertSize(9, pGraph->NumNodes());
		AssertSize(2, pEditor->NumSelected());
		pCopy = pGraph->GetNode(7);
		pRampCopy = pCopy->mpTexture;
		AssertTrue(pRampCopy.IsNotNull());
		AssertTrue(pRampCopy != pRamp);
		AssertTrue(pRampCopy->mpInput == pRamp->mpInput);
		AssertSize(pRamp->masKeys.NumElements(), pRampCopy->masKeys.NumElements());
		AssertTrue(pRampCopy->masKeys.GetData() != pRamp->masKeys.GetData());
		AssertFloat(pRamp->masKeys.Get(2)->sSample.sColour.x, pRampCopy->masKeys.Get(2)->sSample.sColour.x, 6);
		AssertInt(pGraph->GetNode(1)->msPosition.x + SCULPT_NODES_COPY_OFFSET, pCopy->msPosition.x);
		AssertInt(pGraph->GetNode(1)->msPosition.y + SCULPT_NODES_COPY_OFFSET, pCopy->msPosition.y);
		AssertTrue(pNodes->FindBox(&pCopy)->IsSelected());
		AssertFloat(pWarp->mfAmount, Ptr<CSolidWarp>(pGraph->GetNode(8)->mpTexture)->mfAmount, 6);
		AssertTrue(Ptr<CSolidWarp>(pGraph->GetNode(8)->mpTexture)->mpWarp == pWarp->mpWarp);
		AssertInt(0, sChanges.iChanges);

		//The copy's keys are its own.
		pRampCopy->masKeys.Get(2)->sSample.sColour.x = 0.123f;
		AssertTrue(pRamp->masKeys.Get(2)->sSample.sColour.x != 0.123f);

		//The warp deleted (with the output selected too, which stays): the ramp it fed has its own value, and it's baked.
		pEditor->SelectNone();
		pEditor->Select(pNodes->FindBox(&pGraph->GetNode(2)), true);
		pEditor->Select(pEditor->GetNode(0), true);
		pNodes->DeleteSelected();
		pWindow->Layout();
		AssertSize(8, pGraph->NumNodes());
		AssertSize(8, pEditor->NumNodes());
		AssertTrue(pGraph->GetNode(0)->IsOutput());
		AssertTrue(CSculptGraph::IsValue(pRamp->mpInput));
		AssertTrue(CSculptGraph::IsValue(pRampCopy->mpInput));
		AssertTrue(pRamp->mpInput != pRampCopy->mpInput);
		AssertInt(1, sChanges.iChanges);
		AssertDouble(0.5, pNodes->FindValue(&pGraph->GetNode(1), 0)->GetNumber()->GetValue(), 6);

		//A Combine added: a colour and four numbers, each its own value.
		pNode = pNodes->AddNodeOf(TestSculptNodesShow("Combine"));
		AssertSize(5, pGraph->NumInputs(&pNode));
		AssertTrue(pNodes->FindValue(&pNode, 0)->GetColour().IsNotNull());
		AssertTrue(pNodes->FindValue(&pNode, 1)->GetNumber().IsNotNull());
		AssertTrue(pNodes->FindValue(&pNode, 4)->GetNumber().IsNotNull());
		AssertString("Combine", pNodes->FindBox(&pNode)->GetTitle());
		pNode = pNodes->AddNodeOf(TestSculptNodesShow("Separate"));
		AssertSize(1, pGraph->NumInputs(&pNode));
		AssertSize(10, pGraph->NumNodes());
		pEditor->SelectNone();
		pEditor->Select(pNodes->FindBox(&pNode), true);
		pEditor->Select(pNodes->FindBox(&pGraph->GetNode(8)), true);
		pNodes->DeleteSelected();
		AssertSize(8, pGraph->NumNodes());

		//Nothing selected, nothing's deleted or copied.
		pEditor->SelectNone();
		pNodes->DeleteSelected();
		pNodes->DuplicateSelected();
		AssertSize(8, pGraph->NumNodes());
		AssertInt(2, sChanges.iChanges);  //The two deletions.

		pNodes->GetChangeEvent()->Remove(&sChanges);
		pWarp = NULL;
		pRampCopy = NULL;
		pRamp = NULL;
		pNoise = NULL;
		pCopy = NULL;
		pNode = NULL;
		pGraph = NULL;
		pNodes = NULL;
		pEditor = NULL;
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
void TestSculptNodes(void)
{
	BeginTests();

	TestSculptNodesMarble();
	TestSculptNodesSettings();
	TestSculptNodesRamp();
	TestSculptNodesAddCopyRemove();

	TestStatistics();
}

