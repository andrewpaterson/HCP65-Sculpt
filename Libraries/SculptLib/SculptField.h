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
#ifndef __SCULPT_FIELD_H__
#define __SCULPT_FIELD_H__
#include "StandardLib/Object.h"
#include "StandardLib/UnmanagedField.h"
#include "SupportLib/SolidTexture.h"
#include "WindowLib/CheckBox.h"
#include "WindowLib/ComboBox.h"
#include "WindowLib/NumberField.h"


class CSculptNodes;


//What kind of thing a field of a solid texture is, from the type its Class() gave it.
enum ESculptFieldKind
{
	SFK_None,		//Nothing a node shows: a structure, an array, a string.
	SFK_Number,		//A float.
	SFK_Whole,		//An integer.
	SFK_Tick,		//A bool.
	SFK_Enum,		//An enum.
	SFK_Vector,		//Three floats.
};


//One number of a solid texture, as a node shows it: a field its Class() names (or one of a vector's three floats), and
//the component it's shown and changed with: a number field, a tick, or for an enum a combo box of its names.  The component's change is the field's: the number is put in the texture,
//the texture is told its fields changed (CSolidTexture::Changed) and the nodes that the material has.
//
//The nodes hold these while their boxes hold the components.
class CSculptField : public CObject
{
CONSTRUCTABLE(CSculptField);
DESTRUCTABLE(CSculptField);
protected:
	Ptr<CSolidTexture>	mpTexture;
	CUnmanagedField*	mpcField;  //The texture's class's: there while the class is.
	size				muiElement;  //Which float of a vector.
	CSculptNodes*		mpcNodes;  //Not held: they hold this.

public:
	void					Init(Ptr<CSolidTexture> pTexture, CUnmanagedField* pcField, size uiElement, CSculptNodes* pcNodes);
	void					Class(void) override;
	void					Free(void) override;

	float64					Get(void);
	void					Set(float64 fValue);
	char*					GetFieldName(void) const;

	Ptr<CNumberField>		MakeNumber(Ptr<CWindow> pWindow, Ptr<CWindowTheme> pTheme, const char* szLabel);
	Ptr<CCheckBox>			MakeTick(Ptr<CWindow> pWindow, Ptr<CWindowTheme> pTheme, const char* szLabel);
	Ptr<CComboBox>			MakeChoice(Ptr<CWindow> pWindow, Ptr<CWindowTheme> pTheme);
	void					NumberChanged(CNumberField* pcNumber);
	void					Ticked(CCheckBox* pcTick);
	void					Chosen(CComboBox* pcChoice);

	static ESculptFieldKind	Kind(CUnmanagedField* pcField);
};


#endif // __SCULPT_FIELD_H__

