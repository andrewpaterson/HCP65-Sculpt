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
#include "BaseLib/GlobalDataTypesIO.h"
#include "StandardLib/Objects.h"
#include "ProceduralLib/SolidFlat.h"
#include "ProceduralLib/SolidGradient.h"
#include "ProceduralLib/SolidMaterials.h"
#include "ProceduralLib/SolidMerge.h"
#include "ProceduralLib/SolidNoise.h"
#include "ProceduralLib/SolidRamp.h"
#include "ProceduralLib/SolidWarp.h"
#include "ProceduralLib/SolidCavity.h"
#include "SculptLib/SculptGraph.h"
#include "SculptLib/SolidShow.h"
#include "TestLib/Assert.h"


//////////////////////////////////////////////////////////////////////////
//
// pTexture's red at a point.
//
//////////////////////////////////////////////////////////////////////////
static float32 TestSculptGraphRed(Ptr<CSolidTexture> pTexture)
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
// What a texture and its fields are called, and which of its pointers
// are inputs.
//
//////////////////////////////////////////////////////////////////////////
void TestSculptGraphShow(void)
{
	Ptr<CSolidTexture>	pNoise;
	Ptr<CSolidTexture>	pCavity;
	Ptr<CSolidTexture>	pWarp;
	CChars				szText;

	pNoise = OMalloc<CSolidNoise>(FN_Sum, 4, 7);
	pCavity = OMalloc<CSolidCavity>(Ptr<CShape>(), 0.1f);
	pWarp = OMalloc<CSolidWarp>(pNoise, pCavity, 0.5f);

	szText.Init();
	SolidNodeTitle(&szText, &pNoise);
	AssertString("Noise Texture", szText.Text());
	szText.Clear();
	SolidNodeTitle(&szText, &pWarp);
	AssertString("Warp", szText.Text());
	AssertInt(SNK_Texture, SolidNodeKind(&pNoise));
	AssertInt(SNK_Vector, SolidNodeKind(&pWarp));
	AssertString("Vector", SolidNodeKindName(SNK_Vector));

	//A label is what's said of the field, or its name from its first capital.
	szText.Clear();
	SolidFieldLabel(&szText, &pNoise, "muiOctaves");
	AssertString("Detail", szText.Text());
	szText.Clear();
	SolidFieldLabel(&szText, &pNoise, "muiSeed");
	AssertString("Seed", szText.Text());
	szText.Clear();
	SolidFieldLabel(&szText, &pWarp, "mfAmount");
	AssertString("Amount", szText.Text());
	szText.Kill();

	//Each class's meKind is its own.
	AssertString("Sum|Turbulence|Ridged", FindSolidFieldShow(&pNoise, "meKind")->szNames);
	AssertNull(FindSolidFieldShow(&pWarp, "meKind"));
	AssertInt(SFS_Hidden, FindSolidFieldShow(&pWarp, "msOrigin")->eShow);

	//A noise has no inputs, a warp two, and a cavity's shape isn't one.
	AssertSize(0, SolidNumInputs(&pNoise));
	AssertSize(0, SolidNumInputs(&pCavity));
	AssertSize(2, SolidNumInputs(&pWarp));
	AssertString("mpInput", SolidInputField(&pWarp, 0));
	AssertString("mpWarp", SolidInputField(&pWarp, 1));
	AssertTrue(SolidGetInput(&pWarp, 0) == pNoise);
	AssertTrue(SolidGetInput(&pWarp, 1) == pCavity);
	SolidSetInput(&pWarp, 1, pNoise);
	AssertTrue(SolidGetInput(&pWarp, 1) == pNoise);
	SolidSetInput(&pWarp, 1, Ptr<CSolidTexture>());
	AssertTrue(SolidGetInput(&pWarp, 1).IsNull());
}


