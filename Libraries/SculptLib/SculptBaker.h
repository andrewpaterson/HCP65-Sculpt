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
#ifndef __SCULPT_BAKER_H__
#define __SCULPT_BAKER_H__
#include "BaseLib/ConditionVariable.h"
#include "BaseLib/Mutex.h"
#include "BaseLib/NativeThread.h"
#include "SculptBake.h"


//A thread that makes bakes' meshes (CSculptBake::Make), so that the thread which draws the window needn't wait for
//them: a change's bake is 13 ms and one at the whole density 180, in every frame that had one.
//
//The thread has an object system of its own (StandardLib's ObjectsWorkerInit): the stack it makes a mesh with, and
//the meshes made on the way, are its objects and are freed on it, and the mesh is adopted by the main object system
//(ObjectsAdopt) before the bake is said to be baked.  So the objects' locking must be on before Start
//(ObjectsLocking(true)), and stays on until Stop.
//
//Bake gives it a bake to make.  It makes one at a time, and keeps one waiting: a bake given while another waits takes
//its place, and that one is dropped (SBS_Dropped), as what it would show is already out of date.  So is the one being
//made, if it is at the whole density: it is stopped (CSculptBake::Stop), as it is the long one and what follows a
//change shouldn't wait for it.  A coarse one is finished, so that something is shown while changes keep coming.  The
//caller looks at its bakes' states to find them made (the page does at its window's ticks).  The bakes are held here
//while they wait and are made, so whoever asked may let go of one, or go itself, at any time.
//
//Stop ends the thread and waits for it, and must be called before the baker is let go of, and not from a Free: the
//thread may be waiting for the objects' lock, which a thread in a Free holds.
class CSculptBaker : public CObject
{
CONSTRUCTABLE(CSculptBaker);
DESTRUCTABLE(CSculptBaker);
protected:
	Ptr<CSculptBake>	mpWaiting;
	Ptr<CSculptBake>	mpBaking;  //While it's made.
	CJobs*				mpcJobs;
	CNativeThread		mcThread;
	CMutex				mcMutex;  //Over mpWaiting, mpBaking and mbStop.  Taken before the objects' lock, never with it held.
	CConditionVariable	mcWake;
	bool				mbStop;

public:
	void	Init(CJobs* pcJobs);  //pcJobs are threads to bake rows on (CMeshBake::SetJobs), or NULL.  They outlive it.
	void	Class(void) override;
	void	Free(void) override;

	bool	Start(void);
	void	Stop(void);
	bool	IsStarted(void) const;

	void	Bake(Ptr<CSculptBake> pBake);

	void	Run(void);  //The thread's.

protected:
	Ptr<CSculptBake>	Take(void);
	void				Made(void);
};


#endif // __SCULPT_BAKER_H__
