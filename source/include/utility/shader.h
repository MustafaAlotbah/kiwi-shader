#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <glm/glm.hpp>

#include "utility/common.h"


static void parseShader(const std::string &filepath, std::string &vertexShader, std::string &geometryShader,
                        std::string &fragmentShader);

static unsigned int compileShader(unsigned int type, const std::string &source);

static int createShader(
        const std::string &vertexShader,
        const std::string &geometryShader,
        const std::string &fragmentShader
);


// RAII for OpenGL Shader
class Shader {
private:
    unsigned int renderer_id;
    bool compiled = false;
    // caching for uniforms
public:
    explicit Shader(const std::string &filename);

    Shader(const std::string &vertexShader, const std::string &fragmentShader);

    ~Shader();

    void bind() const;

    void unbind() const;

    // set uniforms
    void setUniform4x4f(const std::string &name, glm::mat4 matrix);
    void setUniform3x3f(const std::string &name, glm::mat3 matrix);

    void setUniform4f(const std::string &name, float f0, float f1, float f2, float f3);

    void setUniform3f(const std::string &name, float f0, float f1, float f2);

    void setUniform2f(const std::string &name, float f0, float f1);

    void setUniform1f(const std::string &name, float f0);

    void setUniform1i(const std::string &name, int i);

private:
    [[nodiscard]] int getUniformLocation(const std::string &name) const;

};

