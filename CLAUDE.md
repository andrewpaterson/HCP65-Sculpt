# HCP65-Sculpt

A program for looking at and changing procedural materials on a shape: a 3D view over a node editor, as
Blender's Shading workspace.  It needs CommonLibraries checked out beside it (`../CommonLibraries`).

## Layout

- `SculptPlan.txt`: the plan, its decisions and what is built.  Start there.
- `HCP65Sculpt/`: the app.  `Libraries/SculptLib`: its library.  `Tests/TestSculptLib`.
- The texture generators and isosurfaces it shows are CommonLibraries' ProceduralLib
  (`Libraries/ProceduralLib/TexturePlan.txt`, `Isosurface.txt`); shapes and drawing are SupportLib.
- Where Blender has an unambiguous answer, Sculpt does what Blender does.

## Building and testing

- `cmake -S HCP65Sculpt -B ../build/HCP65Sculpt -DCMAKE_BUILD_TYPE=Release`, then `cmake --build`
  (`RelWithDebInfo` in `../build/HCP65SculptRelDbg` for profiling).
- Tests build with CMake as in CommonLibraries (`cmake -S Tests/TestSculptLib -B ../build/TestSculptLib64
  -DBASELIB_64=ON`, then `cmake --build` and `ctest --test-dir`).
