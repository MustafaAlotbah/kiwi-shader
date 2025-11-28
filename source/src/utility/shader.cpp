#include "utility/shader.h"
#include "utility/common.h"

#include <glm/ext.hpp>


Shader::Shader(const std::string &filename) : renderer_id(0) {
    std::string vertexShader;
    std::string geometryShader;
    std::string fragmentShader;
    parseShader(filename, vertexShader, geometryShader, fragmentShader);
    unsigned int shader = createShader(vertexShader, geometryShader, fragmentShader);
    renderer_id = shader;
    GL_TRY(glUseProgram(renderer_id));
    std::cout << "Shader in!\n";
}


Shader::Shader(const std::string &vertexShader, const std::string &fragmentShader) {

    unsigned int shader = createShader(vertexShader, "", fragmentShader);
    renderer_id = shader;
    GL_TRY(glUseProgram(renderer_id));
    std::cout << "Shader in!\n";
}


Shader::~Shader() {
    std::cout << "Shader out!\n";
    glDeleteProgram(renderer_id);
}

void Shader::bind() const {
    GL_TRY(glUseProgram(renderer_id));
}

void Shader::unbind() const {
    GL_TRY(glUseProgram(0));
}


void Shader::setUniform4f(const std::string &name, float f0, float f1, float f2, float f3) {
    this->bind();
    int location = getUniformLocation(name);
    glUniform4f(location, f0, f1, f2, f3);
}

void Shader::setUniform3f(const std::string &name, float f0, float f1, float f2) {
    this->bind();
    int location = getUniformLocation(name);
    glUniform3f(location, f0, f1, f2);
}

void Shader::setUniform2f(const std::string &name, float f0, float f1) {
    this->bind();
    int location = getUniformLocation(name);
    glUniform2f(location, f0, f1);
}

void Shader::setUniform1f(const std::string &name, float f0) {
    this->bind();
    int location = getUniformLocation(name);
    glUniform1f(location, f0);
}

void Shader::setUniform4x4f(const std::string &name, glm::mat4 i) {
    this->bind();
    int location = getUniformLocation(name);
    GL_TRY(glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(i)));
}

void Shader::setUniform3x3f(const std::string &name, glm::mat3 i) {
    this->bind();
    int location = getUniformLocation(name);
    GL_TRY(glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(i)));
}

void Shader::setUniform1i(const std::string &name, int i) {
    this->bind();
    int location = getUniformLocation(name);
    glUniform1i(location, i);
}

int Shader::getUniformLocation(const std::string &name) const {
    GL_TRY(
            int location = glGetUniformLocation(renderer_id, name.c_str())
    );
    if (location == -1) {
        std::cout << "[OpenGL Error]: uniform \"" << name << "\" doesn't exist!" << std::endl;
    }

    return location;
}


static void parseShader(
        const std::string &filepath,
        std::string &vertexShader,
        std::string &geometryShader,
        std::string &fragmentShader
) {
    std::ifstream stream(filepath);

    enum class ShaderType {
        NONE = -1, VERTEX = 0, GEOMETRY = 1, FRAGMENT = 2
    };

    std::stringstream ss[3];

    std::string line;

    ShaderType type = ShaderType::NONE;

    while (getline(stream, line)) {
        if (line.find("#shader") != std::string::npos) {
            if (line.find("vertex") != std::string::npos) {
                // vertex source
                type = ShaderType::VERTEX;
            } else if (line.find("geometry") != std::string::npos) {
                // geometry source
                type = ShaderType::GEOMETRY;
            } else if (line.find("fragment") != std::string::npos) {
                // fragment source
                type = ShaderType::FRAGMENT;
            }
        } else {
            ss[(int) type] << line << '\n';
        }
    }

    vertexShader = ss[(int) ShaderType::VERTEX].str();
    geometryShader = ss[(int) ShaderType::GEOMETRY].str();
    fragmentShader = ss[(int) ShaderType::FRAGMENT].str();

}

static unsigned int compileShader(unsigned int type, const std::string &source) {
    unsigned int id = glCreateShader(type);
    const char *src = source.c_str();  // &source[0]
    GL_TRY(glShaderSource(id, 1, &src, nullptr));
    GL_TRY(glCompileShader(id));

    // TODO: handle error later
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char *message = (char *) alloca(length * sizeof(char));    // char message[length]   (allocate on the stack)
        glGetShaderInfoLog(id, length, &length, message);

        switch (type) {
            case GL_VERTEX_SHADER:
                std::cout << "Failed to compile vertex shader!" << std::endl;
                break;
            case GL_GEOMETRY_SHADER:
                std::cout << "Failed to compile geometry shader!" << std::endl;
                break;
            case GL_FRAGMENT_SHADER:
                std::cout << "Failed to compile fragment shader!" << std::endl;
                break;
            default:
                break;
        }
        std::cout << message << std::endl;
        return 0;
    }

    return id;
}


static int createShader(
        const std::string &vertexShader,
        const std::string &geometryShader,
        const std::string &fragmentShader
) {
    bool useGeometryShader = !geometryShader.empty();

    GL_TRY(unsigned int program = glCreateProgram());
    unsigned int vs = 0;
    unsigned int gs = 0;
    unsigned int fs = 0;


    vs = compileShader(GL_VERTEX_SHADER, vertexShader);
    glAttachShader(program, vs);

    if (useGeometryShader) {
        gs = compileShader(GL_GEOMETRY_SHADER, geometryShader);
        glAttachShader(program, gs);
    }

    fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);
    glAttachShader(program, fs);


    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);
    if (useGeometryShader) { glDeleteShader(gs); }

    return program;
}


