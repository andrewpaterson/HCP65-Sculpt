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
#include "BaseLib/Timer.h"
#include "StandardLib/Class.h"
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
	mapFields.Init();
	mapValues.Init();
	mapRamps.Init();
	mapHistory.Init();
	muiHistory = 0;
	miSettingTime = 0;
	masAddItems.Init();
	msAddPosition.Init(0, 0);
	mbAddAtPointer = false;
	mcChanged.Init();
	mpEditor->SetListener(this);
	mpAddMenu = MakeAddMenu();
	mpEditor->SetContextMenu(mpAddMenu);
	mpUndoAction = mpEditor->AddInputAction(IAF_KeyDown, "Z", this, &CSculptNodes::UndoAction);

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
	M_Embedded(mapFields);
	M_Embedded(mapValues);
	M_Embedded(mapRamps);
	M_Embedded(mapHistory);
	U_Size(muiHistory);
	U_Int64(miSettingTime);
	M_Pointer(mpUndoAction);
	U_Unknown(CArraySculptAddItem, masAddItems);
	M_Pointer(mpAddMenu);
	U_2Int32(msAddPosition);
	U_Bool(mbAddAtPointer);
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
		mpEditor->SetContextMenu(Ptr<CMenuContainer>());
		mpEditor->RemoveInputAction(mpUndoAction);
	}
	masAddItems.Kill();
	mcChanged.Kill();
}


Ptr<CSculptGraph> CSculptNodes::GetGraph(void) { return mpGraph; }
Ptr<CNodeEditor> CSculptNodes::GetEditor(void) { return mpEditor; }
CComponentEvent<CSculptNodes>* CSculptNodes::GetChangeEvent(void) { return &mcChanged; }
CSculptNode* CSculptNodes::GetNode(CNodeBox* pcBox) { return (CSculptNode*)pcBox->GetData(); }
size CSculptNodes::NumFields(void) { return mapFields.NumElements(); }
Ptr<CSculptField> CSculptNodes::GetField(size uiIndex) { return mapFields.Get(uiIndex); }


