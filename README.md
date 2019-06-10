Star Trek: Klingon Honor Guard - D3D10 renderer
===========================================

A port of Kentie's Unreal D3D10 renderer for Klingon Honor Guard.

# To do:
* Fix viewport resize.
* Fix Toggle Fullscreen
* Fix crash on exit / cutscene playback (GetFullName crash).

# Notes:
* Pallette conversion is not quite as it should be (aka: To make it work, I have implemented a dirty hack, where if element isn't first is array, it's considered visible... not sure how to fix it).
* From what I have tested, video playback crashes the game - setting UseDirectDraw=False seems to prevent video playback and allows for playing the game. Thing is, it crashes with all of the other renderers for me. Great game!

# Installation
1. Download the renderer from [releases page](https://github.com/SuiMachine/khg-d3d10drv/releases) and extract it.
2. Move all of the files to ``<game folder>\System``.
3. Edit ``KHG.ini`` with a text editor.
4. In ``[Engine.Engine]`` section set the ``GameRenderDevice`` to ``D3D10Drv.D3D10RenderDevice``.
5. Save the changes and launch the game.
6. If the game crashes during FMV cinematics, in ``WinDrv.WindowsClient`` section of ``KHG.ini`` set ``UseDirectDraw`` to ``False``.