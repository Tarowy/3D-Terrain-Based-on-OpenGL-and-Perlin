#version 460 core

#define MAX_TEXTURES 5

struct terrain_material {
    sampler2D diff[MAX_TEXTURES];
    sampler2D norm[MAX_TEXTURES];
    sampler2D ao[MAX_TEXTURES];
    sampler2D disp[MAX_TEXTURES];
    float height[MAX_TEXTURES + 1];

    float triplanar_scale;
    int triplanar_sharpness;
};

struct light_data {
    vec3 light_pos;
    vec3 view_pos;
    vec3 light_color;

    float ambient_strength;
    float specular_strength;
    float specular_pow;
};

in texture_data {
    float lower_bound;
    float upper_bound;
    flat int texture_upper_index;
    flat int texture_lower_index;
} tex_data;

in terrain_data {
    float height;
    float height_01;
    vec2 tex_coord;
    vec3 frag_pos;

    vec3 w_normal;
    vec3 tangent;
    vec3 bitangent;
    vec3 blended_normal;
} data;

uniform terrain_material material;
uniform light_data light;

uniform bool enable_light;
uniform bool enable_texture;
uniform bool enable_tangent;

out vec4 FragColor;

vec3 weights;

vec3 blinn_phong_lighting(vec3 normal, vec3 diff, float ao) {
    // ambient
    vec3 ambient = light.ambient_strength * diff * light.light_color * ao;

    // diffuse
    vec3 light_dir = normalize(light.light_pos - data.frag_pos);
    float diff_factor = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = diff * diff_factor * light.light_color;

    // specular

    return vec3(ambient + diffuse);
}

vec4 get_diff(vec2 tex) {
    // the percentage of the height value within the current height range.
    //    float percentage = (data.height_01 - lower_bound) / (upper_bound - lower_bound);

    vec4 base_color = texture2D(material.diff[tex_data.texture_lower_index], tex);
    vec4 next_color = texture2D(material.diff[tex_data.texture_upper_index], tex);
    return mix(base_color, next_color, smoothstep(tex_data.lower_bound, tex_data.upper_bound, data.height_01));
}

vec4 get_ao(vec2 tex) {
    vec4 base_color = texture2D(material.ao[tex_data.texture_lower_index], tex);
    vec4 next_color = texture2D(material.ao[tex_data.texture_upper_index], tex);
    return mix(base_color, next_color, smoothstep(tex_data.lower_bound, tex_data.upper_bound, data.height_01));
}

// triplanar to sample diff texture
vec4 get_diff_triplanar() {
    vec4 x_color = texture2D(material.diff[tex_data.texture_lower_index], data.frag_pos.yz * material.triplanar_scale);
    vec4 y_color = texture2D(material.diff[tex_data.texture_lower_index], data.frag_pos.xz * material.triplanar_scale);
    vec4 z_color = texture2D(material.diff[tex_data.texture_lower_index], data.frag_pos.xy * material.triplanar_scale);

    vec4 base_color = x_color * weights.x + y_color * weights.y + z_color * weights.z;

    x_color = texture2D(material.diff[tex_data.texture_upper_index], data.frag_pos.yz * material.triplanar_scale);
    y_color = texture2D(material.diff[tex_data.texture_upper_index], data.frag_pos.xz * material.triplanar_scale);
    z_color = texture2D(material.diff[tex_data.texture_upper_index], data.frag_pos.xy * material.triplanar_scale);

    vec4 next_color = x_color * weights.x + y_color * weights.y + z_color * weights.z;

    return mix(base_color, next_color, smoothstep(tex_data.lower_bound, tex_data.upper_bound, data.height_01));
}

// triplanar to sample normal texture
vec4 get_normal_triplanar() {
    vec4 x_color = texture2D(material.norm[tex_data.texture_lower_index], data.frag_pos.yz * material.triplanar_scale);
    vec4 y_color = texture2D(material.norm[tex_data.texture_lower_index], data.frag_pos.xz * material.triplanar_scale);
    vec4 z_color = texture2D(material.norm[tex_data.texture_lower_index], data.frag_pos.xy * material.triplanar_scale);

    vec4 base_color = x_color * weights.x + y_color * weights.y + z_color * weights.z;

    x_color = texture2D(material.norm[tex_data.texture_upper_index], data.frag_pos.yz * material.triplanar_scale);
    y_color = texture2D(material.norm[tex_data.texture_upper_index], data.frag_pos.xz * material.triplanar_scale);
    z_color = texture2D(material.norm[tex_data.texture_upper_index], data.frag_pos.xy * material.triplanar_scale);

    vec4 next_color = x_color * weights.x + y_color * weights.y + z_color * weights.z;

    return mix(base_color, next_color, smoothstep(tex_data.lower_bound, tex_data.upper_bound, data.height_01)) * 2 - 1;
}

void compute_normal_weight() {
    weights = abs(data.w_normal);
    weights = vec3(pow(weights.x, material.triplanar_sharpness),
                   pow(weights.y, material.triplanar_sharpness),
                   pow(weights.z, material.triplanar_sharpness));

    weights = weights / (weights.x + weights.y + weights.z);
}

void main()
{
    compute_normal_weight();

    mat3 tbn = mat3(data.tangent, data.bitangent, data.w_normal);

    vec3 color = vec3(data.height_01, data.height_01, data.height_01);
    float ao = 1.0f;
    vec3 tex_noraml = data.w_normal;

    if (enable_texture) {
        //        color = get_diff(data.tex_coord).xyz;
        color = get_diff_triplanar().xyz;
        ao = get_ao(data.tex_coord).r;
    }

    if (enable_tangent) {
        tex_noraml = tbn * get_normal_triplanar().xyz;
    }

    if (enable_light) {

        color = blinn_phong_lighting
        (
        // transform normal of normal map from tangent space to world space
            tex_noraml,
            color.xyz,
            ao
        );
    }

    //    FragColor = vec4(data.blended_normal, 1.0f);
    FragColor = vec4(color, 1.0f);
}