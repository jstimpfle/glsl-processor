#version 130

in vec2 startPointF;
in vec2 centerPointF;
in vec2 positionF;
in vec3 colorF;
in float diffAngleF;
in float radiusF;
flat in int orientationF;

int compute_winding_order(vec2 p, vec2 q, vec2 r)
{
        float area = 0.0;
        area += (q.x - p.x) * (q.y + p.y);
        area += (r.x - q.x) * (r.y + q.y);
        area += (p.x - r.x) * (p.y + r.y);
        return int(area > 0) - int(area < 0);
}

float compute_angle(vec2 p, vec2 q)
{
        return acos(dot(p, q) / (length(p) * length(q)));
}

float compute_signed_angle(vec2 p, vec2 q)
{
        float diffAngle = compute_angle(p, q);
        if (compute_winding_order(p, vec2(0,0), q) < 0)
                diffAngle = -diffAngle;
        return diffAngle;
}

float compute_full_angle(vec2 p, vec2 q)
{
	float PI = 3.14159265359;
	float angle = compute_angle(p, q);
	if (compute_winding_order(p, vec2(0, 0), q) == -1)
		angle = 2 * PI - angle;
	return angle;
}

void main()
{
	float PI = 3.14159265359;
	if (distance(positionF, centerPointF) > radiusF)
		discard;
	if (diffAngleF < 0.0) {
		float angle = -compute_full_angle(positionF - centerPointF, startPointF - centerPointF);
		if (angle < diffAngleF || angle > 0)
			discard;
	}
	else {
		float angle = compute_full_angle(startPointF - centerPointF, positionF - centerPointF);
		if (angle < 0 || angle > diffAngleF)
			discard;
	}
	gl_FragColor = vec4(colorF, 0.2);
}
