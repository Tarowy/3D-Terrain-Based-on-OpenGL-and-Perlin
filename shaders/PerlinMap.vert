#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex_h;
layout (location = 2) in vec2 aTex;

out vec2 tex_coord_h;
out vec2 tex_coord;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    tex_coord_h = aTex_h;
    tex_coord = aTex;
}