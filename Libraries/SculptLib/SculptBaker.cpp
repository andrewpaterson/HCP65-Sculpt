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
#include "BaseLib/Logger.h"
#include "StandardLib/ClassDefines.h"
#include "StandardLib/Objects.h"
#include "StandardLib/ObjectsLock.h"
#include "SculptBaker.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptBaker::Init(CJobs* pcJobs)
{
	PreInit();
	mpWaiting = NULL;
	mpBaking = NULL;
	mpcJobs = pcJobs;
	mcThread.Init();
	mcMutex.Init();
	mcWake.Init();
	mbStop = false;
	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptBaker::Class(void)
{
	M_Pointer(mpWaiting);
	M_Pointer(mpBaking);
	U_Pointer(mpcJobs);
	U_Unknown(CNativeThread, mcThread);
	U_Unknown(CMutex, mcMutex);
	U_Unknown(CConditionVariable, mcWake);
	U_Bool(mbStop);
}


//////////////////////////////////////////////////////////////////////////
//
// A thread still running here was never stopped.  It can't be waited for
// (see the class), and would go on to use what this frees.
//
//////////////////////////////////////////////////////////////////////////
void CSculptBaker::Free(void)
{
	if (mcThread.IsStarted())
	{
		gcLogger.Error2(__METHOD__, " The baker's thread wasn't stopped (Stop) before the baker was freed.", NULL);
	}
	mcWake.Kill();
	mcMutex.Kill();
	mcThread.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
static void SculptBakerThread(void* pvBaker)
{
	((CSculptBaker*)pvBaker)->Run();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CSculptBaker::Start(void)
{
	if (!IsObjectsLocking())
	{
		return gcLogger.Error2(__METHOD__, " The objects' locking is off: ObjectsLocking(true) comes first.", NULL);
	}
	if (mcThread.IsStarted())
	{
		return gcLogger.Error2(__METHOD__, " The baker's thread is started already.", NULL);
	}
	mbStop = false;
	return mcThread.Start(SculptBakerThread, this);
}


//////////////////////////////////////////////////////////////////////////
//
// The thread finishes the bake it is making, if any, and ends; a bake
// still waiting is dropped.
//
//////////////////////////////////////////////////////////////////////////
void CSculptBaker::Stop(void)
{
	if (!mcThread.IsStarted())
	{
		return;
	}

	mcMutex.Lock();
	mbStop = true;
	mcWake.WakeAll();
	mcMutex.Unlock();
	mcThread.Join();
	mcThread.Kill();
	mcThread.Init();

	if (mpWaiting.IsNotNull())
	{
		mpWaiting->SetState(SBS_Dropped);
		mpWaiting = NULL;
	}
	mpBaking = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CSculptBaker::IsStarted(void) const
{
	return mcThread.IsStarted();
}


//////////////////////////////////////////////////////////////////////////
//
// pBake waits to be made, in place of any that was waiting; a bake at the
// whole density being made is stopped for it.  Without a thread it is
// dropped at once.
//
//////////////////////////////////////////////////////////////////////////
void CSculptBaker::Bake(Ptr<CSculptBake> pBake)
{
	if (!mcThread.IsStarted())
	{
		pBake->SetState(SBS_Dropped);
		return;
	}

	mcMutex.Lock();
	if (mpWaiting.IsNotNull())
	{
		mpWaiting->SetState(SBS_Dropped);
	}
	if (mpBaking.IsNotNull() && !mpBaking->mbCoarse)
	{
		mpBaking->Stop();
	}
	pBake->SetState(SBS_Waiting);
	mpWaiting = pBake;
	mcWake.WakeOne();
	mcMutex.Unlock();
}


//////////////////////////////////////////////////////////////////////////
//
// The bake that was waiting, once there is one: it is the one baking now.
// NULL when the thread is to stop.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSculptBake> CSculptBaker::Take(void)
{
	Ptr<CSculptBake>	pBake;

	mcMutex.Lock();
	while (!mbStop && mpWaiting.IsNull())
	{
		mcWake.Wait(&mcMutex);
	}
	if (!mbStop)
	{
		pBake = mpWaiting;
		mpWaiting = NULL;
		mpBaking = pBake;
		pBake->SetState(SBS_Baking);
	}
	mcMutex.Unlock();
	return pBake;
}


//////////////////////////////////////////////////////////////////////////
//
// The bake that was being made is made, or was stopped: it's its asker's
// alone now.
//
//////////////////////////////////////////////////////////////////////////
void CSculptBaker::Made(void)
{
	mcMutex.Lock();
	mpBaking = NULL;
	mcMutex.Unlock();
}


//////////////////////////////////////////////////////////////////////////
//
// The thread: a worker with an object system of its own, which makes each
// bake it is given until it's stopped.
//
//////////////////////////////////////////////////////////////////////////
void CSculptBaker::Run(void)
{
	ObjectsWorkerInit();
	for (;;)
	{
		Ptr<CSculptBake>	pBake;

		pBake = Take();
		if (pBake.IsNull())
		{
			break;
		}
		pBake->Make(mpcJobs);
		Made();
	}
	ObjectsThreadKill();
}
