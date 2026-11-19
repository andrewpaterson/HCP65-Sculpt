/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela TestSculptLib

Codaphela TestSculptLib is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela TestSculptLib is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Codaphela TestSculptLib.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "BaseLib/NativeThread.h"
#include "BaseLib/Timer.h"
#include "StandardLib/Objects.h"
#include "SupportLib/MeshBake.h"
#include "SupportLib/MeshEditor.h"
#include "SupportLib/MeshUnwrap.h"
#include "SupportLib/Sphere.h"
#include "ProceduralLib/MeshShapeEditor.h"
#include "ProceduralLib/MeshShapeNormals.h"
#include "ProceduralLib/SolidMaterials.h"
#include "ThreadLib/WorkerThreadJobs.h"
#include "SculptLib/SculptPage.h"
#include "TestLib/Assert.h"


#define TEST_SCULPT_BAKE_MATERIALS	6


//A texture that takes a Ptr to its input at every sample, and so counts on it there: what a texture mustn't do under
//threads that aren't registered with the object system (SupportLib's MeshBake.h), and may under threads that are.
class CTestSculptHolder : public CSolidTexture
{
CONSTRUCTABLE(CTestSculptHolder);
DESTRUCTABLE(CTestSculptHolder);
public:
	Ptr<CSolidTexture>	mpInput;

	void Init(Ptr<CSolidTexture> pInput)
	{
		PreInit();
		CSolidTexture::Init();
		mpInput = pInput;
		PostInit();
	}

	void Class(void) override
	{
		CSolidTexture::Class();
		M_Pointer(mpInput);
	}

protected:
	void TextureSample(SFloat32Vec3* psPosition, SFloat32Vec3* psNormal, float32 fWidth, SSolidSample* psSample) override
	{
		Ptr<CSolidTexture>	pHeld = mpInput;

		pHeld->Sample(psPosition, psNormal, fWidth, psSample);
	}
};


