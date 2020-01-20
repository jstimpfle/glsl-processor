#version 130

in vec2 startPoint;
in vec2 centerPoint;
in vec2 position;
in vec3 color;
in float diffAngle;
in float radius;
in int orientation;

out vec2 startPointF;
out vec2 centerPointF;
out vec2 positionF;
out vec3 colorF;
out float diffAngleF;
out float radiusF;
flat out int orientationF;

void main()
{
	startPointF = startPoint;
	centerPointF = centerPoint;
	positionF = position;
	colorF = color;
	diffAngleF = diffAngle;
	radiusF = radius;
	orientationF = orientation;
	gl_Position = vec4(position, 0, 1);
}