//////////////////////////////////////////////////////////////////////////
//
// Marble is a gradient warped by a noise, through a ramp: a node each
// and the output's, in columns by how far each is from the output, and
// it's the texture it was.
//
//////////////////////////////////////////////////////////////////////////
void TestSculptGraphMarble(void)
{
	Ptr<CSolidTexture>	pMarble;
	Ptr<CSculptGraph>	pGraph;
	Ptr<CSculptNode>	pOutput;
	Ptr<CSculptNode>	pRamp;
	Ptr<CSculptNode>	pWarp;
	Ptr<CSculptNode>	pGradient;
	Ptr<CSculptNode>	pNoise;
	CChars				szText;
	float32				fRed;

	pMarble = MakeSolidMarble(3);
	fRed = TestSculptGraphRed(pMarble);
	pGraph = OMalloc<CSculptGraph>(pMarble);
	AssertTrue(pGraph->GetSurface() == pMarble);
	AssertFloat(fRed, TestSculptGraphRed(pGraph->GetSurface()), 6);

	AssertSize(5, pGraph->NumNodes());
	pOutput = pGraph->GetOutput();
	pRamp = pGraph->GetNode(1);
	pWarp = pGraph->GetNode(2);
	pGradient = pGraph->GetNode(3);
	pNoise = pGraph->GetNode(4);
	AssertTrue(pOutput->IsOutput());
	AssertTrue(pRamp->mpTexture == pMarble);
	AssertNotNull(dynamic_cast<CSolidWarp*>(&pWarp->mpTexture));
	AssertNotNull(dynamic_cast<CSolidGradient*>(&pGradient->mpTexture));
	AssertNotNull(dynamic_cast<CSolidNoise*>(&pNoise->mpTexture));
	AssertTrue(pGraph->FindNode(&pWarp->mpTexture) == pWarp);
	AssertTrue(pGraph->FindNode(NULL).IsNull());

	AssertSize(1, pGraph->NumInputs(&pOutput));
	AssertSize(1, pGraph->NumInputs(&pRamp));
	AssertSize(2, pGraph->NumInputs(&pWarp));
	AssertSize(0, pGraph->NumInputs(&pNoise));
	szText.Init();
	pGraph->GetInputLabel(&szText, &pOutput, 0);
	szText.Append(",");
	pGraph->GetInputLabel(&szText, &pRamp, 0);
	szText.Append(",");
	pGraph->GetInputLabel(&szText, &pWarp, 0);
	szText.Append(",");
	pGraph->GetInputLabel(&szText, &pWarp, 1);
	AssertString("Surface,Fac,Input,By", szText.Text());
	szText.Kill();

	AssertTrue(pGraph->GetLinked(&pOutput, 0) == pRamp);
	AssertTrue(pGraph->GetLinked(&pRamp, 0) == pWarp);
	AssertTrue(pGraph->GetLinked(&pWarp, 0) == pGradient);
	AssertTrue(pGraph->GetLinked(&pWarp, 1) == pNoise);

	AssertSize(0, pGraph->GetDepth(&pOutput));
	AssertSize(1, pGraph->GetDepth(&pRamp));
	AssertSize(2, pGraph->GetDepth(&pWarp));
	AssertSize(3, pGraph->GetDepth(&pGradient));
	AssertSize(3, pGraph->GetDepth(&pNoise));

	AssertFalse(pRamp->mbPlaced);
	pRamp->Place(40, -30);
	AssertTrue(pRamp->mbPlaced);
	AssertInt(40, pRamp->msPosition.x);
}


