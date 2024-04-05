// http://learnopengl.com/
#include "shader.h"

#include <iostream>
#include <fstream>
#include <sstream>

Shader::Shader(const char *vertexPath,
               const char *fragmentPath,
               const char *geometryPath)
{
    std::string vertexCode;
    std::string fragmentCode;
    std::string geometryCode;

    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    std::ifstream gShaderFile;

    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;

        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();

        vShaderFile.close();
        fShaderFile.close();

        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();

        if (geometryPath)
        {
            gShaderFile.open(geometryPath);
            std::stringstream gShaderStream;
            gShaderStream << gShaderFile.rdbuf();
            gShaderFile.close();
            geometryCode = gShaderStream.str();
        }
    }
    catch (std::ifstream::failure &e)
    {
        std::cerr << "\n[File unsuccessfully read]\n";
    }

    const char *vShaderCode = vertexCode.c_str();
    const char *fShaderCode = fragmentCode.c_str();

    unsigned int vertex, fragment;

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    unsigned int geometry;
    if (geometryPath)
    {
        const char *gShaderCode = geometryCode.c_str();
        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometry, 1, &gShaderCode, NULL);
        glCompileShader(geometry);
        checkCompileErrors(geometry, "GEOMETRY");
    }

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    if (geometryPath)
        glAttachShader(ID, geometry);

    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if (geometryPath)
        glDeleteShader(geometry);
}

void Shader::use() const
{
    glUseProgram(ID);
}

void Shader::setBool(const char *name, bool value) const
{
    glUniform1i(glGetUniformLocation(ID, name), (int)value);
}

void Shader::setInt(const char *name, int value) const
{
    glUniform1i(glGetUniformLocation(ID, name), value);
}

void Shader::setFloat(const char *name, float value) const
{
    glUniform1f(glGetUniformLocation(ID, name), value);
}

void Shader::setVec2(const char *name, const glm::vec2 &value) const
{
    glUniform2fv(glGetUniformLocation(ID, name), 1, &value[0]);
}
void Shader::setVec2(const char *name, float x, float y) const
{
    glUniform2f(glGetUniformLocation(ID, name), x, y);
}

void Shader::setVec3(const char *name, const glm::vec3 &value) const
{
    glUniform3fv(glGetUniformLocation(ID, name), 1, &value[0]);
}
void Shader::setVec3(const char *name, float x, float y, float z) const
{
    glUniform3f(glGetUniformLocation(ID, name), x, y, z);
}

void Shader::setVec4(const char *name, const glm::vec4 &value) const
{
    glUniform4fv(glGetUniformLocation(ID, name), 1, &value[0]);
}
void Shader::setVec4(const char *name, float x, float y, float z, float w) const
{
    glUniform4f(glGetUniformLocation(ID, name), x, y, z, w);
}

void Shader::setMat2(const char *name, const glm::mat2 &mat) const
{
    glUniformMatrix2fv(glGetUniformLocation(ID, name), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setMat3(const char *name, const glm::mat3 &mat) const
{
    glUniformMatrix3fv(glGetUniformLocation(ID, name), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setMat4(const char *name, const glm::mat4 &mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(ID, name), 1, GL_FALSE, &mat[0][0]);
}

void Shader::checkCompileErrors(GLuint shader, const std::string &type)
{
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "\n[Shader compilation of type] " << type << " [info log] " << infoLog << " [end of info]\n";
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "\n[Program linking error of type] " << type << " [info log] " << infoLog << " [end of info]\n";
        }
    }
}
