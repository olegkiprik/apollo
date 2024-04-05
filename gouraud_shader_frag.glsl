#version 330 core

#define NR_LIGHTS 4
#define NR_SPOTLIGHTS 2

struct Light {
    vec3 ambientLightColor;
    vec3 diffuseLightColor;
    vec3 specularLightColor;

    float constant;
    float linear;
    float quadratic;

    vec3 lightPosView;
};

out vec4 FragColor;

in vec3 diffuse_intensity;
in vec3 specular_intensity;
in vec2 TexCoords;
in vec3 FragPosView;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

uniform float gamma;

uniform Light lights[NR_LIGHTS];

uniform float fogDensity;
uniform vec3 fogColor;

void main()
{
    vec3 ambient = vec3(0.0);

    for (int i = 0; i < NR_LIGHTS; ++i) {
        ambient = ambient + lights[i].ambientLightColor;
    }

    ambient *= texture(texture_diffuse1, TexCoords).rgb;

    vec3 diffuse = diffuse_intensity * texture(texture_diffuse1, TexCoords).rgb;
    vec3 specular = specular_intensity * texture(texture_specular1, TexCoords).rgb;

    vec3 color = ambient + diffuse + specular;

    float factor = length(FragPosView) * fogDensity;
    float alpha = 1.0 / exp(factor * factor);
    color = mix(fogColor, color, alpha);

    color = pow(color, vec3(1.0 / gamma));
    FragColor = vec4(color, 1.0);
}
