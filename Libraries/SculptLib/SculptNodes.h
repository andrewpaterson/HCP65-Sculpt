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
#include "WindowLib/NodeEditor.h"
#include "SculptField.h"
#include "SculptGraph.h"


#define SCULPT_NODES_COLUMN_GAP		60	//Between a column of nodes and the next, when they're arranged.
#define SCULPT_NODES_ROW_GAP		20	//Between a node and the one under it.


//A material's graph (CSculptGraph) in a node editor (WindowLib's CNodeEditor): a box for each of the graph's nodes and
//a link for each input that's fed by one.  It is the editor's listener: a link made, taken off or not allowed, and a node
//moved, are the graph's.  Its change event is called whenever the material would be baked differently.
//
//Under a box's output are its texture's settings, a row for each field its Class() names that a node can show
//(CSculptField): a number field for a number, three for a vector, a tick for a bool.  A setting changed is put in the
//texture and the change event called.
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
	void							FieldChanged(void);
	size							NumFields(void);
	Ptr<CSculptField>				GetField(size uiIndex);

	bool							CanLink(CNodeBox* pcFrom, size uiOutput, CNodeBox* pcTo, size uiInput) override;
	void							Linked(CNodeBox* pcFrom, size uiOutput, CNodeBox* pcTo, size uiInput) override;
	void							Unlinked(CNodeBox* pcFrom, size uiOutput, CNodeBox* pcTo, size uiInput) override;
	void							NodesMoved(void) override;

protected:
	Ptr<CNodeBox>					MakeBox(Ptr<CSculptNode> pNode);
	void							MakeSettings(Ptr<CNodeBox> pBox, Ptr<CSolidTexture> pTexture);
	Ptr<CSculptField>				AddField(Ptr<CSolidTexture> pTexture, CUnmanagedField* pcField, size uiElement);
	void							MakeLinks(void);
};


#endif // __SCULPT_NODES_H__

