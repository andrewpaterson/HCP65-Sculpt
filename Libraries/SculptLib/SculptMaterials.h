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
#ifndef __SCULPT_MATERIALS_H__
#define __SCULPT_MATERIALS_H__
#include "SupportLib/Shape.h"
#include "SupportLib/SolidTexture.h"


//The materials Sculpt starts a graph from: ProceduralLib's (SolidMaterials.h), by name, for a menu.
size				NumSculptMaterials(void);
const char*			GetSculptMaterialName(size uiMaterial);
Ptr<CSolidTexture>	MakeSculptMaterial(size uiMaterial, uint32 uiSeed);  //NULL if there's no such material.


#endif // __SCULPT_MATERIALS_H__

