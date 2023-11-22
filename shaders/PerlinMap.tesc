#version 460 core

// specifying the number of vertices per patch
layout (vertices = 4) out;

uniform mat4 model;
uniform mat4 view;

// the array size equals the number of vertices in the patch
in vec2 tex_coord[];
out vec2 texture_coord[];

void main() {
    // identify which vertex of the patch currently be processing by invocation id
    // do not change the model coord of the vertex
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    // do not change the texture coord of the vertex
    texture_coord[gl_InvocationID] = tex_coord[gl_InvocationID];

    // invocation zero controls tessellation levels for the entire patch
    if (gl_InvocationID == 0) {

        const int MIN_TESS_LEVEL = 4;
        const int MAX_TESS_LEVEL = 64;
        const float MIN_DISTANCE = 20;
        const float MAX_DISTANCE = 800;

        // transform model position of corners to view position
        vec4 eyeSpacePos00 = view * model * gl_in[0].gl_Position;
        vec4 eyeSpacePos01 = view * model * gl_in[1].gl_Position;
        vec4 eyeSpacePos10 = view * model * gl_in[2].gl_Position;
        vec4 eyeSpacePos11 = view * model * gl_in[3].gl_Position;

        // "distance" from camera scaled between 0 and 1
        float distance00 = clamp((abs(eyeSpacePos00.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0, 1.0);
        float distance01 = clamp((abs(eyeSpacePos01.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0, 1.0);
        float distance10 = clamp((abs(eyeSpacePos10.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0, 1.0);
        float distance11 = clamp((abs(eyeSpacePos11.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0, 1.0);

        // interpolate the tess level of each edge
        float tessLevel0 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance10, distance00));
        float tessLevel1 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance00, distance01));
        float tessLevel2 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance01, distance11));
        float tessLevel3 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance11, distance10));

        gl_TessLevelOuter[0] = 32;
        gl_TessLevelOuter[1] = 32;
        gl_TessLevelOuter[2] = 32;
        gl_TessLevelOuter[3] = 32;

        gl_TessLevelInner[0] = 32;
        gl_TessLevelInner[1] = 32;
    }

}