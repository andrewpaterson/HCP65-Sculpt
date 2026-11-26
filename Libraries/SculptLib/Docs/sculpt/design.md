# Sculpt Design

Sculpt is one page ([`CSculptPage`]) in a window with a menu bar ([`CSculptApp`]). The page shows a material, a
graph of ProceduralLib's solid textures, three ways at once: baked onto a shape in a 3D view, as nodes in a node
editor under it, and through a panel of what isn't the material's beside them. A change in any of them bakes the
material again.

## The page

The page is laid out as Blender's Shading workspace: the view above and the node editor below, a sash between them
(the view has 550 of 1000 of the height to start), and to their right the panel, [`CSculptPanel`]. A line over the
view says what is drawn, and a line of help.

The view is SceneViewLib's [`CSceneView`] with Blender's buttons: the middle button's drag turns it, with Shift
pans, the wheel zooms, Home puts it back, R switches between Vulkan and the rasteriser. The left button is for what
is being worked on, not for the view: a click drops a sphere a unit in radius where the pointer is, on the plane of
the screen through what the view turns about ([`CSculptPage::Clicked`]). The spheres dropped are joined by a smooth
union ([`CShapeSmoothUnion`], blending half a unit), and Delete or X takes them all away.

The panel chooses what the material is looked at on, what of it is shown and how it is baked:

