#version 460

out vec4 C;

layout(binding = 0, r32ui) uniform coherent uimage2D tex_r;
layout(binding = 1, r32ui) uniform coherent uimage2D tex_g;
layout(binding = 2, r32ui) uniform coherent uimage2D tex_b;

vec3 read_pixel(ivec2 px_coord){
  return vec3(
    //imageLoad(tex_r,px_coord).x,
    //imageLoad(tex_g,px_coord).x,
    //imageLoad(tex_b,px_coord).x
    imageLoad(tex_b,px_coord).x
  )/2048.;
}


void main() {
    ivec2 ifc = ivec2(gl_FragCoord);

    C = vec4(read_pixel(ifc), 1.);
}