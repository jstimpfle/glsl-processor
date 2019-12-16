This is a preprocessor for GLSL, work in progress.

Its goal is to be used in build scripts to generate interfaces to OpenGL
shaders, and preprocess them, written in GLSL (i.e., .vert, .frag... files).

It's written in pure C and weighs only about 2000 lines. Thus it should be
relatively easy to change. Currently, it's only really meant to work as a
standalone executable.

The link file format
--------------------

This program understands its own custom linker file format is implemented here.
Make a file with a .link extension and specify your shader files and their
dependency using these three commands:

  program PROGRAMNAME FILEPATH;
  shader SHADERNAME SHADERTYPE FILEPATH;
  link PROGRAMNAME SHADERNAME;

A "program" line declares an OpenGL (shader) program that is linked from OpenGL
shaders.

A "shader" line declares a shader object, such as a vertex shader or fragment
shader, by specifiying a unique shader name and filepath.

A "link" line adds a shader to the set of shaders that get linked in a program.
This works by referencing the shader and the program using the names that were
specifiedin their declarations. In OpenGL, a program must have at least a
vertex shader and a fragment shader linked.

Here is an example of a test.link file:

  program circle;
  program ellipse;
  
  shader projections_vert VERTEX_SHADER "shaders/projections.vert";
  shader circle_frag FRAGMENT_SHADER "shaders/circle.frag";
  shader ellipse_frag FRAGMENT_SHADER "shaders/ellipse.frag";
  
  link circle projections_vert;
  link circle circle_frag;
  link ellipse projections_vert;
  link ellipse ellipse_frag;

Two programs are defined here, "circle", and "ellipse". Both are created by
linking "projections_vert" and either of the other two shaders.


Running
-------

Currently, some functionality is missing and only a subset of GLSL is recognized.
You can check if the program is already useful for you by running it with the filepath
of a .link file as the own argument

$ ./glsl-processor test.link


TODO
----

Planned functionality includes

 - Preprocess shaders by auto-generating interfaces to other GLSL shaders and
prepending them to the source code. The idea is that you shouldn't need to
duplicate declarations for uniforms, attributes, or functions that are shared
between multiple shaders that get linked into a single program. Instead, we want
to prepend to each shader all the external attributes and uniforms from the
shaders that they're linked with, provided that these can be uniquely
determined (shaders can be used as part of multiple programs).

Having an explicit #include directive would be another option, but that would
mean more typing work, the need to invent a name for the interface, possibly
the need to make interfaces explicitly, and would also require a GLSL language
extension. That's why I first want to try if the automatic approach is
practical.

- Generate nice interfaces for use in C code.

We need a simple and stable data API to describe GLSL shader interfaces. That
mostly means all the link information contained in .link files (because the
compiling and linking needs to happen at runtime), and also description of all
the attributes and uniforms in each program object.

Possibly we also want to auto-generate C structs that hold representations of
all the uniform values of each shader, along with functions to upload these
uniforms to structs.

The general problem is that there endless possibilities how this can be done,
so if you need something specific the recommendation is that you simply change
the code in process.c to achieve what you want.