//////////////////////////////////////////////////////////////////////////
//
// The editor shows pGraph: its nodes as boxes, linked as its textures
// are, those not yet placed arranged, and all of them framed.
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::SetGraph(Ptr<CSculptGraph> pGraph)
{
	mpGraph = pGraph;
	Rebuild();
	mpEditor->FrameAll();

	//There's nothing to undo back to before it.
	mapHistory.RemoveAll();
	muiHistory = 0;
	miSettingTime = 0;
	if (mpGraph.IsNotNull())
	{
		mapHistory.Add(mpGraph->Copy());
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The editor's boxes and links are made again from the graph, which has
// more or fewer nodes: each where its node is, those not yet placed
// arranged.  The view isn't moved.
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::Rebuild(void)
{
	size	i;

	mpEditor->RemoveAllNodes();
	mapFields.RemoveAll();
	mapValues.RemoveAll();
	mapRamps.RemoveAll();
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
}


//////////////////////////////////////////////////////////////////////////
//
// A node's box: its title and kind's colour, its output (the output
// node has none), its texture's settings and its inputs, each with its
// own value to show while nothing is linked to it.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CNodeBox> CSculptNodes::MakeBox(Ptr<CSculptNode> pNode)
{
	Ptr<CNodeBox>		pBox;
	Ptr<CSculptValue>	pValue;
	CChars				szText;
	ESolidNodeKind		eKind;
	size				i;

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
		MakeSettings(pBox, pNode->mpTexture);
	}
	for (i = 0; i < mpGraph->NumInputs(&pNode); i++)
	{
		szText.Clear();
		mpGraph->GetInputLabel(&szText, &pNode, i);
		pValue = OMalloc<CSculptValue>(mpGraph, pNode, i, this);
		mapValues.Add(pValue);
		pBox->AddInput(szText.Text(), SOLID_SOCKET_COLOUR, pValue->Make(mpEditor->GetWindow(), mpEditor->GetTheme(), szText.Text()));
	}
	szText.Kill();
	return pBox;
}


//////////////////////////////////////////////////////////////////////////
//
// A row in pBox for each of pTexture's fields that a node can show and
// that isn't hidden, in the order its Class() names them: a number, a
// tick, a vector's three numbers under its label, or an enum's names to
// choose from (an enum with no names said of it isn't shown).
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::MakeSettings(Ptr<CNodeBox> pBox, Ptr<CSolidTexture> pTexture)
{
	static const char*	aszAxes[3] = { "X", "Y", "Z" };
	Ptr<CWindow>		pWindow;
	Ptr<CWindowTheme>	pTheme;
	Ptr<CSculptField>	pField;
	Ptr<CComboBox>		pChoice;
	Ptr<CSolidRamp>		pRampTexture;
	Ptr<CSculptRamp>	pRamp;
	CArrayVoidPtr*		papcFields;
	CUnmanagedField*	pcField;
	SSolidFieldShow*	psShow;
	ESculptFieldKind	eKind;
	CChars				szLabel;
	size				i;
	size				j;

	pWindow = mpEditor->GetWindow();
	pTheme = mpEditor->GetTheme();
	szLabel.Init();
	papcFields = pTexture->GetClass()->GetUnmanagedFields();
	for (i = 0; i < papcFields->NumElements(); i++)
	{
		pcField = (CUnmanagedField*)papcFields->GetPtr(i);
		psShow = FindSolidFieldShow(&pTexture, pcField->GetName());
		eKind = CSculptField::Kind(pcField);
		if ((psShow != NULL) && (psShow->eShow == SFS_Hidden))
		{
			continue;
		}

		szLabel.Clear();
		SolidFieldLabel(&szLabel, &pTexture, pcField->GetName());
		if ((eKind == SFK_Number) || (eKind == SFK_Whole))
		{
			pBox->AddRow(AddField(pTexture, pcField, 0)->MakeNumber(pWindow, pTheme, szLabel.Text()));
		}
		else if (eKind == SFK_Tick)
		{
			pBox->AddRow(AddField(pTexture, pcField, 0)->MakeTick(pWindow, pTheme, szLabel.Text()));
		}
		else if (eKind == SFK_Enum)
		{
			pField = AddField(pTexture, pcField, 0);
			pChoice = pField->MakeChoice(pWindow, pTheme);
			if (pChoice.IsNotNull())
			{
				pChoice->SetAlignment(CSH_Stretch, CSV_Center);
				pBox->AddRow(pChoice);
			}
			else
			{
				mapFields.Remove(pField);
			}
		}
		else if (eKind == SFK_Vector)
		{
			pBox->AddRow(OMalloc<CLabel>(pWindow, pTheme->GetFont(), szLabel.Text()));
			for (j = 0; j < 3; j++)
			{
				pBox->AddRow(AddField(pTexture, pcField, j)->MakeNumber(pWindow, pTheme, aszAxes[j]));
			}
		}
	}
	szLabel.Kill();

	//A ramp's keys aren't a field a node can show: they have a bar.
	pRampTexture = Ptr<CSolidRamp>(dynamic_cast<CSolidRamp*>(&pTexture));
	if (pRampTexture.IsNotNull())
	{
		pRamp = OMalloc<CSculptRamp>(pRampTexture, this);
		mapRamps.Add(pRamp);
		pRamp->AddRows(pBox, pWindow, pTheme);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The keys of the ramp pcTexture: NULL if it isn't one of the graph's
// ramps.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSculptRamp> CSculptNodes::FindRamp(CSolidTexture* pcTexture)
{
	Ptr<CSculptRamp>	pRamp;
	size				i;

	for (i = 0; i < mapRamps.NumElements(); i++)
	{
		pRamp = mapRamps.Get(i);
		if (&pRamp->GetRamp() == pcTexture)
		{
			return pRamp;
		}
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// A setting of pTexture's the nodes hold.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSculptField> CSculptNodes::AddField(Ptr<CSolidTexture> pTexture, CUnmanagedField* pcField, size uiElement)
{
	Ptr<CSculptField>	pField;

	pField = OMalloc<CSculptField>(pTexture, pcField, uiElement, this);
	mapFields.Add(pField);
	return pField;
}


//////////////////////////////////////////////////////////////////////////
//
// The value of pcNode's uiInput'th input: NULL if it has none.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSculptValue> CSculptNodes::FindValue(CSculptNode* pcNode, size uiInput)
{
	Ptr<CSculptValue>	pValue;
	size				i;

	for (i = 0; i < mapValues.NumElements(); i++)
	{
		pValue = mapValues.Get(i);
		if ((&pValue->GetNode() == pcNode) && (pValue->GetInput() == uiInput))
		{
			return pValue;
		}
	}
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// A setting or a value was changed: the material is another.
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::FieldChanged(void)
{
	Keep(true);
	mcChanged.Call(this);
}


//////////////////////////////////////////////////////////////////////////
//
// The graph has changed: what it now is is kept, after what it was, and
// anything undone is forgotten.  A setting's change (bSetting) close on
// another's takes the last's place, so a drag is undone at once.
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::Keep(bool bSetting)
{
	int64	iNow;
	bool	bSameStep;

	iNow = GetDebugTickCount();
	bSameStep = bSetting && (miSettingTime != 0) && (iNow - miSettingTime < SCULPT_NODES_UNDO_PAUSE) && (muiHistory > 0);
	miSettingTime = bSetting ? iNow : 0;

	while (mapHistory.NumElements() > muiHistory + (bSameStep ? 0 : 1))
	{
		mapHistory.RemoveAt(mapHistory.NumElements() - 1);
	}
	mapHistory.Add(mpGraph->Copy());
	muiHistory = mapHistory.NumElements() - 1;
}


//////////////////////////////////////////////////////////////////////////
//
// The graph is a copy of the muiHistory'th kept: shown where the view
// is, and baked.
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::ShowHistory(void)
{
	miSettingTime = 0;
	mpGraph = mapHistory.Get(muiHistory)->Copy();
	Rebuild();
	mcChanged.Call(this);
}


//////////////////////////////////////////////////////////////////////////
//
// The graph as it was before the last change.  False if there's none.
//
//////////////////////////////////////////////////////////////////////////
bool CSculptNodes::Undo(void)
{
	if (!CanUndo())
	{
		return false;
	}
	muiHistory--;
	ShowHistory();
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The change last undone, again.  False if nothing was undone since the
// last change.
//
//////////////////////////////////////////////////////////////////////////
bool CSculptNodes::Redo(void)
{
	if (!CanRedo())
	{
		return false;
	}
	muiHistory++;
	ShowHistory();
	return true;
}


bool CSculptNodes::CanUndo(void) { return muiHistory > 0; }
bool CSculptNodes::CanRedo(void) { return muiHistory + 1 < mapHistory.NumElements(); }


//////////////////////////////////////////////////////////////////////////
//
// Control+Z undoes and Control+Shift+Z redoes; Z alone is someone else's.
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::UndoAction(SComponentInputEvent* psEvent)
{
	if (!psEvent->bControl)
	{
		psEvent->bPass = true;
	}
	else if (psEvent->bShift)
	{
		Redo();
	}
	else
	{
		Undo();
	}
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
	Keep(false);
	mcChanged.Call(this);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::Unlinked(CNodeBox* pcFrom, size uiOutput, CNodeBox* pcTo, size uiInput)
{
	Ptr<CSculptValue>	pValue;

	mpGraph->Unlink(GetNode(pcTo), uiInput);
	pValue = FindValue(GetNode(pcTo), uiInput);
	if (pValue.IsNotNull())
	{
		pValue->Show();  //The input has a new value of its own.
	}
	Keep(false);
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
	Keep(false);
}


//////////////////////////////////////////////////////////////////////////
//
// A menu of the kinds of node, each with the classes of that kind that
// can be made new under it.  Its items add a node (AddMenuItemClicked).
//
//////////////////////////////////////////////////////////////////////////
Ptr<CMenuContainer> CSculptNodes::MakeAddMenu(void)
{
	Ptr<CMenuContainer>		pMenu;
	Ptr<CMenuContainer>		pKindMenu;
	SSolidNodeShow*			psShow;
	SSculptAddItem*			psItem;
	size					uiKind;
	size					i;

	pMenu = OMalloc<CMenuContainer>(mpEditor->GetWindow(), mpEditor->GetTheme());
	for (uiKind = 0; uiKind < SNK_Output; uiKind++)
	{
		pKindMenu = OMalloc<CMenuContainer>(mpEditor->GetWindow(), mpEditor->GetTheme());
		for (i = 0; i < NumSolidNodeShows(); i++)
		{
			psShow = GetSolidNodeShow(i);
			if (((size)psShow->eKind == uiKind) && (psShow->fMake != NULL))
			{
				psItem = masAddItems.Add();
				psItem->pcItem = &pKindMenu->AddItem(psShow->szTitle);
				psItem->uiShow = i;
			}
		}
		pKindMenu->GetItemClickEvent()->Add(this, &CSculptNodes::AddMenuItemClicked);
		pMenu->AddItem(SolidNodeKindName((ESolidNodeKind)uiKind), NULL, pKindMenu);
	}
	return pMenu;
}


//////////////////////////////////////////////////////////////////////////
//
// An Add menu's item was clicked: its class's node is added.
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::AddMenuItemClicked(CMenuItem* pcItem)
{
	size	i;

	for (i = 0; i < masAddItems.NumElements(); i++)
	{
		if (masAddItems.Get(i)->pcItem == pcItem)
		{
			AddNodeOf(masAddItems.Get(i)->uiShow);
			return;
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A new texture of the uiShow'th class in SolidShow.h's table, as a node
// linked to nothing: where Shift+A was pressed, or in the middle of the
// editor.  NULL for a class that isn't made new.  Nothing is baked: the
// material is as it was until the node's linked into it.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSculptNode> CSculptNodes::AddNodeOf(size uiShow)
{
	Ptr<CSculptNode>	pNode;
	SSolidNodeShow*		psShow;
	SInt32Vec2			sSize;
	SInt32Vec2			sPan;

	psShow = (uiShow < NumSolidNodeShows()) ? GetSolidNodeShow(uiShow) : NULL;
	if (mpGraph.IsNull() || (psShow == NULL) || (psShow->fMake == NULL))
	{
		return NULL;
	}
	if (!mbAddAtPointer)
	{
		sSize = mpEditor->GetActualSize();
		sPan = mpEditor->GetPan();
		msAddPosition.Init(sSize.x / 2 - sPan.x - NODE_BOX_WIDTH / 2, sSize.y / 2 - sPan.y);
	}
	mbAddAtPointer = false;

	pNode = mpGraph->AddNode(psShow->fMake());
	pNode->Place(msAddPosition.x, msAddPosition.y);
	Keep(false);
	Rebuild();
	mpEditor->Select(FindBox(&pNode), false);
	return pNode;
}


//////////////////////////////////////////////////////////////////////////
//
// Shift+A at x, y in the editor's space: the Add menu opens under the
// pointer, and what's chosen from it goes there.
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::AddNode(int32 x, int32 y)
{
	SInt32Vec2	sEditor;
	SInt32Vec2	sPan;

	msAddPosition.Init(x, y);
	mbAddAtPointer = true;
	sEditor = mpEditor->GetPosition();
	sPan = mpEditor->GetPan();
	mpEditor->OpenContextMenu(SInt32Vec2(sEditor.x + sPan.x + x, sEditor.y + sPan.y + y));
}


//////////////////////////////////////////////////////////////////////////
//
// The selected nodes go, but for the output: every input they fed has
// its own value again, and the material is baked if it changed.
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::DeleteSelected(void)
{
	Ptr<CNodeBox>		pBox;
	Ptr<CSculptNode>	pNode;
	bool				bRemoved;
	size				i;

	bRemoved = false;
	for (i = 0; i < mpEditor->NumNodes(); i++)
	{
		pBox = mpEditor->GetNode(i);
		pNode = GetNode(&pBox);
		if (pBox->IsSelected() && mpGraph->RemoveNode(pNode))
		{
			bRemoved = true;
		}
	}
	if (bRemoved)
	{
		Keep(false);
		Rebuild();
		mcChanged.Call(this);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A copy of each selected node whose class can be made new, a little
// right and down of it, fed by what feeds it.  The copies are then the
// selected nodes.
//
//////////////////////////////////////////////////////////////////////////
void CSculptNodes::DuplicateSelected(void)
{
	CArrayTemplate<CSculptNode*>	apcCopies;  //The graph holds them.
	Ptr<CNodeBox>		pBox;
	Ptr<CSculptNode>	pNode;
	Ptr<CSculptNode>	pCopy;
	Ptr<CSolidTexture>	pTexture;
	size				i;

	apcCopies.Init();
	for (i = 0; i < mpEditor->NumNodes(); i++)
	{
		pBox = mpEditor->GetNode(i);
		pNode = GetNode(&pBox);
		if (pBox->IsSelected() && !pNode->IsOutput())
		{
			pTexture = SolidCopy(&pNode->mpTexture);
			if (pTexture.IsNotNull())
			{
				pCopy = mpGraph->AddNode(pTexture);
				pCopy->Place(pNode->msPosition.x + SCULPT_NODES_COPY_OFFSET, pNode->msPosition.y + SCULPT_NODES_COPY_OFFSET);
				*apcCopies.Add() = &pCopy;
			}
		}
	}
	if (apcCopies.NumElements() > 0)
	{
		Keep(false);
		Rebuild();
		mpEditor->SelectNone();
		for (i = 0; i < apcCopies.NumElements(); i++)
		{
			mpEditor->Select(FindBox(*apcCopies.Get(i)), true);
		}
	}
	apcCopies.Kill();
}