//////////////////////////////////////////////////////////////////////////
//
// A link is an input pointing at a node's texture.  A node can't feed
// what it's made from, and the output feeds nothing.  An input with no
// link has a value of its own, which isn't a node; a node removed leaves
// what it fed with their own values.
//
//////////////////////////////////////////////////////////////////////////
void TestSculptGraphLinks(void)
{
	Ptr<CSculptGraph>	pGraph;
	Ptr<CSculptNode>	pOutput;
	Ptr<CSculptNode>	pRamp;
	Ptr<CSculptNode>	pWarp;
	Ptr<CSculptNode>	pGradient;
	Ptr<CSculptNode>	pNoise;
	Ptr<CSculptNode>	pMerge;
	Ptr<CSolidRamp>		pRampTexture;
	Ptr<CSolidTexture>	pValue;
	SSolidSample		sSample;

	pGraph = OMalloc<CSculptGraph>(MakeSolidMarble(3));
	pOutput = pGraph->GetOutput();
	pRamp = pGraph->GetNode(1);
	pWarp = pGraph->GetNode(2);
	pGradient = pGraph->GetNode(3);
	pNoise = pGraph->GetNode(4);
	pRampTexture = pRamp->mpTexture;

	//Up the graph, to itself and from the output: no.  Down it, and across: yes.
	AssertFalse(pGraph->CanLink(&pRamp, &pWarp, 0));
	AssertFalse(pGraph->CanLink(&pRamp, &pRamp, 0));
	AssertFalse(pGraph->CanLink(&pWarp, &pWarp, 1));
	AssertFalse(pGraph->CanLink(&pOutput, &pRamp, 0));
	AssertFalse(pGraph->CanLink(&pNoise, &pRamp, 1));  //It has one input.
	AssertTrue(pGraph->CanLink(&pNoise, &pRamp, 0));
	AssertTrue(pGraph->CanLink(&pNoise, &pWarp, 0));
	AssertTrue(pGraph->CanLink(&pWarp, &pOutput, 0));
	AssertFalse(pGraph->Link(&pRamp, &pWarp, 0));

	//The noise straight into the ramp: the warp is still a node, and reaches the output no more.
	AssertTrue(pGraph->Link(&pNoise, &pRamp, 0));
	AssertTrue(pRampTexture->mpInput == pNoise->mpTexture);
	AssertTrue(pGraph->GetLinked(&pRamp, 0) == pNoise);
	AssertSize(5, pGraph->NumNodes());
	AssertSize(0, pGraph->GetDepth(&pWarp));
	AssertSize(2, pGraph->GetDepth(&pNoise));

	//Taken off, the ramp's input has a middle grey of its own; taken off again it keeps it.
	pGraph->Unlink(&pRamp, 0);
	AssertTrue(pGraph->GetLinked(&pRamp, 0).IsNull());
	pValue = pGraph->GetInput(&pRamp, 0);
	AssertTrue(CSculptGraph::IsValue(pValue));
	AssertTrue(pRampTexture->mpInput == pValue);
	AssertFloat(0.5f, TestSculptGraphRed(pValue), 6);
	pGraph->Unlink(&pRamp, 0);
	AssertTrue(pGraph->GetInput(&pRamp, 0) == pValue);

	//The warp to the output: the material's surface is the warp.
	AssertTrue(pGraph->Link(&pWarp, &pOutput, 0));
	AssertTrue(pGraph->GetSurface() == pWarp->mpTexture);
	pGraph->Unlink(&pOutput, 0);
	AssertTrue(CSculptGraph::IsValue(pGraph->GetSurface()));

	//The noise removed: the warp's second input has its own value.  The output can't be.
	AssertTrue(pGraph->GetLinked(&pWarp, 1) == pNoise);
	AssertTrue(pGraph->RemoveNode(pNoise));
	AssertSize(4, pGraph->NumNodes());
	AssertTrue(pGraph->GetLinked(&pWarp, 1).IsNull());
	AssertTrue(CSculptGraph::IsValue(pGraph->GetInput(&pWarp, 1)));
	AssertTrue(pGraph->GetLinked(&pWarp, 0) == pGradient);
	AssertFalse(pGraph->RemoveNode(pNoise));
	AssertFalse(pGraph->RemoveNode(pOutput));

	//A new node, with inputs that point at nothing and at one flat: each is given a value of its own.
	sSample.InitValue(0.25f);
	pValue = OMalloc<CSolidFlat>(&sSample);
	pMerge = pGraph->AddNode(OMalloc<CSolidMerge>(SM_Add, pValue, pValue));
	AssertSize(5, pGraph->NumNodes());
	AssertTrue(CSculptGraph::IsValue(pGraph->GetInput(&pMerge, 0)));
	AssertTrue(CSculptGraph::IsValue(pGraph->GetInput(&pMerge, 1)));
	AssertTrue(pGraph->GetInput(&pMerge, 0) != pGraph->GetInput(&pMerge, 1));
	AssertFloat(0.5f, TestSculptGraphRed(pMerge->mpTexture), 6);
	pMerge = pGraph->AddNode(OMalloc<CSolidMerge>(SM_Add, Ptr<CSolidTexture>(), Ptr<CSolidTexture>()));
	AssertTrue(CSculptGraph::IsValue(pGraph->GetInput(&pMerge, 0)));
	AssertFloat(0.0f, TestSculptGraphRed(pMerge->mpTexture), 6);

	//A material of nothing is the output and a black value.
	pGraph = OMalloc<CSculptGraph>(Ptr<CSolidTexture>());
	AssertSize(1, pGraph->NumNodes());
	AssertTrue(CSculptGraph::IsValue(pGraph->GetSurface()));
	AssertFloat(0.0f, TestSculptGraphRed(pGraph->GetSurface()), 6);
}


