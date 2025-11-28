#pragma once

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

void GLClearError();

[[maybe_unused]] bool GLCheckError(const char *function_name, const char *file_path, int line);

#define ASSERT(x) if (!(x)) __debugbreak();

#define GL_TRY(x) GLClearError();\
    x;\
    ASSERT(GLCheckError(#x, __FILE__, __LINE__))

