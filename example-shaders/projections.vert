uniform mat3 projMat;

vec4 compute_screenpos(vec2 position)
{
    vec3 v = projMat * vec3(position, 1.0);
    return vec4(v.xy, 0.0, 1.0);
}

in vec2 position;
out vec2 positionF;

void main()
{
    positionF = position;
    gl_Position = compute_screenpos(position);
}
