#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

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

out vec3 diffuse_intensity;
out vec3 specular_intensity;
out vec3 FragPosView;
out vec2 TexCoords;

uniform float shininess;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normViewModelMatrix;

uniform Light lights[NR_LIGHTS];
uniform SpotlightComponents spotlightComponents[NR_SPOTLIGHTS];

float get_attenuation(float dist, Light light) {
    return 1.0 / (light.constant + light.linear * dist + light.quadratic * (dist * dist));
}

vec3 calc_diffuse_point_light(Light light, vec3 normal, vec3 fragPosView, vec3 viewDir) {
    vec3 lightDir   = normalize(light.lightPosView - fragPosView);
    float diff      = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
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

    vec4 worldPos = model * vec4(aPos, 1.0);
    vec4 viewPos = view * worldPos;
    gl_Position = projection * viewPos;

    vec3 Position = vec3(viewPos);               // view
    vec3 Normal = normViewModelMatrix * aNormal; // view, unnormalized

    vec3 norm    = normalize(Normal);            // view
    vec3 viewDir = normalize(-Position);

    vec3 diffuse_result = vec3(0.0);
    diffuse_result += calc_diffuse_point_light(lights[0], norm, vec3(Position), viewDir);
    diffuse_result += calc_diffuse_point_light(lights[1], norm, vec3(Position), viewDir);
    diffuse_result += calc_spotlight(calc_diffuse_point_light(lights[2], norm, vec3(Position), viewDir), spotlightComponents[0],
                                     normalize(lights[2].lightPosView - vec3(Position)));
    diffuse_result += calc_spotlight(calc_diffuse_point_light(lights[3], norm, vec3(Position), viewDir), spotlightComponents[1],
                                     normalize(lights[3].lightPosView - vec3(Position)));

    vec3 specular_result = vec3(0.0);
    specular_result += calc_specular_point_light(lights[0], norm, vec3(Position), viewDir);
    specular_result += calc_specular_point_light(lights[1], norm, vec3(Position), viewDir);
    specular_result += calc_spotlight(calc_specular_point_light(lights[2], norm, vec3(Position), viewDir), spotlightComponents[0],
                                     normalize(lights[2].lightPosView - vec3(Position)));
    specular_result += calc_spotlight(calc_specular_point_light(lights[3], norm, vec3(Position), viewDir), spotlightComponents[1],
                                     normalize(lights[3].lightPosView - vec3(Position)));

    diffuse_intensity = diffuse_result;
    specular_intensity = specular_result;
    TexCoords = aTexCoords;
    FragPosView = vec3(viewPos);
}
