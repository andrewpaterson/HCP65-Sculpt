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
#ifndef __SCULPT_RAMP_H__
#define __SCULPT_RAMP_H__
#include "StandardLib/Object.h"
#include "WindowLib/ColourRampBar.h"
#include "WindowLib/NodeBox.h"
#include "ProceduralLib/SolidRamp.h"


#define SCULPT_RAMP_EXTRAS	4	//A key's numbers besides its colour: height, shine, gloss and glow.


class CSculptNodes;


//A ramp's keys, as its node shows them: a colour ramp bar (WindowLib's CColourRampBar), a stop for each key at the
//key's value in the key's colour, and under it the chosen key's height, shine, gloss and glow, which a colour doesn't
//say.  The bar's changes are the ramp's: its keys are made again from the bar's stops, each keeping the rest of its
//sample (a stop's data is its key's index, so a key is known wherever its stop's been dragged); a stop the bar added
//is a key of the ramp's sample at its value.
//
//The nodes hold this while the ramp's box holds the components.
class CSculptRamp : public CObject
{
CONSTRUCTABLE(CSculptRamp);
DESTRUCTABLE(CSculptRamp);
protected:
	Ptr<CSolidRamp>			mpRamp;
	Ptr<CColourRampBar>		mpBar;
	Ptr<CNumberField>		mapExtras[SCULPT_RAMP_EXTRAS];
	CSculptNodes*			mpcNodes;  //Not held: they hold this.

public:
	void					Init(Ptr<CSolidRamp> pRamp, CSculptNodes* pcNodes);
	void					Class(void) override;
	void					Free(void) override;

	void					AddRows(Ptr<CNodeBox> pBox, Ptr<CWindow> pWindow, Ptr<CWindowTheme> pTheme);
	Ptr<CSolidRamp>			GetRamp(void);
	Ptr<CColourRampBar>		GetBar(void);
	Ptr<CNumberField>		GetExtra(size uiExtra);

	void					BarChanged(CColourRampBar* pcBar);
	void					BarChosen(CColourRampBar* pcBar);
	void					ExtraChanged(CNumberField* pcNumber);

protected:
	void					ShowExtras(void);
	static float32*			Extra(SSolidSample* psSample, size uiExtra);
};


#endif // __SCULPT_RAMP_H__

