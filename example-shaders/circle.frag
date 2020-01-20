#version 130

in vec2 diffF;
in vec3 colorF;
in float radiusF;

void main()
{
	float d = length(diffF);
	if (d <= radiusF)
		gl_FragColor = vec4(colorF, 1);
	else
		discard;
}
