#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

#define NR_LIGHTS 4
#define NR_SPOTLIGHTS 2

struct Light {
    vec3 ambientLightColor;
    vec3 diffuseLightColor;
    vec3 specularLightColor;

    // attenuation
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

in VS_OUT {
    vec2 texCoords;
    vec4 position;  // view
    vec3 normal;    // view
} gs_in[];

out vec3 diffuse_intensity;
out vec3 specular_intensity;
out vec2 TexCoords;
out vec3 FragPosView;

uniform float shininess;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normViewModelMatrix;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

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
    vec3 Position = vec3((gs_in[0].position + gs_in[1].position + gs_in[2].position) / 3);
    vec3 norm = normalize(gs_in[0].normal);
    vec3 viewDir = normalize(-Position);
    
    vec3 diffuse_result = vec3(0.0);
    diffuse_result += calc_diffuse_point_light(lights[0], norm, Position, viewDir);
    diffuse_result += calc_diffuse_point_light(lights[1], norm, Position, viewDir);
    diffuse_result += calc_spotlight(calc_diffuse_point_light(lights[2], norm, Position, viewDir),
                                     spotlightComponents[0], normalize(lights[2].lightPosView - Position));
    diffuse_result += calc_spotlight(calc_diffuse_point_light(lights[3], norm, Position, viewDir),
                                     spotlightComponents[1], normalize(lights[3].lightPosView - Position));

    vec3 specular_result = vec3(0.0);
    specular_result += calc_specular_point_light(lights[0], norm, Position, viewDir);
    specular_result += calc_specular_point_light(lights[1], norm, Position, viewDir);
    specular_result += calc_spotlight(calc_specular_point_light(lights[2], norm, Position, viewDir),
                                      spotlightComponents[0], normalize(lights[2].lightPosView - Position));
    specular_result += calc_spotlight(calc_specular_point_light(lights[3], norm, Position, viewDir),
                                      spotlightComponents[1], normalize(lights[3].lightPosView - Position));

    for (int i = 0; i < 3; ++i) {
        gl_Position = projection * gs_in[i].position;
        TexCoords = gs_in[i].texCoords;
        diffuse_intensity = diffuse_result;
        specular_intensity = specular_result;
        FragPosView = vec3(gs_in[i].position);
        EmitVertex();
    }

    EndPrimitive();
}
