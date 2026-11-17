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
#ifndef __SCULPT_VALUE_H__
#define __SCULPT_VALUE_H__
#include "StandardLib/Object.h"
#include "WindowLib/ColourField.h"
#include "WindowLib/NumberField.h"
#include "ProceduralLib/SolidFlat.h"
#include "SculptGraph.h"


class CSculptNodes;


//An input's own value, as a node shows it while nothing is linked to the input: the flat texture the input points at
//(CSculptGraph), shown and changed as a colour (a colour field) or, for an input that's said to be a number from 0 to 1
//(SolidShow.h: a ramp's or a mask's factor), as a number (a number field), which is a grey of that brightness.
//
//It's found afresh from its node and input each time, as taking a link off gives the input another flat texture.
class CSculptValue : public CObject
{
CONSTRUCTABLE(CSculptValue);
DESTRUCTABLE(CSculptValue);
protected:
	Ptr<CSculptGraph>	mpGraph;
	Ptr<CSculptNode>	mpNode;
	size				muiInput;
	Ptr<CNumberField>	mpNumber;  //One of these two is what it's shown with.
	Ptr<CColourField>	mpColour;
	CSculptNodes*		mpcNodes;  //Not held: they hold this.

public:
	void				Init(Ptr<CSculptGraph> pGraph, Ptr<CSculptNode> pNode, size uiInput, CSculptNodes* pcNodes);
	void				Class(void) override;
	void				Free(void) override;

	Ptr<CComponent>		Make(Ptr<CWindow> pWindow, Ptr<CWindowTheme> pTheme, const char* szLabel);
	Ptr<CSolidFlat>		GetFlat(void);
	Ptr<CSculptNode>	GetNode(void) const;
	size				GetInput(void) const;
	Ptr<CNumberField>	GetNumber(void) const;
	Ptr<CColourField>	GetColour(void) const;
	void				Show(void);

	void				NumberChanged(CNumberField* pcNumber);
	void				ColourChanged(CColourField* pcColour);

protected:
	void				SetColour(float32 fRed, float32 fGreen, float32 fBlue);
};


#endif // __SCULPT_VALUE_H__

