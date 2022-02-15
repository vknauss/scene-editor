# scene-editor

WIP OpenGL scene viewer / editor, eventually to be integrated with a future game engine runtime

This project depends on my other projects ogu (opengl-utils) and vvm (vecmath).
I intend to try to loosen the coupling with the latter but for now it's pretty baked in.
ogu will stay a must, as this project and that are intended to grow together.

Features right now include a fairly flexible mesh system and a custom binary file implementation to work with it.
Basic IO utilities are implemented for this filetype, and I'm in the early stages of a blender exporter which will
end up in my blender scripts repo.
