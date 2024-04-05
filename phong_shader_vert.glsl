#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 FragPosView;
out vec3 Normal;      // view

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normViewModelMatrix;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    vec4 viewPos = view * worldPos;
    FragPosView = vec3(viewPos);
    TexCoords = aTexCoords;
    Normal = normViewModelMatrix * aNormal;
    gl_Position = projection * viewPos;
}
