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
#ifndef __SOLID_SHOW_H__
#define __SOLID_SHOW_H__
#include "BaseLib/Chars.h"
#include "SupportLib/ColourARGB32.h"
#include "SupportLib/SolidTexture.h"


//The kinds of node, as Blender sorts and colours them.
enum ESolidNodeKind
{
	SNK_Texture,	//Makes a pattern from nothing: noise, cells, a gradient, bricks.
	SNK_Colour,		//Changes or mixes colours.
	SNK_Converter,	//Turns one thing into another: a ramp.
	SNK_Vector,		//Moves where a texture is looked at.
	SNK_Output,		//What the material is.

	SNK_NumKinds,
};


//How a field of a solid texture is shown on its node, where what Class() says of it (its name and its type) isn't
//enough.
enum ESolidFieldShow
{
	SFS_Plain,		//As its type is: a number, a whole number, a tick, three numbers.
	SFS_Hidden,		//Not at all.
	SFS_Range,		//A number kept between fMinimum and fMaximum.  Of an input: its own value is a number, not a colour.
	SFS_Enum,		//One of szNames, which are separated by |, in the enum's order.
};


//How a class of solid texture is shown as a node: what it's called (Blender's name where it has the same thing) and its
//kind.  A class with no entry is called by its class's name less "CSolid", and is a texture.
struct SSolidNodeShow
{
	const char*		szClass;
	const char*		szTitle;
	ESolidNodeKind	eKind;
};


//How a field is shown.  A field with no entry is plain, and labelled by its name less its prefix ("mfScale" is "Scale").
struct SSolidFieldShow
{
	const char*		szClass;  //The class the field is shown this way on: CSolidTexture for every class.
	const char*		szField;
	const char*		szLabel;  //NULL for its name less its prefix.
	ESolidFieldShow	eShow;
	float64			fMinimum;
	float64			fMaximum;
	const char*		szNames;
};


#define SOLID_SOCKET_COLOUR		Set32BitColour((uint8)199, (uint8)199, (uint8)41)	//A sample's, as Blender's colour sockets are.


void				SolidNodeTitle(CChars* pszDest, CSolidTexture* pcTexture);
ESolidNodeKind		SolidNodeKind(CSolidTexture* pcTexture);
const char*			SolidNodeKindName(ESolidNodeKind eKind);
ARGB32				SolidNodeKindColour(ESolidNodeKind eKind);
SSolidFieldShow*	FindSolidFieldShow(CSolidTexture* pcTexture, const char* szField);
void				SolidFieldLabel(CChars* pszDest, CSolidTexture* pcTexture, const char* szField);


#endif // __SOLID_SHOW_H__

