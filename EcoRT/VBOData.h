#pragma once

#include <GL/glew.h>

class VBOData {
public:
    GLuint vboID;
    size_t size;
    GLsizei dataSize;  // Size of data in bytes

    VBOData();
};
