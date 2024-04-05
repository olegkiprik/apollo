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

struct SpotlightComponents {
    float cutOff;
    float outerCutoff;
    vec3 spotDirectionView;
};

out vec4 FragColor;

in vec3 Normal;
in vec2 TexCoords;
in vec3 FragPosView;

uniform float fogDensity;
uniform vec3 fogColor;

uniform float shininess;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

uniform float gamma;

uniform Light lights[NR_LIGHTS];
uniform SpotlightComponents spotlightComponents[NR_SPOTLIGHTS];

float get_attenuation(float dist, Light light) {
    return 1.0 / (light.constant + light.linear * dist + light.quadratic * (dist * dist));
}

vec3 calc_diffuse_point_light(Light light, vec3 normal, vec3 fragPosView, vec3 viewDir) {
    vec3 lightDir   = normalize(light.lightPosView - fragPosView);
    float diff      = max(dot(normal, lightDir), 0.0);
    float dist      = length(light.lightPosView - fragPosView);
    vec3 diffuse    = light.diffuseLightColor * diff;
    diffuse         = diffuse * get_attenuation(dist, light);
    return            diffuse;
}

vec3 calc_specular_point_light(Light light, vec3 normal, vec3 fragPosView, vec3 viewDir) {
    vec3 lightDir   = normalize(light.lightPosView - fragPosView);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec      = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    float dist      = length(light.lightPosView - fragPosView);
    vec3 specular   = light.specularLightColor * spec;
    specular        = specular * get_attenuation(dist, light);
    return            specular;
}

vec3 calc_spotlight(vec3 color, SpotlightComponents spotlight, vec3 lightDir) {
    float theta     = dot(lightDir, normalize(spotlight.spotDirectionView));
    float epsilon   = spotlight.cutOff - spotlight.outerCutoff;
    float intensity = clamp((theta - spotlight.outerCutoff) / epsilon, 0.0, 1.0);
    color           = color * intensity;
    return            color;
}

void main() {

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(-FragPosView);
    vec4 Position = vec4(FragPosView, 1.0);
    
    vec3 ambient = vec3(0.0);

    for (int i = 0; i < NR_LIGHTS; ++i) {
        ambient = ambient + lights[i].ambientLightColor;
    }

    ambient *= texture(texture_diffuse1, TexCoords).rgb;

    vec3 diffuse_result = vec3(0.0);
    diffuse_result += calc_diffuse_point_light(lights[0], norm, vec3(Position), viewDir);
    diffuse_result += calc_diffuse_point_light(lights[1], norm, vec3(Position), viewDir);
    diffuse_result += calc_spotlight(calc_diffuse_point_light(lights[2], norm, vec3(Position), viewDir), spotlightComponents[0],
                                     normalize(lights[2].lightPosView - vec3(Position)));
    diffuse_result += calc_spotlight(calc_diffuse_point_light(lights[3], norm, vec3(Position), viewDir), spotlightComponents[1],
                                     normalize(lights[3].lightPosView - vec3(Position)));
    vec3 diffuse = diffuse_result * texture(texture_diffuse1, TexCoords).rgb;

    vec3 specular_result = vec3(0.0);
    specular_result += calc_specular_point_light(lights[0], norm, vec3(Position), viewDir);
    specular_result += calc_specular_point_light(lights[1], norm, vec3(Position), viewDir);
    specular_result += calc_spotlight(calc_specular_point_light(lights[2], norm, vec3(Position), viewDir), spotlightComponents[0],
                                     normalize(lights[2].lightPosView - vec3(Position)));
    specular_result += calc_spotlight(calc_specular_point_light(lights[3], norm, vec3(Position), viewDir), spotlightComponents[1],
                                     normalize(lights[3].lightPosView - vec3(Position)));
    vec3 specular = specular_result * texture(texture_specular1, TexCoords).rgb;

    vec3 result = ambient + diffuse + specular;

    float factor = length(Position) * fogDensity;
    float alpha = 1.0 / exp(factor * factor);
    result = mix(fogColor, result, alpha);
    
    result = pow(result, vec3(1.0 / gamma));
    FragColor = vec4(result, 1.0);
}
