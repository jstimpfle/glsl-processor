#version 130

in vec2 centerPoint;
in vec2 diff;
in vec3 color;
in float radius;

out vec2 diffF;
out vec3 colorF;
out float radiusF;

void main()
{
	diffF = radius * diff;
	colorF = color;
	radiusF = radius;
	vec2 position = centerPoint + radius * diff;
	gl_Position = vec4(position, 0, 1);
}
