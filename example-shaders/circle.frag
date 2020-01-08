uniform mat3 projMat;
uniform vec2 centerPoint;
uniform float radius;
uniform vec3 color;

in vec2 positionF;

float compute_specular_strength(vec3 lightPos, vec3 surfacePoint, vec3 normalizedSurfaceNormal, vec3 spectatorPosition) {
    vec3 lightToSurface = surfacePoint - lightPos;
    vec3 reflectVector = normalize(lightToSurface - 2.0 * dot(lightToSurface, normalizedSurfaceNormal) * normalizedSurfaceNormal);
    vec3 spectateDirection = normalize(spectatorPosition - surfacePoint);
    float strength = pow(clamp(dot(reflectVector, spectateDirection), 0, 1), 4.0);
    return strength;
}

void main()
{
    float d = distance(positionF, centerPoint);
    if (d > radius)
        discard;
    /* Find height h which is the y-component such that vec3(positionF, h) is on the surface of the circle ("ball"). */
    /* That means that h must be such that h^2 + d^2 = radius^2 */
    float h = sqrt(radius * radius - d * d);
    vec3 surfacePoint = vec3(positionF, h);
    vec3 centerToSurface = surfacePoint - vec3(centerPoint, 0.0);
    vec3 lightPos = vec3(0.2, 0.5, 5.0*radius);
    vec3 lightPos2 = vec3(1.0, 1.0, 1.0);
    vec3 surfaceToLight = lightPos - vec3(positionF, h);
    vec3 spectatorPosition = vec3(0.5, 0.5, 6.0);  // center of screen

    float dotProduct = dot(normalize(surfaceToLight), normalize(centerToSurface));
    float diffuseStrength = clamp(dotProduct, 0.0, 1.0) + 0.2;

    vec3 surfaceNormal = normalize(centerToSurface);
    float specularStrength = compute_specular_strength(lightPos, surfacePoint, surfaceNormal, spectatorPosition);
    float specularStrength2 = compute_specular_strength(lightPos2, surfacePoint, surfaceNormal, spectatorPosition);

                /* smooth shape at the edges */
    float rdx = fwidth(d);
    float val = (d - (radius - rdx)) / rdx;

    vec3 specularLight = vec3(0.0, 1.0, 1.0);
    vec3 specularLight2 = vec3(0.3, 0.0, 0.6);
    vec3 specularColor = 0.5 * specularStrength * specularLight;
    vec3 specularColor2 = 0.5 * specularStrength2 * specularLight2;
    float strength = 0.1 + 0.3 * diffuseStrength;
    gl_FragColor = vec4(strength * color + (specularColor + specularColor2), 1.0 - val);
}
