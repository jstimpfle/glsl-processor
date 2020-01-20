#version 130

in vec2 position;
in vec2 normal;
in vec3 color;

out vec2 positionF;
out vec2 normalF;
out vec3 colorF;

void main()
{
	positionF = position;
	normalF = normal;
	colorF = color;
	gl_Position = vec4(position + normal, 0, 1);
}
