# HCP65-Sculpt

HCP65-Sculpt is a program for looking at procedural materials on a shape and changing them: a 3D view over a node
editor, as Blender's Shading workspace. A material is ProceduralLib's graph of solid textures; Sculpt shows it as
nodes, bakes it onto a shape meshed as an isosurface, and bakes it again as each number is dragged. It is a
WindowLib application, in its own repository beside CommonLibraries.

It began as the isosurface tab of WindowInputTester (spheres dropped and baked with a checker) and as "a textures
tab" at the end of ProceduralLib's texture generators. On 2026-11-17 the user made it a program of its own; the tab
left WindowInputTester, which no longer links ProceduralLib, and the view both draw with went to a new
CommonLibraries library, SceneViewLib. Where Blender has an unambiguous answer (navigation, the node editor,
shortcuts, files), Sculpt does what Blender does.

| Page | Covers |
|---|---|
| [Sculpt Design](design.md) | the page, the graph and its nodes, undo, files, baking on threads and the baker, speed, the plan's steps, decisions, what isn't done |

## Its parts

| Part | Where | Is |
|---|---|---|
| The app | `HCP65Sculpt/HCP65Sculpt` | [`CSculptApp`]: the window, its menu bar (File, Edit, Add) over the page, the thread pool and the baker; Windows', X11's and the reference screen's mains |
| SculptLib | `Libraries/SculptLib` | the page ([`CSculptPage`]) and everything on it: the graph ([`CSculptGraph`]), its nodes ([`CSculptNodes`]), the panel ([`CSculptPanel`]), the bake ([`CSculptBake`], [`CSculptBaker`]), files ([`CSculptDocument`]) |
| TestSculptLib | `Tests/TestSculptLib` | the graph, the nodes, the bake and the page, in windows drawn into memory with no screen |

The page is SculptLib's rather than the app's so that it is tested without the app: SculptTestWindow
([`TestSculptLib/SculptTestWindow.h`]) makes a window on WinRefLib's reference screen with Sculpt's font and theme.

Made in CommonLibraries for Sculpt, and used by others since:

| Library | What |
|---|---|
| SceneViewLib | [`CSceneView`] (a canvas with the view's mouse and keys), [`CSceneCanvasDraw`], the turntable [`STurntable`] |
| WindowLib | [`CNodeEditor`] and [`CNodeBox`], [`CNumberField`], [`CColourField`] and [`CColourPicker`], [`CColourRampBar`], [`CCelCanvasDraw`] |
| ProceduralLib | [`CSolidChannel`] and [`CSolidCombine`] (Blender's Separate and Combine), materials as files ([`WriteSolidMaterial`], [`ReadSolidMaterial`]) |
| SupportLib | [`CSolidTexture::Changed`], [`ReadTrueTypeFont`], [`VectorLevelCubic`] (the link's curve, shared with the diff divider), [`ColourRGBToHSV`] |
| StandardLib | [`WriteNamedObjectFile`] and [`ReadNamedObjectFile`] |

## What it is built on

What is looked at is a stack of mesh modifiers ([`CSculptBake::MakeEditor`]): ProceduralLib's [`CMeshShapeEditor`]
meshes a [`CShape`] as an isosurface and [`CMeshShapeNormals`] gives it the shape's normals
([Isosurfaces](../libraries/procedurallib/isosurface.md)); SupportLib's [`CMeshUnwrap`] unwraps it and
[`CMeshBake`] bakes the material into its texels ([Unwrapping and Virtual
Textures](../libraries/supportlib/unwrapping.md)). The shapes are SupportLib's
([Shapes](../libraries/supportlib/shapes.md)): a sphere, a box, the two met, or the spheres dropped by clicking,
joined by a smooth union ([`CShapeSmoothUnion`]).

The material is ProceduralLib's ([Texture Generators](../libraries/procedurallib/textures.md)): the generators and
operators are the nodes, File's New starts from one of its materials ([`MakeSolidMarble`] and the rest), File's
Weather is [`MakeSolidWeathered`], and Export Material writes the file another program reads with
[`ReadSolidMaterial`].

The view is SceneViewLib's, drawn by Vulkan or the rasteriser
([Scenes](../libraries/supportlib/scenes.md#viewing-a-scene)). Bakes are made on a thread with an object system of
its own and on a pool of threads ([Objects and Threads](../libraries/standardlib/threads.md#in-the-applications)).

## Building, running and seeing it

It needs CommonLibraries checked out beside it. From the folder both are in:

```text
cmake -S HCP65-Sculpt/HCP65Sculpt -B build/HCP65Sculpt -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/HCP65Sculpt
cd HCP65-Sculpt/HCP65Sculpt/HCP65Sculpt && ../../../build/HCP65Sculpt/HCP65Sculpt
```

It runs from its sources' folder, where its font (`Resources/InterVariable.ttf`) and WinInputLib's device files are
found. A file given as the first argument is opened: a Sculpt file or a material's. Without one it opens showing
marble on a sphere. The build is 64-bit only on Linux and needs Vulkan's headers; Visual Studio's solution is
`HCP65Sculpt/HCP65Sculpt.sln`.

| Set in the environment | Does |
|---|---|
| `HCP65_SCULPT_SNAPSHOT=file` | saves the window as a PNG once it has been drawn |
| `HCP65_SCULPT_EXIT` | with the snapshot, closes the window after saving it |
| `HCP65_SCULPT_NO_BAKER` | bakes on the window's thread, in its frames, with the objects' locking off |

HCP65SculptRef (built beside HCP65Sculpt) draws the same window on WinRefLib's reference screen, in memory: with
`HCP65_SCULPT_SNAPSHOT` and `HCP65_SCULPT_EXIT` it is how Sculpt is looked at with no screen. Nothing can press a
key or move the pointer in it.

TestSculptLib builds with CMake as CommonLibraries' tests do. What its tests read from the environment:

| Set in the environment | Does |
|---|---|
| `SCULPT_PAGE_PICTURE=file` | saves the page test's window as a PNG |
| `SCULPT_PAGE_FILE=file` | saves the file test's page as a Sculpt file |
| `SCULPT_MATERIAL_FILES=folder` | saves a Sculpt file of each starting material on the sphere and on the box, to open in Sculpt |
| `SCULPT_BAKE_REPORT` | prints every material's bake times on one thread and on the pool, and the page's longest frame with and without the baker |

The node editor on its own is in TestWindowLib (`NODE_EDITOR_PICTURE=file` saves its window).
