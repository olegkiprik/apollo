// http://learnopengl.com/
#ifndef SHADER_H
#define SHADER_H
#include <vector>
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader
{
public:
    unsigned int ID;

    Shader(const char *vertexPath,
           const char *fragmentPath,
           const char *geometryPath = nullptr);

    void use() const;
    void setInt(const char *name, int value) const;
    void setFloat(const char *name, float value) const;

    void setBool(const char *name, bool value) const;
    void setVec2(const char *name, const glm::vec2 &value) const;
    void setVec2(const char *name, float x, float y) const;
    void setVec3(const char *name, const glm::vec3 &value) const;
    void setVec3(const char *name, float x, float y, float z) const;
    void setVec4(const char *name, const glm::vec4 &value) const;
    void setVec4(const char *name, float x, float y, float z, float w) const;
    void setMat2(const char *name, const glm::mat2 &mat) const;
    void setMat3(const char *name, const glm::mat3 &mat) const;
    void setMat4(const char *name, const glm::mat4 &mat) const;

private:
    void checkCompileErrors(GLuint shader, const std::string &type);
};

#endif