| Setting | Choices |
|---|---|
| Shape | a sphere (the start), a box, the two met (the box turned so its sides aren't square to the texture's axes), or the spheres dropped; clicks drop nothing on the others |
| Show | the bake (colour shaded by its height's relief), the colour alone (no relief), or the height, shine, gloss or glow as greys |
| Bake | the most samples each way in a texel, how strongly the height shades it, and the light's direction |

A channel is shown by baking a [`CSolidChannel`] of the material; the colour alone is the material baked with no
relief. The materials are made with z up (bricks' courses, wood's grain, moss on what faces up) and the view turns
about y, so the shape is stood up in the view: its z is the view's up and its -y faces the view at the start. A
box's top and bottom are kept off the tenths of a unit, where bricks' courses meet, as a face lying in the mortar
between two courses is all mortar.

## The graph

[`CSculptGraph`] is a material as nodes: a [`CSculptNode`] for the output (no texture; its one input is what the
material is) and one for each texture, each with where its box is in the editor. The textures are the material; the
graph is how they are looked at. ProceduralLib's [`CSolidTexture`] knows nothing of editors, so where a node is
lives in Sculpt.

A texture's inputs are found from its Class(): its texture pointers, less any SolidShow's table hides. A link from
a node to an input is that input pointing at the node's texture. An input is never NULL: with no link it points at
a flat texture of its own ([`CSolidFlat`]), which is the input's value and not a node ([`CSculptGraph::IsValue`]).
So a saved material is nothing but textures, and a program reading one needn't know of values. When a graph is
made, inputs that point at nothing, or share one flat, are each given their own.

A node can't be linked to an input of itself or of anything it is made from, however far down, and the output feeds
nothing. Nodes not linked through to the output are kept and do nothing, as Blender's.

[`CSculptGraph::Copy`] is another graph of copied textures, linked as this one's, its nodes in the same order and
places: what undo keeps. Textures are copied by [`SolidCopy`]: the class's new texture given the old one's plain
fields, its inputs and, of a ramp, its keys. A cavity ([`CSolidCavity`]) can't be added from a menu (its shape is
one a node can't give it) but is copied with its shape, so Duplicate copies a Cavity node and Undo puts back its
reach.

## The nodes

[`CSculptNodes`] shows the graph in WindowLib's [`CNodeEditor`] and is the editor's [`CNodeEditorListener`]: a link
made, taken off or refused, and nodes moved, deleted, copied or added, are the graph's. The editor holds its boxes
and links itself, as a tree holds its rows, and knows nothing of textures.

A node's rows come from its texture's Class(), so a new operator needs nothing written in Sculpt. Each field a node
can show is a [`CSculptField`], whose component is a number field for a number, three for a vector, a tick for a
bool and a combo box for an enum ([`CSculptField::Kind`]). A setting changed is put in the texture, the texture is
told ([`CSolidTexture::Changed`], which remakes a placement's axes, a slope's direction and a ramp's key order) and
the material is baked. What Class() doesn't say is SolidShow's table ([`SculptLib/SolidShow.h`]), by class and
field:

| Table | Says |
|---|---|
| [`SSolidNodeShow`] | a class's title (Blender's where it has the same thing: Noise Texture, Voronoi Texture, Gradient Texture, Brick Texture, Color Ramp, Mix, Hue/Saturation/Value, Separate, Combine), its kind (Texture, Color, Converter, Vector) and how a new one is made |
| [`SSolidFieldShow`] | a field's label, whether it is hidden, a number's range, an enum's names, and which inputs are a number from 0 to 1 |

An input with nothing linked shows its own value beside its socket ([`CSculptValue`]): a colour field, or for a
factor (a ramp's, a mask's, a warp's) a number from 0 to 1, which is a grey. A ramp's keys are a colour ramp bar
([`CSculptRamp`] over [`CColourRampBar`]): a stop for each key, and under the bar the chosen key's height, shine,
gloss and glow, which a colour doesn't say. Each key keeps the rest of its sample wherever its stop is dragged, and
a stop added is the ramp's whole sample there.

Nodes are added from an Add menu ([`CSculptNodes::MakeAddMenu`]): the menu bar's, and the editor's own at Shift+A
or the right button, under the pointer. X or Delete removes the selected nodes (never the output), each input they
fed having its own value again; Shift+D copies them. After either the editor's boxes are made again from the graph
([`CSculptNodes::Rebuild`]), keeping where they were. Adding a node bakes nothing: the material is the same until
it is linked in. Nodes not yet placed are arranged in columns by their distance from the output, the output on the
right ([`CSculptNodes::Arrange`]).

Every socket is one colour: Blender's link carries one thing and a texture node has a socket for each, but ours
carries a whole sample (colour, height, shine, gloss, glow) and an operator reads the lot. Taking one channel out,
or putting channels from several textures together, is [`CSolidChannel`] and [`CSolidCombine`].

## Undo

Undo keeps the graph, not the changes: [`CSculptNodes`] keeps a copy ([`CSculptGraph::Copy`]) after every change,
the first the graph as given. Undo shows a copy of the one before, so the graph is then another object; redo the
one after; a change after an undo forgets what was undone. Settings changed within half a second of each other are
one step, so a drag is one. Control+Z and Control+Shift+Z in the editor, and the Edit menu. Dropping and clearing
spheres and opening a file aren't undone.

With a graph kept for every change there are too many objects to validate them all at each change, so the tests'
window turns the objects' validation off while it is there and validates once at the end. The app does the same
while it runs (its own reason is the text's glyphs).

## Files

A Sculpt file is a [`CSculptDocument`]: the graph with its nodes where they were, the spheres dropped, and the
panel's settings, written as the named object of a chunk file of objects ([`WriteNamedObjectFile`]; DRG, as every
object file). Not kept: where the view is turned to, and the history for undo. File's Open, Save, Save As and
Export Material each ask for a file from the window's tick, not from the click, as a dialog reads the input itself.

Export Material ([`CSculptPage::ExportMaterial`]) writes the material alone with ProceduralLib's
[`WriteSolidMaterial`], which another program reads back with one call; nodes not linked through to the output
aren't in it. Open ([`CSculptPage::Open`]) reads either kind of file. File's New is a menu of ProceduralLib's six
materials ([`MakeSculptMaterial`]); Weather ([`CSculptPage::WeatherMaterial`]) weathers the material there is, on
the shape shown, giving it Cavity nodes of that shape.

## Baking

What the view shows is a mesh editor whose stack meshes the shape, gives it the shape's normals, unwraps it and
bakes the material onto it ([`CSculptBake::MakeEditor`]). A [`CSculptBake`] describes one bake (the shape, the
texture, the density, the samples, the relief, the light) and gives a stack its settings ([`CSculptBake::Apply`]).

A change of the shape, the material or the panel is baked at once at a quarter of the density (32 texels a unit, a
sixteenth of the texels) with one sample a texel, and at the whole density (128 texels a unit, the panel's samples)
once nothing has changed for 330 ms ([`CSculptPage::Rebake`], posted with [`CWindow::PostAfter`]; a new change
cancels the post). The status line says "coarse" while the coarse bake is shown.

Two kinds of threads make a bake, and both are the app's:

- **The pool.** A bake's rows are shared among a [`CWorkerThreadJobs`], one thread for each the hardware runs at
  once ([`CSculptPage::SetJobs`], [`CMeshBake::SetJobs`]). The image is the same byte for byte. Its threads aren't
  registered with the object system, so a texture's [`CSolidTexture::TextureSample`] and a shape's
  [`CShape::Distance`] must not copy, assign or let go of a [`Ptr`]: a Debug build reports one that does, and
  TestSculptBakeJobs bakes every starting material that way. How this is safe is on [Objects and
  Threads](../libraries/standardlib/threads.md#using-objects-from-threads).
- **The baker.** [`CSculptBaker`] is a thread with an object system of its own ([`ObjectsWorkerInit()`]) that
  makes each bake's mesh ([`CSculptBake::Make`]) behind the window. The page gives it the bake with a copy of the
  graph, so the page may go on changing its own, and leaves the view showing the last mesh. The baker's thread
  makes the mesh and cooks it as the renderer wants it ([`CCookedMesh`]); both are adopted by the main object
  system ([`ObjectsAdopt`]) before the bake is said to be baked. The page looks every 2 ms
  ([`CSculptPage::ShowBaked`]), makes the newest mesh made its stack's output ([`CMeshEditor::SetOutput`]) and
  gives the view the cooked mesh ([`CSceneCanvasDraw::SetCooked`]); with Vulkan only the copy to the graphics card
  is left to the window's thread.

The baker makes one bake at a time and keeps one waiting ([`CSculptBaker::Bake`]). A bake given while another waits
takes its place and that one is dropped, as it is already out of date. One at the whole density being made is
stopped ([`CSculptBake::Stop`], through [`CMeshBake::SetStop`]); a coarse one is finished, so something shows while
changes keep coming. A bake's state ([`ESculptBakeState`]) and its stop are the only things both threads write.

The objects' locking ([`ObjectsLocking`]) is on while the app runs with a baker. [`CSculptBaker::Stop`] waits for
the thread and must not be called from a Free, as the thread may be waiting for the objects' lock a Free holds; the
app stops it when the window closes. `HCP65_SCULPT_NO_BAKER` turns the baker and the locking off: the page's stack
then bakes when the view next asks for its mesh, in the window's frame.

## Speed

Measured by TestSculptLib with `SCULPT_BAKE_REPORT` (Release x64, 2026-11-17, before the pool): a sphere of radius
1 meshed, unwrapped and baked on one thread, in milliseconds, at 128, 64, 32 and 16 texels a unit:

| Material | 128 | 64 | 32 | 16 |
|---|---|---|---|---|
| Marble | 627 | 161 | 43 | 12 |
| Wood | 579 | 147 | 39 | 11 |
| Stone | 946 | 247 | 64 | 18 |
| Rust | 412 | 105 | 29 | 9 |
| Bricks | 276 | 69 | 17 | 6 |
| Tiles | 264 | 68 | 18 | 6 |

All but a few milliseconds of each is the bake: meshing and unwrapping cost next to nothing, so nothing is gained
by keeping the mesh between bakes. A quarter of the density is a sixteenth of the time, 17 to 64 ms, which follows
a drag: so the coarse density and the pause above.

On 2026-11-17 the user found Sculpt "extremely slow" in use. Measured in the running app (the starting marble, a
1080 by 400 view, a GTX 1050 Ti, 24 hardware threads; the build not recorded), and what was done:

| Found | Done | After |
|---|---|---|
| The node editor repainted its whole back at every layout of the window: half the time of turning the view | repaints only when a link's ends or its size change ([`CNodeEditor::GetPaintedState`]) | turning the view about 10 ms a frame |
| Vulkan's frame was read back from write-combined memory: 8.4 ms | host-cached memory | 0.4 ms |
| A coarse bake took the panel's samples | one sample a texel | |
| Every change baked coarsely on the window's thread, about 100 ms (9 changes a second while dragging), and the whole bake held the window for about 1.7 s | the bake's rows on the pool (with the machine busy with other work) | coarse 124 to 13 ms, whole 2.3 s to 0.18 s |

Left then, the whole bake still held the window for under a fifth of a second. On 2026-11-19 the baker took every
bake off the window's thread: the window's longest frame while marble is changed and baked went from about 70 ms to
13 ms (TestSculptLib's page test with `SCULPT_BAKE_REPORT`), the 13 ms being the new mesh cooked and given to the
renderer. Cooking on the baker's thread followed; a sphere's mesh is too small for it to show, and the longest
frame stayed 14 to 16 ms.

The status line's "ms a frame" is a smoothed average ([`CSceneCanvasDraw`], 0.9 of the last plus 0.1 of this frame)
with the bakes' frames in it, so it reads high and isn't the time to turn the view.

## The plan's steps

SculptPlan.txt's 24 steps were built on 2026-11-17, each with a note of where it differs from what was proposed;
the baker and cooking on its thread followed on 2026-11-19.

| Steps | Built |
|---|---|
| 1-4: the project | SceneViewLib ([`STurntable`], [`CSceneCanvasDraw`], [`CSceneView`]) and TestSceneViewLib; the app with three mains; the isosurface page moved to SculptLib as [`CSculptPage`]; the tab out of WindowInputTester |
| 5-7: the node editor | [`CNumberField`]; [`CNodeEditor`] and [`CNodeBox`], links, selecting, moving, panning, Home. No zoom |
| 8-9: unlinked inputs, Changed | every operator already gave something with an input missing (TestSolidOperatorsUnlinked holds them to it); an input's value is a [`CSolidFlat`]; [`CSolidTexture::Changed`] |
| 10-11: the graph in Sculpt | [`CSculptGraph`], [`CSculptNodes`], SolidShow's table, [`CSculptField`] |
| 12: bake times | coarse at once, whole after 330 ms (above) |
| 13-15: enums, colours, ramps | combo boxes; [`CColourField`] and [`CColourPicker`], [`CSculptValue`]; [`CColourRampBar`], [`CSculptRamp`] |
| 16-18: add, remove, copy; Separate and Combine; undo | the Add menu, [`SolidCopy`]; [`CSolidChannel`], [`CSolidCombine`]; graph copies |
| 19: the panel | shape, show and bake settings. Not the normals |
| 20-22: materials and files | File's New and Weather; [`CSculptDocument`]; [`WriteSolidMaterial`] and [`ReadSolidMaterial`] |
| 23: tune the materials | not done as written: each was looked at in Sculpt, which found the view looking from above and the all-mortar box, both mended; the numbers are unchanged |
| 24: clean up | done |

## Decisions

- **A program of its own** (the user, 2026-11-17). Looking at materials isn't a tab in WindowInputTester: it is
  HCP65-Sculpt, its own repository, with WindowInputTester as its inspiration; the isosurface tab comes out of the
  tester. The view both draw with goes to SceneViewLib, its turntable a plain struct so it is tested without a
  window. Two repositories can't share a source file, so what both want went to CommonLibraries and the rest (the
  app's start, about 500 lines) was written again.
- **Blender where it is unambiguous** (the user, 2026-11-17). Its layout, the view's buttons, nodes, links,
  unlinked inputs as fields, Shift+A, X, Shift+D, number fields, the colour ramp, the picker, undo keys, node
  titles and one file are all taken from Blender without asking; only where Blender has no answer is the user
  asked. Blender is followed as the more used now; the user knows 3D Studio Max.
- **A material is a configuration, not code** (the user, 2026-11-17). No C++ is written out to paste in: a
  material is data saying what to call with what settings, per node (the graph itself, each node naming its
  operator and holding its settings), which may be written to disk. ProceduralLib's material functions stay as a
  help to programmers.
- **Baked, not drawn live** (2026-11-17). Blender works a procedural texture out per pixel on the graphics card;
  ours is baked into the mesh's texels on the processor, coarse at once and whole after a pause, from the step 12
  times.
- **An input's value is a flat texture** (2026-11-17, step 8). Not a new field on every operator: a saved material
  is only textures.
- **Settings from Class()** (2026-11-17). A node's rows are made from its texture's Class(); only what Class()
  can't say is in SolidShow's table, so a new operator needs nothing in Sculpt.
- **The node editor holds its nodes** (2026-11-17, step 7). Not an interface asked for what nodes there are:
  whoever owns them adds boxes and links and is told of changes through a listener; a node's settings are real
  components.
- **Undo keeps copies of the graph** (2026-11-17, step 18), and settings changed within half a second are one
  step.
- **No zoom in the node editor for now** (the user, 2026-11-17). A node's settings are WindowLib components, drawn
  at one size; zoom would mean fonts at several sizes and every component's measurements scaled. Pan and Home only;
  not to be offered again unless a material doesn't fit.
- **No reader for enums saved at the old size** (the user, 2026-11-17). Step 13 found StandardLib registering an
  enum field as 16 bytes where it is 4, so saving read and loading wrote 12 bytes past it; it passes the enum's
  size now. Objects with enums saved before don't load; the user has probably none.
- **The materials are tuned by the user** (the user, 2026-11-17: "I'll try"). Tuning is by eye on a screen.
- **Trying it by hand in Xephyr** (the user, 2026-11-17). It found two faults: a real Control+Z arrives only as
  the window's undo command ([`CWindow::InputCommand`]), never as a Z key, so undo is bound to the command, and a
  test calling the action itself hides this; and Vulkan lost the device on a whole bake whose size wasn't a power
  of two, as a GPU halves mip levels rounding down and ours rounded up ([`CTextureLevels::Init`]'s round down).
- **The bake on threads** (the user, 2026-11-17, by asking for the thread safety plan's step 17). The cost is
  threads in SupportLib's bake with no [`Ptr`] copied on them.
- **The baker on by default** (the user, 2026-11-19), with the objects' locking on while the app runs (a pointer's
  copy on the window's thread about 25 ns against 12); `HCP65_SCULPT_NO_BAKER` turns both off.
- **A cavity is copied with its graph** (the user, 2026-11-19), so Duplicate copies a Cavity node and Undo puts
  back its reach.
- **The page polls for a finished bake every 2 ms** (the user, 2026-11-19: "fine as it is"). WindowLib has no way
  for another thread to wake a window, and none is to be built for this.
- **The baker's thread cooks the mesh** (the user, 2026-11-19), as Sculpt's scenes will get larger.
- **Not in it** (2026-11-17): groups of nodes, frames and reroutes, a colour wheel, muting and collapsing nodes,
  and any sculpting beyond dropping spheres.

## Not done

- The status line's "ms a frame" is a smoothed average with the bakes' frames in it: show the last frame's time,
  or leave the bakes out.
- The materials aren't tuned: the user's to try in Sculpt. What they send or name becomes the starting materials
  in ProceduralLib's SolidMaterials.cpp, and the gallery is made again.
- The normals aren't shown. The bake can make a normal image, but neither renderer draws one, and showing it as
  colours means baking normals where the colours go, which the bake can't be asked for.
- A Sculpt file doesn't keep the view; opening a file and dropping or clearing spheres aren't undone.
- Not yet tried by hand: Shift held with a click (a test has no keyboard to hold it), and the app's file choosers
  (Open, Save, Save As, Export Material).
- Not built with Visual Studio: HCP65Sculpt.sln, TestSculptLib.sln and TestSceneViewLib.sln are there, made from
  the existing ones.
- The node editor doesn't zoom (decided above, for now).