//////////////////////////////////////////////////////////////////////////
//
// The uiIndex'th material and its name.
//
//////////////////////////////////////////////////////////////////////////
static Ptr<CSolidTexture> TestSculptBakeMaterial(size uiIndex, const char** pszName)
{
	static const char*	aszNames[TEST_SCULPT_BAKE_MATERIALS] = { "Marble", "Wood", "Stone", "Rust", "Bricks", "Tiles" };

	*pszName = aszNames[uiIndex];
	switch (uiIndex)
	{
	case 0:		return MakeSolidMarble(1);
	case 1:		return MakeSolidWood(1);
	case 2:		return MakeSolidStone(1);
	case 3:		return MakeSolidRust(1);
	case 4:		return MakeSolidBricks(1);
	default:	return MakeSolidTiles(1);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The milliseconds a sphere a unit in radius takes to be given pTexture
// at fTexelsPerUnit: meshed, unwrapped and baked first with a texture
// (the whole), then baked again with pTexture, the mesh and its UVs as
// they were (the bake alone, which is what a setting changed costs).
// With pcJobs the bake's rows are shared among its threads.  Returns the
// image baked.
//
//////////////////////////////////////////////////////////////////////////
static Ptr<CImage> TestSculptBakeTimes(Ptr<CSolidTexture> pTexture, float32 fTexelsPerUnit, int32 iSamples, CJobs* pcJobs, int64* piWhole, int64* piBake)
{
	Ptr<CMeshEditor>		pEditor;
	Ptr<CMeshShapeEditor>	pShape;
	Ptr<CMeshShapeNormals>	pNormals;
	Ptr<CMeshUnwrap>		pUnwrap;
	Ptr<CMeshBake>			pBake;
	Ptr<CMesh>				pUnwrapped;
	Ptr<CSphere>			pSphere;
	SFloat32Vec3			sCentre;
	int64					iStart;

	sCentre.Init(0, 0, 0);
	pSphere = OMalloc<CSphere>(&sCentre, SCULPT_SPHERE_RADIUS);
	pEditor = OMalloc<CMeshEditor>(OMalloc<CMesh>());
	pShape = pEditor->AddModifier<CMeshShapeEditor>(pSphere, SCULPT_SMALLEST_CELL, SCULPT_LARGEST_CELL);
	pNormals = pEditor->AddModifier<CMeshShapeNormals>(pSphere);
	pUnwrap = pEditor->AddModifier<CMeshUnwrap>(fTexelsPerUnit);
	pBake = pEditor->AddModifier<CMeshBake>(pTexture);
	pBake->miSamples = iSamples;
	pBake->SetJobs(pcJobs);

	iStart = GetDebugTickCount();
	pEditor->GetOutput();
	*piWhole = GetDebugTickCount() - iStart;

	pUnwrapped = pUnwrap->Modify(pNormals->Modify(pShape->Modify(pEditor->mpMesh)));
	iStart = GetDebugTickCount();
	pBake->Modify(pUnwrapped);
	*piBake = GetDebugTickCount() - iStart;
	return pBake->mpImage;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
static bool TestSculptBakeSameImage(Ptr<CImage> pFirst, Ptr<CImage> pSecond)
{
	if ((pFirst->GetWidth() != pSecond->GetWidth()) || (pFirst->GetHeight() != pSecond->GetHeight()))
	{
		return false;
	}
	return memcmp(pFirst->GetData(), pSecond->GetData(), (size_t)pFirst->GetWidth() * (size_t)pFirst->GetHeight() * 3) == 0;
}


//////////////////////////////////////////////////////////////////////////
//
// Every material baked on a pool of threads is the image, byte for byte,
// that it is baked on one.  (Its textures are sampled by several threads
// at once: a ThreadSanitizer build says if any of them isn't only read.)
//
//////////////////////////////////////////////////////////////////////////
void TestSculptBakeJobs(void)
{
	CWorkerThreadJobs	cJobs;
	Ptr<CSolidTexture>	pMaterial;
	Ptr<CImage>			pAlone;
	Ptr<CImage>			pShared;
	const char*			szName;
	int64				iWhole;
	int64				iBake;
	size				uiThreadErrors;
	size				i;

	uiThreadErrors = ObjectsNumThreadErrors();
	cJobs.Init(4);
	for (i = 0; i < TEST_SCULPT_BAKE_MATERIALS; i++)
	{
		pMaterial = TestSculptBakeMaterial(i, &szName);
		pAlone = TestSculptBakeTimes(pMaterial, 16.0f, 2, NULL, &iWhole, &iBake);
		pShared = TestSculptBakeTimes(pMaterial, 16.0f, 2, &cJobs, &iWhole, &iBake);
		AssertTrue(pAlone->GetHeight() > 16);
		AssertTrue(TestSculptBakeSameImage(pAlone, pShared));
	}
	cJobs.Kill();
	AssertSize(uiThreadErrors, ObjectsNumThreadErrors());
}


//////////////////////////////////////////////////////////////////////////
//
// The marble behind a texture that takes a Ptr to it at every sample, held by the root as a material in use is.
//
//////////////////////////////////////////////////////////////////////////
static Ptr<CSolidTexture> TestSculptBakeHeldMarble(void)
{
	Ptr<CTestSculptHolder>	pHolder;

	pHolder = OMalloc<CTestSculptHolder>(MakeSolidMarble(1));
	ORoot()->Add(pHolder);
	return pHolder;
}


//////////////////////////////////////////////////////////////////////////
//
// A pool whose threads are registered with the object system, its locking on (level 2 of CommonLibraries'
// StandardLib/ThreadSafetyPlan.txt), bakes a texture that takes a Ptr at every sample, on every thread at once,
// to the one object: the image is the one baked alone, and no thread is told it did wrong.  Each Ptr is taken
// without the object system's lock, as the object is held by the root.
//
//////////////////////////////////////////////////////////////////////////
void TestSculptBakeRegisteredJobs(void)
{
	CWorkerThreadJobs	cJobs;
	Ptr<CSolidTexture>	pMaterial;
	Ptr<CImage>			pAlone;
	Ptr<CImage>			pShared;
	int64				iWhole;
	int64				iBake;
	size				uiThreadErrors;

	uiThreadErrors = ObjectsNumThreadErrors();
	pMaterial = TestSculptBakeHeldMarble();
	pAlone = TestSculptBakeTimes(pMaterial, 16.0f, 2, NULL, &iWhole, &iBake);

	AssertTrue(ObjectsLocking(true));
	cJobs.Init(4, ObjectsThreadInit, ObjectsThreadKill);
	pShared = TestSculptBakeTimes(pMaterial, 16.0f, 2, &cJobs, &iWhole, &iBake);
	cJobs.Kill();
	AssertSize(0, ObjectsNumThreads());
	AssertTrue(ObjectsLocking(false));

	AssertTrue(TestSculptBakeSameImage(pAlone, pShared));
	AssertSize(uiThreadErrors, ObjectsNumThreadErrors());
	AssertSize(1, pMaterial.NumStackFroms());
	pAlone = NULL;
	pShared = NULL;
	pMaterial = NULL;
	ORoot()->RemoveAll();
}


//////////////////////////////////////////////////////////////////////////
//
// A bake of a unit sphere and pTexture, as a page would describe it.
//
//////////////////////////////////////////////////////////////////////////
static Ptr<CSculptBake> TestSculptBakeDescribed(Ptr<CSolidTexture> pTexture, float32 fTexelsPerUnit)
{
	Ptr<CSphere>	pSphere;
	SFloat32Vec3	sCentre;
	SFloat32Vec3	sLight;

	sCentre.Init(0, 0, 0);
	pSphere = OMalloc<CSphere>(&sCentre, SCULPT_SPHERE_RADIUS);
	sLight.Init(MESH_BAKE_LIGHT_X, MESH_BAKE_LIGHT_Y, MESH_BAKE_LIGHT_Z);
	sLight.Normalize();
	return OMalloc<CSculptBake>(pSphere, pTexture, fTexelsPerUnit, 2, 1.0f, &sLight, false);
}


//////////////////////////////////////////////////////////////////////////
//
// The image a made bake's mesh is textured with.
//
//////////////////////////////////////////////////////////////////////////
static Ptr<CImage> TestSculptBakeImage(Ptr<CSculptBake> pBake)
{
	return pBake->mpMesh->mcMaterials.mapMaterials.Get(0)->GetImage(MI_Diffuse)->mpImage;
}


//////////////////////////////////////////////////////////////////////////
//
// Waits for a baker to have made pBake, or dropped it: false if it does
// neither in ten seconds.
//
//////////////////////////////////////////////////////////////////////////
static bool TestSculptBakeWait(Ptr<CSculptBake> pBake)
{
	int32	i;

	for (i = 0; (i < 10000) && !pBake->IsBaked() && !pBake->IsDropped(); i++)
	{
		ThreadSleep(1);
	}
	return pBake->IsBaked() || pBake->IsDropped();
}


//////////////////////////////////////////////////////////////////////////
//
// A baker's thread makes a bake's mesh in an object system of its own
// (level 3 of CommonLibraries' StandardLib/ThreadSafetyPlan.txt) and the
// main one adopts it: the mesh, its material and its image are main
// objects, the image is the one the bake makes on this thread, the
// baker's own objects are gone, and no thread is told it did wrong.  A
// bake given while another waits takes its place; one given to a baker
// with no thread is dropped at once; and the baker holds what it was
// given, so a bake let go of here is still made.  A bake stopped is
// dropped, with no mesh; the baker stops one at the whole density for
// the bake that follows it.
//
//////////////////////////////////////////////////////////////////////////
void TestSculptBaker(void)
{
	CWorkerThreadJobs	cJobs;
	Ptr<CSolidTexture>	pMaterial;
	Ptr<CSculptBaker>	pBaker;
	Ptr<CSculptBake>	pAlone;
	Ptr<CSculptBake>	pBake;
	Ptr<CSculptBake>	apBakes[3];
	Ptr<CMaterial>		pBaked;
	size				uiThreadErrors;
	int64				iObjects;
	size				i;

	uiThreadErrors = ObjectsNumThreadErrors();
	iObjects = gcObjects.NumMemoryIndexes();
	pMaterial = MakeSolidMarble(1);
	ORoot()->Add(pMaterial);
	pAlone = TestSculptBakeDescribed(pMaterial, 16.0f);
	pAlone->Make(NULL);
	AssertTrue(pAlone->IsBaked());
	AssertTrue(pAlone->mpMesh->NumFaces() > 0);

	//Stopped before it's made, it isn't.
	pBake = TestSculptBakeDescribed(pMaterial, 16.0f);
	pBake->Stop();
	pBake->Make(NULL);
	AssertTrue(pBake->IsDropped());
	AssertTrue(pBake->mpMesh.IsNull());

	//No thread: dropped.
	pBaker = OMalloc<CSculptBaker>(&cJobs);
	ORoot()->Add(pBaker);
	pBake = TestSculptBakeDescribed(pMaterial, 16.0f);
	AssertInt(SBS_Described, pBake->GetState());
	pBaker->Bake(pBake);
	AssertTrue(pBake->IsDropped());
	AssertFalse(pBaker->Start());  //Nor a thread without the locking.

	AssertTrue(ObjectsLocking(true));
	cJobs.Init(4);
	AssertTrue(pBaker->Start());
	AssertFalse(pBaker->Start());

	pBake = TestSculptBakeDescribed(pMaterial, 16.0f);
	pBaker->Bake(pBake);
	AssertTrue(TestSculptBakeWait(pBake));
	AssertTrue(pBake->IsBaked());
	AssertTrue(pBake->mpMesh->GetObjectsThisIn() == &gcObjects);
	pBaked = pBake->mpMesh->mcMaterials.mapMaterials.Get(0);
	AssertTrue(pBaked->GetObjectsThisIn() == &gcObjects);
	AssertTrue(TestSculptBakeImage(pBake)->GetObjectsThisIn() == &gcObjects);
	AssertInt(pAlone->mpMesh->NumFaces(), pBake->mpMesh->NumFaces());
	AssertTrue(TestSculptBakeSameImage(TestSculptBakeImage(pAlone), TestSculptBakeImage(pBake)));
	pBaked = NULL;

	//Three at once: the last is made, and each of the others is made, or was dropped or stopped for the one after it.
	for (i = 0; i < 3; i++)
	{
		apBakes[i] = TestSculptBakeDescribed(pMaterial, 32.0f);
		pBaker->Bake(apBakes[i]);
	}
	for (i = 0; i < 3; i++)
	{
		AssertTrue(TestSculptBakeWait(apBakes[i]));
	}
	AssertTrue(apBakes[2]->IsBaked());
	AssertTrue(apBakes[2]->mpMesh->NumFaces() > 0);
	AssertTrue(apBakes[0]->IsDropped() == apBakes[0]->mpMesh.IsNull());
	AssertTrue(apBakes[1]->IsDropped() == apBakes[1]->mpMesh.IsNull());
	for (i = 0; i < 3; i++)
	{
		apBakes[i] = NULL;
	}

	//Let go of here, it is the baker's to make or to drop: Stop waits for it.
	pBake = TestSculptBakeDescribed(pMaterial, 16.0f);
	pBaker->Bake(pBake);
	pBake = NULL;

	pBaker->Stop();
	cJobs.Kill();
	AssertFalse(pBaker->IsStarted());
	AssertSize(0, ObjectsNumThreads());
	ObjectsCollect();
	AssertTrue(ObjectsLocking(false));
	AssertSize(uiThreadErrors, ObjectsNumThreadErrors());

	//Nothing is left of the bakes, here or the baker's.
	pAlone = NULL;
	pBaker = NULL;
	pMaterial = NULL;
	ORoot()->RemoveAll();
	AssertLong(iObjects, gcObjects.NumMemoryIndexes());
}


//////////////////////////////////////////////////////////////////////////
//
// SCULPT_BAKE_REPORT prints what a sphere takes to bake with each
// material at the densities a change could be shown at, on this thread
// and then on a thread for each the hardware runs at once.  Without it,
// one small bake says the stack works.
//
//////////////////////////////////////////////////////////////////////////
void TestSculptBakeReport(void)
{
	float32				afDensities[4] = { 128.0f, 64.0f, 32.0f, 16.0f };
	CWorkerThreadJobs	cJobs;
	CJobs*				pcJobs;
	const char*			szName;
	Ptr<CSolidTexture>	pMaterial;
	int64				iWhole;
	int64				iBake;
	size				i;
	size				j;
	size				k;

	if (getenv("SCULPT_BAKE_REPORT") == NULL)
	{
		TestSculptBakeTimes(MakeSolidMarble(1), 8.0f, 1, NULL, &iWhole, &iBake);
		AssertTrue(iWhole >= 0);
		return;
	}

	cJobs.Init();
	for (k = 0; k < 2; k++)
	{
		pcJobs = (k == 0) ? NULL : &cJobs;
		printf("A sphere of radius %.1f on %d thread%s: milliseconds to mesh, unwrap and bake (whole), and to bake alone (bake)\n", SCULPT_SPHERE_RADIUS, (k == 0) ? 1 : cJobs.NumThreads(), (k == 0) ? "" : "s");
		printf("%-8s", "");
		for (j = 0; j < 4; j++)
		{
			printf("  %3.0f/unit whole  bake", afDensities[j]);
		}
		printf("\n");
		for (i = 0; i < TEST_SCULPT_BAKE_MATERIALS; i++)
		{
			pMaterial = TestSculptBakeMaterial(i, &szName);
			printf("%-8s", szName);
			for (j = 0; j < 4; j++)
			{
				TestSculptBakeTimes(pMaterial, afDensities[j], MESH_BAKE_SAMPLES, pcJobs, &iWhole, &iBake);
				printf("  %14d %5d", (int)iWhole, (int)iBake);
			}
			printf("\n");
		}
	}
	cJobs.Kill();

	//And the marble behind a texture that takes a Ptr at every sample: alone, then on threads registered with the
	//object system with its locking on.
	pMaterial = TestSculptBakeHeldMarble();
	for (k = 0; k < 2; k++)
	{
		if (k == 1)
		{
			ObjectsLocking(true);
			cJobs.Init(NumHardwareThreads(), ObjectsThreadInit, ObjectsThreadKill);
		}
		printf("%-8s", (k == 0) ? "Ptr, 1" : "Ptr, all");
		for (j = 0; j < 4; j++)
		{
			TestSculptBakeTimes(pMaterial, afDensities[j], MESH_BAKE_SAMPLES, (k == 0) ? NULL : &cJobs, &iWhole, &iBake);
			printf("  %14d %5d", (int)iWhole, (int)iBake);
		}
		printf("\n");
	}
	cJobs.Kill();
	ObjectsLocking(false);
	pMaterial = NULL;
	ORoot()->RemoveAll();
	fflush(stdout);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void TestSculptBake(void)
{
	BeginTests();
	ObjectsInit();

	TestSculptBakeReport();
	TestSculptBakeJobs();
	TestSculptBakeRegisteredJobs();
	TestSculptBaker();

	ObjectsDiscard();
	ObjectsKill(false);
	TestStatistics();
}

