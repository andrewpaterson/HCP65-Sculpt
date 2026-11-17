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
#ifndef __SCULPT_PANEL_H__
#define __SCULPT_PANEL_H__
#include "BaseLib/Float32Vec3.h"
#include "WindowLib/ComboBox.h"
#include "WindowLib/ComponentEvent.h"
#include "WindowLib/GridContainer.h"
#include "WindowLib/Label.h"
#include "WindowLib/NumberField.h"
#include "WindowLib/WindowTheme.h"


#define SCULPT_PANEL_WIDTH	200


//What the material is looked at on.
enum ESculptShape
{
	SS_Sphere,
	SS_Box,
	SS_Both,		//A sphere and a box, met: the box's faces are square to bricks, and where they meet is a hollow.
	SS_Sculpted,	//The spheres dropped by clicking in the view.

	SS_NumShapes,
};


//What of the material is shown.
enum ESculptShow
{
	SSH_Baked,		//Its colour, shaded by its height's relief: what a bake makes of it.
	SSH_Colour,		//Its colour alone.
	SSH_Height,		//One of its other channels, as greys.
	SSH_Shine,
	SSH_Gloss,
	SSH_Glow,

	SSH_NumShows,
};


//The settings beside Sculpt's view that aren't the material's: the shape it's looked at on, what of it is shown, and
//how it's baked (the most samples each way in a texel, how strongly its height shades it, and where the light that
//shades it is).  Its change event is called when any is changed with it.
class CSculptPanel : public CGridContainer
{
CONSTRUCTABLE(CSculptPanel);
DESTRUCTABLE(CSculptPanel);
protected:
	Ptr<CComboBox>					mpShape;
	Ptr<CComboBox>					mpShow;
	Ptr<CNumberField>				mpSamples;
	Ptr<CNumberField>				mpRelief;
	Ptr<CNumberField>				mapLight[3];
	int32							miNumRows;
	CComponentEvent<CSculptPanel>	mcChanged;

public:
	void							Init(Ptr<CWindow> pWindow, Ptr<CWindowTheme> pTheme);
	void							Class(void) override;
	void							Free(void) override;

	ESculptShape					GetShape(void);
	void							SetShape(ESculptShape eShape);
	ESculptShow						GetShow(void);
	void							SetShow(ESculptShow eShow);
	int32							GetSamples(void);
	float32							GetRelief(void);
	void							GetLight(SFloat32Vec3* psLight);
	void							SetBake(int32 iSamples, float32 fRelief, SFloat32Vec3* psLight);
	CComponentEvent<CSculptPanel>*	GetChangeEvent(void);

	void							Chosen(CComboBox* pcChoice);
	void							NumberChanged(CNumberField* pcNumber);

protected:
	void							AddHeading(Ptr<CWindowTheme> pTheme, const char* szText);
	void							AddRow(Ptr<CComponent> pComponent);
	Ptr<CNumberField>				AddNumber(Ptr<CWindowTheme> pTheme, const char* szName, float64 fMinimum, float64 fMaximum);
};


#endif // __SCULPT_PANEL_H__

