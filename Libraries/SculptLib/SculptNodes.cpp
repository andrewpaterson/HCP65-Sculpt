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
#include "StandardLib/ClassDefines.h"
#include "StandardLib/Objects.h"
#include "SolidShow.h"
#include "SculptNodes.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::Init(Ptr<CNodeEditor> pEditor)
{
	PreInit();

	mpEditor = pEditor;
	mcChanged.Init();
	mpEditor->SetListener(this);

	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::Class(void)
{
	M_Pointer(mpEditor);
	M_Pointer(mpGraph);
	U_Unknown(CComponentEvent<CSculptNodes>, mcChanged);
}


//////////////////////////////////////////////////////////////////////////
//
// The editor may outlive this: it's no longer listened to.
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::Free(void)
{
	if (mpEditor.IsNotNull())
	{
		mpEditor->SetListener(NULL);
	}
	mcChanged.Kill();
}


Ptr<CSculptGraph> CSculptNodes::GetGraph(void) { return mpGraph; }
Ptr<CNodeEditor> CSculptNodes::GetEditor(void) { return mpEditor; }
CComponentEvent<CSculptNodes>* CSculptNodes::GetChangeEvent(void) { return &mcChanged; }
CSculptNode* CSculptNodes::GetNode(CNodeBox* pcBox) { return (CSculptNode*)pcBox->GetData(); }


//////////////////////////////////////////////////////////////////////////
//
// The editor shows pGraph: its nodes as boxes, linked as its textures
// are, those not yet placed arranged, and all of them framed.
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::SetGraph(Ptr<CSculptGraph> pGraph)
{
	size	i;

	mpEditor->RemoveAllNodes();
	mpGraph = pGraph;
	if (mpGraph.IsNull())
	{
		return;
	}
	for (i = 0; i < mpGraph->NumNodes(); i++)
	{
		mpEditor->AddNode(MakeBox(mpGraph->GetNode(i)));
	}
	MakeLinks();
	Arrange();
	mpEditor->FrameAll();
}


//////////////////////////////////////////////////////////////////////////
//
// A node's box: its title and kind's colour, its output (the output
// node has none) and its inputs.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CNodeBox> CSculptNodes::MakeBox(Ptr<CSculptNode> pNode)
{
	Ptr<CNodeBox>	pBox;
	CChars			szText;
	ESolidNodeKind	eKind;
	size			i;

	szText.Init();
	if (pNode->IsOutput())
	{
		eKind = SNK_Output;
		szText.Append("Material Output");
	}
	else
	{
		eKind = SolidNodeKind(&pNode->mpTexture);
		SolidNodeTitle(&szText, &pNode->mpTexture);
	}
	pBox = OMalloc<CNodeBox>(mpEditor->GetWindow(), mpEditor->GetTheme(), szText.Text(), SolidNodeKindColour(eKind));
	pBox->SetData((int64)(size)&pNode);
	pBox->SetNodePosition(pNode->msPosition.x, pNode->msPosition.y);

	if (!pNode->IsOutput())
	{
		pBox->AddOutput("Texture", SOLID_SOCKET_COLOUR);
	}
	for (i = 0; i < mpGraph->NumInputs(&pNode); i++)
	{
		szText.Clear();
		mpGraph->GetInputLabel(&szText, &pNode, i);
		pBox->AddInput(szText.Text(), SOLID_SOCKET_COLOUR);
	}
	szText.Kill();
	return pBox;
}


//////////////////////////////////////////////////////////////////////////
//
// A link for each input that a node feeds.
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::MakeLinks(void)
{
	Ptr<CSculptNode>	pNode;
	Ptr<CSculptNode>	pFrom;
	size				i;
	size				j;

	for (i = 0; i < mpGraph->NumNodes(); i++)
	{
		pNode = mpGraph->GetNode(i);
		for (j = 0; j < mpGraph->NumInputs(&pNode); j++)
		{
			pFrom = mpGraph->GetLinked(&pNode, j);
			if (pFrom.IsNotNull())
			{
				mpEditor->Link(&FindBox(&pFrom), 0, &FindBox(&pNode), j);
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The box of pcNode: NULL if it has none.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CNodeBox> CSculptNodes::FindBox(CSculptNode* pcNode)
{
	Ptr<CNodeBox>	pBox;
	size			i;

	for (i = 0; i < mpEditor->NumNodes(); i++)
	{
		pBox = mpEditor->GetNode(i);
		if (GetNode(&pBox) == pcNode)
		{
			return pBox;
		}
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// The nodes that haven't been placed are put in columns by their depth,
// the output's column at x = 0 and each deeper column to its left, each
// column's nodes one under the other from y = 0 in the graph's order.
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::Arrange(void)
{
	Ptr<CSculptNode>	pNode;
	Ptr<CNodeBox>		pBox;
	SInt32Vec2			sSize;
	size				uiDepth;
	size				uiDeepest;
	size				i;
	int32				y;

	uiDeepest = 0;
	for (i = 0; i < mpGraph->NumNodes(); i++)
	{
		uiDepth = mpGraph->GetDepth(&mpGraph->GetNode(i));
		uiDeepest = (uiDepth > uiDeepest) ? uiDepth : uiDeepest;
	}

	for (uiDepth = 0; uiDepth <= uiDeepest; uiDepth++)
	{
		y = 0;
		for (i = 0; i < mpGraph->NumNodes(); i++)
		{
			pNode = mpGraph->GetNode(i);
			if (!pNode->mbPlaced && (mpGraph->GetDepth(&pNode) == uiDepth))
			{
				pBox = FindBox(&pNode);
				sSize = CContainer::GetMeasuredSize(pBox);
				pNode->Place(-(int32)uiDepth * (NODE_BOX_WIDTH + SCULPT_NODES_COLUMN_GAP), y);
				pBox->SetNodePosition(pNode->msPosition.x, pNode->msPosition.y);
				y += sSize.y + SCULPT_NODES_ROW_GAP;
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CSculptNodes::CanLink(CNodeBox* pcFrom, size uiOutput, CNodeBox* pcTo, size uiInput)
{
	return mpGraph->CanLink(GetNode(pcFrom), GetNode(pcTo), uiInput);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::Linked(CNodeBox* pcFrom, size uiOutput, CNodeBox* pcTo, size uiInput)
{
	mpGraph->Link(GetNode(pcFrom), GetNode(pcTo), uiInput);
	mcChanged.Call(this);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::Unlinked(CNodeBox* pcFrom, size uiOutput, CNodeBox* pcTo, size uiInput)
{
	mpGraph->Unlink(GetNode(pcTo), uiInput);
	mcChanged.Call(this);
}


//////////////////////////////////////////////////////////////////////////
//
// The nodes are where their boxes now are.
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::NodesMoved(void)
{
	Ptr<CNodeBox>	pBox;
	SInt32Vec2		sPosition;
	size			i;

	for (i = 0; i < mpEditor->NumNodes(); i++)
	{
		pBox = mpEditor->GetNode(i);
		sPosition = pBox->GetNodePosition();
		GetNode(&pBox)->Place(sPosition.x, sPosition.y);
	}
}

