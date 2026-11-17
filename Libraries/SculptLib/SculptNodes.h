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
#ifndef __SCULPT_NODES_H__
#define __SCULPT_NODES_H__
#include "StandardLib/Object.h"
#include "WindowLib/ComponentEvent.h"
#include "WindowLib/MenuContainer.h"
#include "WindowLib/NodeEditor.h"
#include "SculptField.h"
#include "SculptGraph.h"
#include "SculptRamp.h"
#include "SculptValue.h"


#define SCULPT_NODES_COLUMN_GAP		60	//Between a column of nodes and the next, when they're arranged.
#define SCULPT_NODES_ROW_GAP		20	//Between a node and the one under it.
#define SCULPT_NODES_COPY_OFFSET	30	//Right and down from a node to its copy.


//An item of an Add menu, and which class of texture (SolidShow.h's table) it adds.
struct SSculptAddItem
{
	CMenuItem*	pcItem;  //Its menu holds it.
	size		uiShow;
};

typedef CArrayTemplate<SSculptAddItem>	CArraySculptAddItem;


//A material's graph (CSculptGraph) in a node editor (WindowLib's CNodeEditor): a box for each of the graph's nodes and
//a link for each input that's fed by one.  It is the editor's listener: a link made, taken off or not allowed, and a node
//moved, are the graph's.  Its change event is called whenever the material would be baked differently.
//
//Under a box's output are its texture's settings, a row for each field its Class() names that a node can show
//(CSculptField): a number field for a number, three for a vector, a tick for a bool, a combo box for an enum.  A ramp's
//keys, which are an array and no field of that kind, are a colour ramp bar (CSculptRamp).  A setting changed is put in the
//texture and the change event called.
//
//An input shows its own value while nothing is linked to it (CSculptValue): a colour, or a number for a factor.  A value
//changed is the material's too.
//
//Nodes are added from an Add menu (MakeAddMenu), a menu of the kinds of node with the classes of each under it, as
//Blender's is: the editor's own, which Shift+A (or the right button) opens under the pointer, and any other made, for
//a menu bar say.  A node added is a new texture linked to nothing, where the pointer was or in the middle of the editor.
//X or Delete removes the selected nodes (never the output), each input they fed having its own value again; Shift+D
//copies them, a little to the right and down, the copies sharing the originals' inputs.
//
//A box's data is its node (the graph holds it).  Nodes that haven't been placed are arranged in columns by how far each
//is from the output (CSculptGraph::GetDepth), the output on the right, as Blender's graphs read.
class CSculptNodes : public CObject, public CNodeEditorListener
{
CONSTRUCTABLE(CSculptNodes);
DESTRUCTABLE(CSculptNodes);
protected:
	Ptr<CNodeEditor>				mpEditor;
	Ptr<CSculptGraph>				mpGraph;
	CArray<CSculptField>			mapFields;  //The settings the boxes show.
	CArray<CSculptValue>			mapValues;  //The inputs' own values.
	CArray<CSculptRamp>				mapRamps;  //The ramps' keys.
	CArraySculptAddItem				masAddItems;  //Of every Add menu made.
	Ptr<CMenuContainer>				mpAddMenu;  //The editor's context menu.
	SInt32Vec2						msAddPosition;  //Where the next node added goes, in the editor's space.
	bool							mbAddAtPointer;  //It's where Shift+A was pressed; otherwise the middle of the editor.
	CComponentEvent<CSculptNodes>	mcChanged;

public:
	void							Init(Ptr<CNodeEditor> pEditor);
	void							Class(void) override;
	void							Free(void) override;

	void							SetGraph(Ptr<CSculptGraph> pGraph);
	Ptr<CSculptGraph>				GetGraph(void);
	Ptr<CNodeEditor>				GetEditor(void);
	CComponentEvent<CSculptNodes>*	GetChangeEvent(void);

	Ptr<CNodeBox>					FindBox(CSculptNode* pcNode);
	static CSculptNode*				GetNode(CNodeBox* pcBox);
	void							Arrange(void);
	void							Rebuild(void);

	Ptr<CMenuContainer>				MakeAddMenu(void);
	void							AddMenuItemClicked(CMenuItem* pcItem);
	Ptr<CSculptNode>				AddNodeOf(size uiShow);
	void							FieldChanged(void);
	size							NumFields(void);
	Ptr<CSculptField>				GetField(size uiIndex);
	Ptr<CSculptValue>				FindValue(CSculptNode* pcNode, size uiInput);
	Ptr<CSculptRamp>				FindRamp(CSolidTexture* pcTexture);

	bool							CanLink(CNodeBox* pcFrom, size uiOutput, CNodeBox* pcTo, size uiInput) override;
	void							Linked(CNodeBox* pcFrom, size uiOutput, CNodeBox* pcTo, size uiInput) override;
	void							Unlinked(CNodeBox* pcFrom, size uiOutput, CNodeBox* pcTo, size uiInput) override;
	void							NodesMoved(void) override;
	void							DeleteSelected(void) override;
	void							DuplicateSelected(void) override;
	void							AddNode(int32 x, int32 y) override;

protected:
	Ptr<CNodeBox>					MakeBox(Ptr<CSculptNode> pNode);
	void							MakeSettings(Ptr<CNodeBox> pBox, Ptr<CSolidTexture> pTexture);
	Ptr<CSculptField>				AddField(Ptr<CSolidTexture> pTexture, CUnmanagedField* pcField, size uiElement);
	void							MakeLinks(void);
};


#endif // __SCULPT_NODES_H__

