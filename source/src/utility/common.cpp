#include "utility/common.h"


void GLClearError() {
    while (glGetError() != GL_NO_ERROR);
}

[[maybe_unused]] bool GLCheckError(const char *function_name, const char *file_path, int line) {

    while (GLenum error = glGetError()) {
        std::cout << "[OpenGL Error] (" << error << ") : " << line << " at " << function_name << " in " << file_path
                  << std::endl;
        return false;
    }

    return true;
}
