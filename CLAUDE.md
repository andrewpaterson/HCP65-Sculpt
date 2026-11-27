# HCP65-Sculpt

A program for looking at and changing procedural materials on a shape: a 3D view over a node editor, as
Blender's Shading workspace.  It needs CommonLibraries checked out beside it (`../CommonLibraries`).

## Layout

- Design pages: `Libraries/SculptLib/Docs/sculpt` (`index.md`: what it is and how to build, run and see it;
  `design.md`: how it works, its speed, decisions and what isn't done); the website gathers them.  Read them first.
- A design page's main classes name it in their headers, one line above the class, `//Design page: Docs/...`
  (`DocsPlan.txt` G1); a new design page adds it to its own.
- `SculptPlan.txt`: the plan, still being built from; its "Questions for the user" are kept as they come up, and work
  carries on with the answer assumed in brackets.
- `HCP65Sculpt/`: the app.  `Libraries/SculptLib`: its library.  `Tests/TestSculptLib`.
- The texture generators and isosurfaces it shows are CommonLibraries' ProceduralLib
  (`Libraries/ProceduralLib/TexturePlan.txt`, `Isosurface.txt`); shapes and drawing are SupportLib.
- Where Blender has an unambiguous answer, Sculpt does what Blender does (navigation, the node editor, shortcuts,
  widgets, files), without asking, recorded in the plan as "taken from Blender".  Ask only where Blender has no
  answer or Sculpt genuinely differs (materials are C++ code, not files).  Explain Blender's terms plainly; the user
  knows 3ds Max, not Blender.

## Building and testing

- `cmake -S HCP65Sculpt -B ../build/HCP65Sculpt -DCMAKE_BUILD_TYPE=Release`, then `cmake --build`
  (`RelWithDebInfo` in `../build/HCP65SculptRelDbg` for profiling).
- Tests build with CMake as in CommonLibraries (`cmake -S Tests/TestSculptLib -B ../build/TestSculptLib64
  -DBASELIB_64=ON`, then `cmake --build` and `ctest --test-dir`).

## Recipes

- Profiling: `perf` and attaching gdb don't work on this machine (`perf_event_paranoid` 4, `ptrace_scope` 1).  Sample
  with an `LD_PRELOAD` library instead: a `SIGALRM` interval timer whose handler writes `backtrace()` to a file,
  symbolised with `addr2line` against the base address in `/proc/<pid>/maps`.  The app sometimes fails to start
  under it: try again.
- Real input: a Xephyr window is allowed for Sculpt (the user, 2026-11-17), driven as HCP65-IDE's `CLAUDE.md` says;
  space XTest events at least about 120 ms apart or they are lost.  If the user closes the window, don't reopen it.
- A private copy of a test folder links its inputs file by file: never symlink a repository folder into it (a
  runner once followed the link into the real repository).

## Working in these repositories

<!-- This section is the same in every Codaphela repository's CLAUDE.md: change them together. -->

- Several Claude sessions work in these checkouts at once, on the main branch (no private branches or worktrees).
  Run `git status` before touching a file and leave other sessions' uncommitted work alone.  Stage and commit only
  your own files, by path: `git add <paths>` and `git commit -m "..." -- <paths>`; never `git add -u`/`-A` or
  `commit -a`, and not `repos.py commit` (it stages everything).  Check `git show --stat HEAD` after.
- Commit messages have no Co-Authored-By line.  Make commits logical: one change each, its tests with it.
- Bring work from a copy into the main checkout with a 3-way merge (`git merge-file`, or `diff -u base mine |
  patch`), never by copying files over it: another session may have changed the target since.
- Before building a step of a shared plan, `git fetch` and read `git log HEAD..origin/<branch>` for the plan's
  files: a session on another machine may have done it.
- The shared build folders (`../build/*`) can hold binaries linked from another session's half-finished edits.  A
  failure seen only there: build the same sources in a private build folder; if that passes, rebuild the shared one
  clean.  Tests rewrite `Output/` in their project folder: run from a copy of that folder when another session may
  be running the same suite.
- Build, run and measure in Release x64 by default; Debug for full test sweeps or chasing a problem.  Say which build
  a number came from.  A speed claim that feeds a decision needs a like-for-like A/B (same commit but the change, in
  the whole test program) judged by instruction counts (callgrind) with the clock beside them.
- A decision the user states while planning settles that question only: plan with them, and wait to be told to
  build.
- Put decisions to the user one at a time, each explained plainly (what changes, what it costs), with the
  recommended option first, especially outside their speciality.
- Show images as full `file://` paths to the real files; don't make enlarged copies.
- Every tracked file says what it is (`HCP65/Website/tools/check_files.py`, in the website's build): an item of its
  project (plans and notes too), test data, or a row of the `README.md` of a folder above it, whose table
  (`| Files | Are | From |`) says source, generated (with the command and its inputs) or third-party (with where from
  and which version).
- Code files are LF.  `.sln`, `.vcxproj` and `.filters` are CRLF with a BOM: edit them as bytes and check `git diff
  --stat` shows only your lines.  Test `Input` files are kept byte for byte.
