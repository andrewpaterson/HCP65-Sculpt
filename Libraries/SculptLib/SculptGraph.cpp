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
#include "StandardLib/Class.h"
#include "StandardLib/ClassDefines.h"
#include "StandardLib/Objects.h"
#include "StandardLib/PointerField.h"
#include "ProceduralLib/SolidFlat.h"
#include "SolidShow.h"
#include "SculptGraph.h"


//////////////////////////////////////////////////////////////////////////
//
// The uiInput'th of the texture's pointers that isn't hidden: NULL if it
// has fewer.
//
//////////////////////////////////////////////////////////////////////////
static CPointerField* SolidInputPointerField(CSolidTexture* pcTexture, size uiInput)
{
	CArrayVoidPtr*		papcFields;
	CPointerField*		pcField;
	SSolidFieldShow*	psShow;
	size				i;

	papcFields = pcTexture->GetClass()->GetPointerFields();
	for (i = 0; i < papcFields->NumElements(); i++)
	{
		pcField = (CPointerField*)papcFields->GetPtr(i);
		psShow = FindSolidFieldShow(pcTexture, pcField->GetName());
		if ((psShow == NULL) || (psShow->eShow != SFS_Hidden))
		{
			if (uiInput == 0)
			{
				return pcField;
			}
			uiInput--;
		}
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
size SolidNumInputs(CSolidTexture* pcTexture)
{
	size	uiCount;

	uiCount = 0;
	while (SolidInputPointerField(pcTexture, uiCount) != NULL)
	{
		uiCount++;
	}
	return uiCount;
}


//////////////////////////////////////////////////////////////////////////
//
// The name of the field the input is, "mpInput" say.
//
//////////////////////////////////////////////////////////////////////////
const char* SolidInputField(CSolidTexture* pcTexture, size uiInput)
{
	return SolidInputPointerField(pcTexture, uiInput)->GetName();
}


//////////////////////////////////////////////////////////////////////////
//
// What the input points at: NULL for nothing, or something that isn't a
// solid texture.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSolidTexture> SolidGetInput(CSolidTexture* pcTexture, size uiInput)
{
	CPointer*	pcPointer;

	pcPointer = SolidInputPointerField(pcTexture, uiInput)->GetPointer(pcTexture);
	return Ptr<CSolidTexture>(dynamic_cast<CSolidTexture*>(pcPointer->Object()));
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void SolidSetInput(CSolidTexture* pcTexture, size uiInput, Ptr<CSolidTexture> pInput)
{
	CPointer*	pcPointer;

	pcPointer = SolidInputPointerField(pcTexture, uiInput)->GetPointer(pcTexture);
	*pcPointer = pInput;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptNode::Init(Ptr<CSolidTexture> pTexture)
{
	PreInit();

	mpTexture = pTexture;
	msPosition.Init(0, 0);
	mbPlaced = false;

	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptNode::Class(void)
{
	M_Pointer(mpTexture);
	U_2Int32(msPosition);
	U_Bool(mbPlaced);
}


void CSculptNode::Free(void) {}
bool CSculptNode::IsOutput(void) { return mpTexture.IsNull(); }
void CSculptNode::Place(int32 x, int32 y) { msPosition.Init(x, y); mbPlaced = true; }


//////////////////////////////////////////////////////////////////////////
//
// The graph of the material pSurface is the top of: a node for the
// output and one for each texture it's made of that isn't a value.
// NULL is a material of nothing: the opaque black a bake makes of none.
//
//////////////////////////////////////////////////////////////////////////
void CSculptGraph::Init(Ptr<CSolidTexture> pSurface)
{
	SSolidSample	sBlack;

	PreInit();

	mapNodes.Init();
	mapNodes.Add(OMalloc<CSculptNode>(Ptr<CSolidTexture>()));
	mpSurface = pSurface;
	if (mpSurface.IsNull())
	{
		sBlack.Init();
		mpSurface = OMalloc<CSolidFlat>(&sBlack);
	}
	AddNodesOf(mpSurface);

	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptGraph::Class(void)
{
	M_Pointer(mpSurface);
	M_Embedded(mapNodes);
}


void CSculptGraph::Free(void) {}
Ptr<CSolidTexture> CSculptGraph::GetSurface(void) { return mpSurface; }
size CSculptGraph::NumNodes(void) { return mapNodes.NumElements(); }
Ptr<CSculptNode> CSculptGraph::GetNode(size uiIndex) { return mapNodes.Get(uiIndex); }
Ptr<CSculptNode> CSculptGraph::GetOutput(void) { return mapNodes.Get(0); }


//////////////////////////////////////////////////////////////////////////
//
// A flat texture is an input's value, not a node.
//
//////////////////////////////////////////////////////////////////////////
bool CSculptGraph::IsValue(Ptr<CSolidTexture> pTexture)
{
	return dynamic_cast<CSolidFlat*>(&pTexture) != NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// The node of pcTexture: NULL if it has none.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSculptNode> CSculptGraph::FindNode(CSolidTexture* pcTexture)
{
	Ptr<CSculptNode>	pNode;
	size				i;

	for (i = 1; i < mapNodes.NumElements(); i++)
	{
		pNode = mapNodes.Get(i);
		if (&pNode->mpTexture == pcTexture)
		{
			return pNode;
		}
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// A node for pTexture and for what it's made of, each once however many
// inputs it feeds.
//
//////////////////////////////////////////////////////////////////////////
void CSculptGraph::AddNodesOf(Ptr<CSolidTexture> pTexture)
{
	Ptr<CSculptNode>	pNode;
	Ptr<CSolidTexture>	pInput;
	size				i;

	if (IsValue(pTexture) || FindNode(&pTexture).IsNotNull())
	{
		return;
	}
	pNode = OMalloc<CSculptNode>(pTexture);
	mapNodes.Add(pNode);
	GiveInputsValues(&pNode);
	for (i = 0; i < SolidNumInputs(&pTexture); i++)
	{
		pInput = SolidGetInput(&pTexture, i);
		AddNodesOf(pInput);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Each of the node's inputs that points at nothing is given a value of
// opaque black, which is what an operator makes of nothing; each that
// points at a value another input points at too is given a copy.
//
//////////////////////////////////////////////////////////////////////////
void CSculptGraph::GiveInputsValues(CSculptNode* pcNode)
{
	Ptr<CSolidTexture>	pInput;
	Ptr<CSolidFlat>		pFlat;
	SSolidSample		sSample;
	size				i;

	for (i = 0; i < SolidNumInputs(&pcNode->mpTexture); i++)
	{
		pInput = SolidGetInput(&pcNode->mpTexture, i);
		if (pInput.IsNull())
		{
			sSample.Init();
			SolidSetInput(&pcNode->mpTexture, i, OMalloc<CSolidFlat>(&sSample));
		}
		else if (IsValue(pInput) && (pInput->NumHeapFroms() > 1))
		{
			pFlat = pInput;
			SolidSetInput(&pcNode->mpTexture, i, OMalloc<CSolidFlat>(&pFlat->msSample));
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The output has one input: the material's surface.
//
//////////////////////////////////////////////////////////////////////////
size CSculptGraph::NumInputs(CSculptNode* pcNode)
{
	return pcNode->IsOutput() ? 1 : SolidNumInputs(&pcNode->mpTexture);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptGraph::GetInputLabel(CChars* pszDest, CSculptNode* pcNode, size uiInput)
{
	if (pcNode->IsOutput())
	{
		pszDest->Append("Surface");
	}
	else
	{
		SolidFieldLabel(pszDest, &pcNode->mpTexture, SolidInputField(&pcNode->mpTexture, uiInput));
	}
}


//////////////////////////////////////////////////////////////////////////
//
// What the input points at: a node's texture, or its own value.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSolidTexture> CSculptGraph::GetInput(CSculptNode* pcNode, size uiInput)
{
	return pcNode->IsOutput() ? mpSurface : SolidGetInput(&pcNode->mpTexture, uiInput);
}


//////////////////////////////////////////////////////////////////////////
//
// The node linked to the input: NULL if it has its own value.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSculptNode> CSculptGraph::GetLinked(CSculptNode* pcNode, size uiInput)
{
	return FindNode(&GetInput(pcNode, uiInput));
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptGraph::SetInput(CSculptNode* pcNode, size uiInput, Ptr<CSolidTexture> pTexture)
{
	if (pcNode->IsOutput())
	{
		mpSurface = pTexture;
	}
	else
	{
		SolidSetInput(&pcNode->mpTexture, uiInput, pTexture);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// True if pcTexture is pcOn, or is made from it however far down.
//
//////////////////////////////////////////////////////////////////////////
bool CSculptGraph::DependsOn(CSolidTexture* pcTexture, CSolidTexture* pcOn)
{
	Ptr<CSolidTexture>	pInput;
	size				i;

	if (pcTexture == pcOn)
	{
		return true;
	}
	for (i = 0; i < SolidNumInputs(pcTexture); i++)
	{
		pInput = SolidGetInput(pcTexture, i);
		if (pInput.IsNotNull() && DependsOn(&pInput, pcOn))
		{
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// The output feeds nothing, and a node can't feed what it's made from.
//
//////////////////////////////////////////////////////////////////////////
bool CSculptGraph::CanLink(CSculptNode* pcFrom, CSculptNode* pcTo, size uiInput)
{
	if (pcFrom->IsOutput() || (uiInput >= NumInputs(pcTo)))
	{
		return false;
	}
	return pcTo->IsOutput() || !DependsOn(&pcFrom->mpTexture, &pcTo->mpTexture);
}


//////////////////////////////////////////////////////////////////////////
//
// The input points at pcFrom's texture.  False if it can't.
//
//////////////////////////////////////////////////////////////////////////
bool CSculptGraph::Link(CSculptNode* pcFrom, CSculptNode* pcTo, size uiInput)
{
	if (!CanLink(pcFrom, pcTo, uiInput))
	{
		return false;
	}
	SetInput(pcTo, uiInput, pcFrom->mpTexture);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The input has its own value again: a middle grey, if it was linked.
//
//////////////////////////////////////////////////////////////////////////
void CSculptGraph::Unlink(CSculptNode* pcTo, size uiInput)
{
	SSolidSample	sGrey;

	if (!IsValue(GetInput(pcTo, uiInput)))
	{
		sGrey.InitValue(0.5f);
		sGrey.fHeight = 0.0f;
		SetInput(pcTo, uiInput, OMalloc<CSolidFlat>(&sGrey));
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A node for a texture that's linked to nothing yet.  Its inputs are
// given values, and what they already point at nodes.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSculptNode> CSculptGraph::AddNode(Ptr<CSolidTexture> pTexture)
{
	AddNodesOf(pTexture);
	return FindNode(&pTexture);
}


//////////////////////////////////////////////////////////////////////////
//
// The node goes, and every input it fed has its own value again.  False
// for the output, which stays, and for a node that isn't the graph's.
//
//////////////////////////////////////////////////////////////////////////
bool CSculptGraph::RemoveNode(Ptr<CSculptNode> pNode)
{
	Ptr<CSculptNode>	pOther;
	size				i;
	size				j;

	if (pNode->IsOutput() || !mapNodes.Contains(pNode))
	{
		return false;
	}
	for (i = 0; i < mapNodes.NumElements(); i++)
	{
		pOther = mapNodes.Get(i);
		for (j = 0; j < NumInputs(&pOther); j++)
		{
			if (GetInput(&pOther, j) == pNode->mpTexture)
			{
				Unlink(&pOther, j);
			}
		}
	}
	mapNodes.Remove(pNode);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// How many links the node is from the output by the longest way: 0 for
// the output, and for a node that doesn't reach it.
//
//////////////////////////////////////////////////////////////////////////
size CSculptGraph::GetDepth(CSculptNode* pcNode)
{
	Ptr<CSculptNode>	pOther;
	size				uiDepth;
	size				uiOther;
	size				i;
	size				j;

	uiDepth = 0;
	for (i = 0; i < mapNodes.NumElements(); i++)
	{
		pOther = mapNodes.Get(i);
		for (j = 0; j < NumInputs(&pOther); j++)
		{
			if (!pcNode->IsOutput() && (&GetInput(&pOther, j) == &pcNode->mpTexture))
			{
				uiOther = GetDepth(&pOther);
				if (pOther->IsOutput() || (uiOther > 0))
				{
					uiDepth = (uiOther + 1 > uiDepth) ? uiOther + 1 : uiDepth;
				}
			}
		}
	}
	return uiDepth;
}


//////////////////////////////////////////////////////////////////////////
//
// Where pcNode is among the nodes: their number if it isn't one.
//
//////////////////////////////////////////////////////////////////////////
size CSculptGraph::IndexOf(CSculptNode* pcNode)
{
	size	i;

	for (i = 0; i < mapNodes.NumElements(); i++)
	{
		if (&mapNodes.Get(i) == pcNode)
		{
			return i;
		}
	}
	return mapNodes.NumElements();
}


//////////////////////////////////////////////////////////////////////////
//
// Each node's texture is copied (SolidCopy), then each copy's inputs are
// pointed at the copies of what the original's point at: a node's, or a
// value of its own.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSculptGraph> CSculptGraph::Copy(void)
{
	Ptr<CSculptGraph>	pCopy;
	Ptr<CSculptNode>	pNode;
	Ptr<CSculptNode>	pNew;
	Ptr<CSculptNode>	pLinked;
	Ptr<CSolidTexture>	pTexture;
	Ptr<CSolidFlat>		pFlat;
	size				i;
	size				j;

	pCopy = OMalloc<CSculptGraph>(Ptr<CSolidTexture>());
	pCopy->GetOutput()->msPosition = GetOutput()->msPosition;
	pCopy->GetOutput()->mbPlaced = GetOutput()->mbPlaced;
	for (i = 1; i < mapNodes.NumElements(); i++)
	{
		pNode = mapNodes.Get(i);
		pTexture = SolidCopy(&pNode->mpTexture);
		pNew = OMalloc<CSculptNode>(pTexture.IsNotNull() ? pTexture : pNode->mpTexture);
		pNew->msPosition = pNode->msPosition;
		pNew->mbPlaced = pNode->mbPlaced;
		pCopy->mapNodes.Add(pNew);
	}

	for (i = 0; i < mapNodes.NumElements(); i++)
	{
		pNode = mapNodes.Get(i);
		pNew = pCopy->mapNodes.Get(i);
		for (j = 0; j < NumInputs(&pNode); j++)
		{
			pTexture = GetInput(&pNode, j);
			pLinked = FindNode(&pTexture);
			if (pLinked.IsNotNull())
			{
				pCopy->SetInput(&pNew, j, pCopy->mapNodes.Get(IndexOf(&pLinked))->mpTexture);
			}
			else
			{
				pFlat = pTexture;
				pCopy->SetInput(&pNew, j, OMalloc<CSolidFlat>(&pFlat->msSample));
			}
		}
	}
	return pCopy;
}

