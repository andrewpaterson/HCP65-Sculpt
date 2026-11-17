/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela HCP65Sculpt

Codaphela HCP65Sculpt is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela HCP65Sculpt is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Codaphela HCP65Sculpt.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#ifndef __SCULPT_TICK_H__
#define __SCULPT_TICK_H__
#include "WindowLib/WindowTick.h"


class CSculptApp;


//The window's tick, which is the app's (CSculptApp::Tick).  The app holds the window and so this: it isn't held back.
class CSculptTick : public CWindowTick
{
CONSTRUCTABLE(CSculptTick);
DESTRUCTABLE(CSculptTick);
protected:
	CSculptApp*		mpcApp;

public:
	void	Init(CSculptApp* pcApp);
	void	Class(void) override;
	void	Tick(Ptr<CWindow> pWindow, int64 iUpdateTimeInMillieconds, int64 iTotalTimeInMillieconds) override;
};


#endif // __SCULPT_TICK_H__