//////////////////////////////////////////////////////////////////////////
//
// A copy is other textures that are as the graph's and linked as they
// are, its nodes where the graph's are; changing one changes nothing of
// the other.
//
//////////////////////////////////////////////////////////////////////////
void TestSculptGraphCopy(void)
{
	Ptr<CSculptGraph>	pGraph;
	Ptr<CSculptGraph>	pCopy;
	Ptr<CSolidRamp>		pRamp;
	Ptr<CSolidRamp>		pRampCopy;
	Ptr<CSolidWarp>		pWarpCopy;
	float32				fRed;
	size				i;

	pGraph = OMalloc<CSculptGraph>(MakeSolidMarble(3));
	pGraph->GetNode(2)->Place(-200, 35);
	pGraph->Unlink(&pGraph->GetNode(2), 1);  //The warp is by a value of its own.
	fRed = TestSculptGraphRed(pGraph->GetSurface());

	pCopy = pGraph->Copy();
	AssertSize(pGraph->NumNodes(), pCopy->NumNodes());
	AssertFloat(fRed, TestSculptGraphRed(pCopy->GetSurface()), 6);
	AssertTrue(pCopy->GetSurface() != pGraph->GetSurface());
	AssertTrue(pCopy->GetOutput()->IsOutput());
	for (i = 1; i < pGraph->NumNodes(); i++)
	{
		AssertTrue(pCopy->GetNode(i)->mpTexture != pGraph->GetNode(i)->mpTexture);
		AssertString(pGraph->GetNode(i)->mpTexture->GetClass()->GetName(), pCopy->GetNode(i)->mpTexture->GetClass()->GetName());
	}
	AssertInt(-200, pCopy->GetNode(2)->msPosition.x);
	AssertTrue(pCopy->GetNode(2)->mbPlaced);
	AssertFalse(pCopy->GetNode(1)->mbPlaced);

	//Linked as the graph is, to its own textures; values are its own.
	AssertTrue(pCopy->GetLinked(&pCopy->GetOutput(), 0) == pCopy->GetNode(1));
	AssertTrue(pCopy->GetLinked(&pCopy->GetNode(1), 0) == pCopy->GetNode(2));
	AssertTrue(pCopy->GetLinked(&pCopy->GetNode(2), 0) == pCopy->GetNode(3));
	AssertTrue(pCopy->GetLinked(&pCopy->GetNode(2), 1).IsNull());
	AssertTrue(CSculptGraph::IsValue(pCopy->GetInput(&pCopy->GetNode(2), 1)));
	AssertTrue(pCopy->GetInput(&pCopy->GetNode(2), 1) != pGraph->GetInput(&pGraph->GetNode(2), 1));

	//The copy changed: the graph is as it was.
	pRamp = pGraph->GetNode(1)->mpTexture;
	pRampCopy = pCopy->GetNode(1)->mpTexture;
	pWarpCopy = pCopy->GetNode(2)->mpTexture;
	pRampCopy->masKeys.Get(0)->sSample.sColour.x = 0.0f;
	pWarpCopy->mfAmount = 3.0f;
	pCopy->Unlink(&pCopy->GetNode(1), 0);
	AssertFloat(fRed, TestSculptGraphRed(pGraph->GetSurface()), 6);
	AssertTrue(pRamp->masKeys.Get(0)->sSample.sColour.x > 0.5f);
	AssertTrue(pGraph->GetLinked(&pGraph->GetNode(1), 0) == pGraph->GetNode(2));
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void TestSculptGraph(void)
{
	BeginTests();
	ObjectsInit();

	TestSculptGraphShow();
	TestSculptGraphMarble();
	TestSculptGraphLinks();
	TestSculptGraphCopy();

	ObjectsDiscard();
	ObjectsKill(false);
	TestStatistics();
}

