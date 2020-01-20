#version 130

in vec2 positionF;
in vec2 normalF;
in vec3 colorF;

void main()
{
	float w = length(fwidth(positionF));
	gl_FragColor = vec4(colorF, 1);
}
