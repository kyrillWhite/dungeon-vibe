#pragma once

#include <GL/glew.h>

inline unsigned int compileShaderPipeline(const char* vSource, const char* fSource) {
	unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vSource, NULL);
	glCompileShader(vs);
	unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fSource, NULL);
	glCompileShader(fs);
	unsigned int program = glCreateProgram();
	glAttachShader(program, vs); glAttachShader(program, fs);
	glLinkProgram(program);
	glDeleteShader(vs); glDeleteShader(fs);
	return program;
}

