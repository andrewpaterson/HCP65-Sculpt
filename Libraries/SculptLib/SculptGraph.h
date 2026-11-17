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
#ifndef __SCULPT_GRAPH_H__
#define __SCULPT_GRAPH_H__
#include "StandardLib/Array.h"
#include "StandardLib/Object.h"
#include "SupportLib/SolidTexture.h"


//A node of a material's graph: a solid texture and where its box is in the node editor.  The material's output is a node
//too, with no texture: its one input is what the material is.
class CSculptNode : public CObject
{
CONSTRUCTABLE(CSculptNode);
DESTRUCTABLE(CSculptNode);
public:
	Ptr<CSolidTexture>	mpTexture;  //NULL for the output.
	SInt32Vec2			msPosition;  //In the node editor's space.
	bool				mbPlaced;  //msPosition has been set: a node that hasn't been is put where the graph is arranged.

	void	Init(Ptr<CSolidTexture> pTexture);
	void	Class(void) override;
	void	Free(void) override;

	bool	IsOutput(void);
	void	Place(int32 x, int32 y);
};


//A material as a node editor shows it: the solid textures it's made of as nodes, and what it is (its surface, the
//texture linked to its output).  The textures are the material; this is how they're looked at.
//
//A texture's inputs are the pointers its Class() names, less any said to be hidden (SolidShow.h): a link from a node to
//an input is the input pointing at the node's texture.  An input always points at something.  With no link it points at
//a flat texture of its own (CSolidFlat), which is the input's value and not a node: so every flat texture in a material
//is shown as the value of the input it feeds.  Two inputs fed by one flat are each given their own when the graph is
//made, as are inputs that point at nothing, whose value is the opaque black an operator makes of nothing.
//
//A node can't be linked to an input of itself, nor of anything it's made from, however far down.
//
//Copy is another graph of other textures that are as this one's and linked as they are, its nodes in the same order and
//places: what undo keeps.  A texture of a class that can't be made new (SolidShow.h) is shared, not copied.
class CSculptGraph : public CObject
{
CONSTRUCTABLE(CSculptGraph);
DESTRUCTABLE(CSculptGraph);
protected:
	Ptr<CSolidTexture>		mpSurface;  //What the output's input points at: never NULL.
	CArray<CSculptNode>		mapNodes;  //The first is the output.

public:
	void				Init(Ptr<CSolidTexture> pSurface);
	void				Class(void) override;
	void				Free(void) override;

	Ptr<CSolidTexture>	GetSurface(void);
	size				NumNodes(void);
	Ptr<CSculptNode>	GetNode(size uiIndex);
	Ptr<CSculptNode>	GetOutput(void);
	Ptr<CSculptNode>	FindNode(CSolidTexture* pcTexture);

	size				NumInputs(CSculptNode* pcNode);
	void				GetInputLabel(CChars* pszDest, CSculptNode* pcNode, size uiInput);
	Ptr<CSolidTexture>	GetInput(CSculptNode* pcNode, size uiInput);
	Ptr<CSculptNode>	GetLinked(CSculptNode* pcNode, size uiInput);

	bool				CanLink(CSculptNode* pcFrom, CSculptNode* pcTo, size uiInput);
	bool				Link(CSculptNode* pcFrom, CSculptNode* pcTo, size uiInput);
	void				Unlink(CSculptNode* pcTo, size uiInput);

	Ptr<CSculptNode>	AddNode(Ptr<CSolidTexture> pTexture);
	bool				RemoveNode(Ptr<CSculptNode> pNode);
	size				GetDepth(CSculptNode* pcNode);
	Ptr<CSculptGraph>	Copy(void);

	static bool			IsValue(Ptr<CSolidTexture> pTexture);

protected:
	void				SetInput(CSculptNode* pcNode, size uiInput, Ptr<CSolidTexture> pTexture);
	void				AddNodesOf(Ptr<CSolidTexture> pTexture);
	void				GiveInputsValues(CSculptNode* pcNode);
	bool				DependsOn(CSolidTexture* pcTexture, CSolidTexture* pcOn);
	size				IndexOf(CSculptNode* pcNode);
};


//A solid texture's inputs, from its Class(): its pointers, less the hidden ones.
size				SolidNumInputs(CSolidTexture* pcTexture);
const char*			SolidInputField(CSolidTexture* pcTexture, size uiInput);
Ptr<CSolidTexture>	SolidGetInput(CSolidTexture* pcTexture, size uiInput);
void				SolidSetInput(CSolidTexture* pcTexture, size uiInput, Ptr<CSolidTexture> pInput);


#endif // __SCULPT_GRAPH_H__

