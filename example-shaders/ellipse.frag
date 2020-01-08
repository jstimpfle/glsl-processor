uniform vec2 p0;
uniform vec2 p1;
uniform float radius;
uniform vec3 color;

in vec2 positionF;

void main()
{
    float d0 = distance(p0.xy, positionF);
    float d1 = distance(p1.xy, positionF);
    float d = d0 + d1;
    float rdx = fwidth(d);
    if (d > radius)
        discard;
    float val = (d - (radius - rdx)) / rdx;
    gl_FragColor = vec4(color, 1.0 - val);
}
