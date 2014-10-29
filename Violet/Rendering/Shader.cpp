#include "stdafx.h"
#include "Shader.hpp"
#include <iostream>
#include <fstream>
#include <vector>

ShaderProgram::ShaderResource::ShaderResource(std::string path)
	: ResourceTy(path)
{
    std::ifstream vert(path + ".vert");
    std::ifstream frag(path + ".frag");
    init(vert, frag);
}

ShaderProgram::ShaderResource::~ShaderResource()
{
    //the program will be deleted once it is no longer part of an active rendering state
    glDeleteProgram(program);
}

void ShaderProgram::use() const
{
    //set this program as current
    glUseProgram(program);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

#if 0
GLint ShaderProgram::Ref::GetUniformLocation(const char* name) const
{
    return glGetUniformLocation(program, name);
}

GLenum ShaderProgram::Ref::GetUniformType(GLint loc) const
{
	GLenum ty;
	GLint size;
	//request to write 0 characters of name, so nullptr should be OK
	glGetActiveUniform(program, loc, 0, nullptr, &size, &ty, nullptr);
	return ty;
}
#endif

GLenum ShaderProgram::GetAttribType(GLint loc) const
{
	GLint size;
	GLenum ret;
	glGetActiveAttrib(program, loc, 0, nullptr, &size, &ret, nullptr);
	return ret;
}

GLint ShaderProgram::GetAttribLocation(const char* name) const
{
    return glGetAttribLocation(program, name);
}

UBO ShaderProgram::GetUBO(const std::string& name) const
{
	if (name == "Common")
		return UBO::Create(resource->uniforms[name], UBO::Common);
	return UBO::Create(resource->uniforms[name], UBO::Material);
}

//These should stay the same for much of the shader's lifetime
void ShaderProgram::TextureOrder(const std::vector<std::string>& order)
{
	use(); //program needs to be active
	for (size_t i = 0; i < order.size(); ++i)
	{
		//Samplers are not in blocks (so "")
		glUniform1i(resource->uniforms[""][order[i]].location, i);
	}
}

GLuint CreateShader(GLenum eShaderType, std::istream &t)
{
    if (!t)
        throw "Shader file not found";
    //read the stream into a string
    t.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(t.tellg());
    std::string buffer(size, ' ');
    t.seekg(0);
    t.read(&buffer[0], size); 

    //create the shader Render
    GLuint shader = glCreateShader(eShaderType);

    //attach and compile the source
    const GLchar *strFileData = buffer.c_str();
    glShaderSource(shader, 1, &strFileData, NULL);
    glCompileShader(shader);
    
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) //compile failed
    {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::vector<GLchar> infoLog(infoLogLength + 1);

        //print error message
        glGetShaderInfoLog(shader, infoLogLength, NULL, infoLog.data());
        std::cerr << "Compile failure in shader: " << infoLog.data() << "\n";
    }

    return shader;
}

void ShaderProgram::ShaderResource::init(std::istream &vert, std::istream &frag)
{
    //create our empty program Render
    program = glCreateProgram();

    GLuint vertShdr = CreateShader(GL_VERTEX_SHADER, vert);
    GLuint fragShdr = CreateShader(GL_FRAGMENT_SHADER, frag);

    //attach vertex and fragment shaders
    glAttachShader(program, vertShdr);
    glAttachShader(program, fragShdr);
    
    //link the program Render
    glLinkProgram(program);

    //check for linker errors
    GLint status;
    glGetProgramiv (program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::vector<GLchar> infoLog(infoLogLength + 1);

        glGetProgramInfoLog(program, infoLogLength, NULL, infoLog.data());
        std::cerr << "Linker failure: " << infoLog.data() << "\n";
    }
	else
		uniforms = ::Uniforms(program);

    //shaders are no longer used now that the program is linked
    glDetachShader(program, vertShdr);
    glDeleteShader(vertShdr);
    glDetachShader(program, fragShdr);
    glDeleteShader(fragShdr);
}
