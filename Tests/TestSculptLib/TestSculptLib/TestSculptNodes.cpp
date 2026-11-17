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
#include "StandardLib/Objects.h"
#include "ProceduralLib/SolidMaterials.h"
#include "ProceduralLib/SolidRamp.h"
#include "SculptLib/SculptNodes.h"
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
//
//////////////////////////////////////////////////////////////////////////
void TestSculptNodes(void)
{
	BeginTests();

	TestSculptNodesMarble();

	TestStatistics();
}

