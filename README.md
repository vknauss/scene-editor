# scene-editor

WIP OpenGL scene viewer / editor, eventually to be integrated with a future game engine runtime

This project depends on my other projects ogu (opengl-utils) and vvm (vecmath).
I intend to try to loosen the coupling with the latter but for now it's pretty baked in.
ogu will stay a must, as this project and that are intended to grow together.

Features right now include a fairly flexible mesh system and a custom binary file implementation to work with it.
Basic IO utilities are implemented for this filetype, and I'm in the early stages of a blender exporter which will
end up in my blender scripts repo.

This is subject to change a lot. Right now most of the implementation is in the header files, in the include directory.
The mesh system is heavily templated so I guess this makes sense for now, but we'll see how things end up.

Future work I intend to do is to implement a custom shader generator, which will take in some code blocks and layouts for
input data and output compiled GLSL shader programs in various permutations. Also need to implement an actual render framework.
Some preliminary work on that is done (early stages of MeshRenderer which essentially wraps vertex array objects, for instance),
but since the actual draw calls are still using hard-coded index counts I suppose there's still quite a ways to go. I need to
take care to design a system which can flexibly handle meshes with many (custom) attributes and instances (and instanced vertex buffers),
as well as carefully manage both memory and draw call overhead. I have some ideas, though. Finally I guess there actually will need to be
some sort of editor framework, considering that's the goal of the project. Probably that will entail a scene framework, entities and hierarchies,
and some sort of UI (probably dear imgui).
