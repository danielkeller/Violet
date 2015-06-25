#include "stdafx.h"
#include "Shader.hpp"

#include "Utils/Template.hpp"
#include "File/Persist.hpp"
#include "File/Filesystem.hpp"
#include "RenderPasses.hpp"

#include <iostream>

UniformType::UniformType(GLenum type)
{
	switch (type)
	{
#define INTEGRAL(SCALAR) \
	case SCALAR: case SCALAR##_VEC2: case SCALAR##_VEC3: case SCALAR##_VEC4:\
		scalar = SCALAR; break

	INTEGRAL(GL_INT); INTEGRAL(GL_UNSIGNED_INT); INTEGRAL(GL_BOOL); INTEGRAL(GL_FLOAT);

	case GL_FLOAT_MAT2: case GL_FLOAT_MAT3: case GL_FLOAT_MAT4: 
		scalar = GL_FLOAT; break;

	default: scalar = type;
	}

	switch (type)
	{	
#define VEC(TYPE, DIM) case TYPE##_VEC##DIM:
#define VECS(DIM) \
VEC(GL_INT, DIM) VEC(GL_UNSIGNED_INT, DIM) VEC(GL_BOOL, DIM) VEC(GL_FLOAT, DIM)\
rows = DIM; cols = 1; break

	VECS(2); VECS(3); VECS(4);

	case GL_FLOAT_MAT2: rows = cols = 2; break;
	case GL_FLOAT_MAT3: rows = cols = 3; break;
	case GL_FLOAT_MAT4: rows = cols = 4; break;
	default: rows = cols = 1;
	}
}

struct Uniforms
{
	Uniforms() = default;
	Uniforms(GLuint program);

	std::vector<UniformBlock> blocks;

	UniformBlock& operator[](const std::string& name);
	const UniformBlock& operator[](const std::string& name) const
	{
		return static_cast<UniformBlock&>(const_cast<Uniforms&>(*this)[name]);
	}
};

struct ShaderProgram::ShaderResource : public Resource<ShaderResource>
{
	ShaderResource(ShaderResource &&other);

	//free resources associated with this program
	~ShaderResource();

	//object is not copyable
	ShaderResource(const ShaderResource&) = delete;
	ShaderResource& operator=(const ShaderResource&) = delete;

	//Construct from given vertex and fragment files
	ShaderResource(std::string path);

	std::string Name() const { return key; }

	//the actual GL program reference
	GLuint program;
	void init(const std::string& vertName, const std::string& fragName);

	Uniforms uniforms;
};

//It doesn't matter what these are, as long as they're always the same.
//This lets us swap out shaders that use standard attributes
//at a minimum there are 16 attribute locations, and generally there are no more
std::map<std::string, GLint> standardAttribs = {
    {"transform", 10}, //take up 4 locations
    {"position",  14},
    {"object",    15}
};

ShaderProgram::ShaderResource::ShaderResource(std::string path)
	: ResourceTy(path)
{
    init(path + ".vert", path + ".frag");
}

ShaderProgram::ShaderResource::ShaderResource(ShaderResource &&other)
	: ResourceTy(std::move(other)), program(other.program)
	, uniforms(std::move(other.uniforms))
{
	other.program = 0;
}

ShaderProgram::ShaderResource::~ShaderResource()
{
    //the program will be deleted once it is no longer part of an active rendering state
    glDeleteProgram(program);
}

ShaderProgram::ShaderProgram(std::string path)
	: ShaderProgram(ShaderResource::FindOrMake(path))
{}

ShaderProgram::ShaderProgram(std::shared_ptr<ShaderResource> ptr)
	: program(ptr->program)
	, resource(std::move(ptr))
{}

void ShaderProgram::use() const
{
    //set this program as current
    glUseProgram(program);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

std::string ShaderProgram::Name() const
{
	return resource->Name();
}

GLenum ShaderProgram::GetAttribType(GLint loc) const
{
	GLint size;
	GLenum ret;
	glGetActiveAttrib(program, loc, 0, nullptr, &size, &ret, nullptr);
	return ret;
}

GLint ShaderProgram::GetAttribLocation(const std::string& name) const
{
    auto it = standardAttribs.find(name);
    if (it != standardAttribs.end())
        return it->second;
    return glGetAttribLocation(program, name.c_str());
}

//These should stay the same for much of the shader's lifetime
void ShaderProgram::TextureOrder(const std::vector<std::string>& order)
{
	use(); //program needs to be active
	for (GLint i = 0; i < static_cast<GLint>(order.size()); ++i)
	{
		//Samplers are not in blocks (so "")
		glUniform1i(resource->uniforms[""][order[i]].location, i);
	}
}

GLuint CreateShader(GLenum eShaderType, const std::string& name)
{
    MappedFile shaderFile{name};
    
    //create the shader object
    GLuint shader = glCreateShader(eShaderType);
    
    //attach and compile the source
    
	static const std::string versionString =
#ifdef __APPLE__
		"#version 330\n"
		"#line 1\n";
#else
		"#version 130\n"
		"#extension GL_ARB_uniform_buffer_object : require\n"
		"#line 0\n";
#endif
    
    GLint lens[2] = {
        static_cast<GLint>(versionString.size()),
        static_cast<GLint>(shaderFile.Size()) };
    const GLchar* srcs[2] = { versionString.c_str(), shaderFile.Data<GLchar>() };
    
    glShaderSource(shader, 2, srcs, lens);
    glCompileShader(shader);
    
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) //compile failed
    {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::vector<GLchar> infoLog(infoLogLength + 1);

        //print error message
        glGetShaderInfoLog(shader, infoLogLength, NULL, infoLog.data());
        throw std::runtime_error(name + ":\n" + infoLog.data());
    }

    return shader;
}

void ShaderProgram::ShaderResource::init(const std::string& vertName, const std::string& fragName)
{
    //create our empty program Render
    program = glCreateProgram();

    GLuint vertShdr = CreateShader(GL_VERTEX_SHADER, vertName);
    GLuint fragShdr = CreateShader(GL_FRAGMENT_SHADER, fragName);

    //attach vertex and fragment shaders
    glAttachShader(program, vertShdr);
    glAttachShader(program, fragShdr);

    for (const auto& pair : standardAttribs)
        glBindAttribLocation(program, pair.second, pair.first.c_str());
    
	int colorBuffer = 0;
	for (const auto& str : shaderOutputs)
		glBindFragDataLocation(program, colorBuffer++, str);

    //link the program Render
    glLinkProgram(program);

    //check for linker errors
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status)
    {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::vector<GLchar> infoLog(infoLogLength + 1);

        glGetProgramInfoLog(program, infoLogLength, NULL, infoLog.data());
        //FIXME: leaks shaders
        throw std::runtime_error("Linker failure: " + Name() + "\n" + infoLog.data());
    }
	
    uniforms = Uniforms(program);
    
	//block binding is done by convention
    for(GLuint uniformBlockIndex = 0; uniformBlockIndex < uniforms.blocks.size(); ++uniformBlockIndex)
        if (uniforms.blocks[uniformBlockIndex].name != "")
            glUniformBlockBinding(program, uniformBlockIndex,
                uniforms.blocks[uniformBlockIndex].name == "Common" ? UBO::Common : UBO::Material);

    //shaders are no longer used now that the program is linked
#ifdef NDEBUG
    glDetachShader(program, vertShdr);
    glDeleteShader(vertShdr);
    glDetachShader(program, fragShdr);
    glDeleteShader(fragShdr);
#endif
}

std::vector<UniformBlock> DoQuery(GLuint program)
{
	GLint activeUniforms;
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &activeUniforms);

	GLint activeUniformBlocks;
	glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCKS, &activeUniformBlocks);
	std::vector<UniformBlock> ret(activeUniformBlocks + 1);
	GLint outParam;

	for (GLuint uniformBlockIndex = 0; uniformBlockIndex < (GLuint)activeUniformBlocks; ++uniformBlockIndex)
	{
		glGetActiveUniformBlockiv(program, uniformBlockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &outParam);
		ret[uniformBlockIndex].byte_size = outParam;
		glGetActiveUniformBlockiv(program, uniformBlockIndex, GL_UNIFORM_BLOCK_NAME_LENGTH, &outParam);
		std::vector<char> name(outParam);
		glGetActiveUniformBlockName(program, uniformBlockIndex, outParam, NULL, name.data());
		ret[uniformBlockIndex].name.assign(name.begin(), name.end() - 1); //remove null
	}

	for (GLuint uniformIndex = 0; uniformIndex < (GLuint)activeUniforms; ++uniformIndex)
	{
		Uniform unif;
		glGetActiveUniformsiv(program, 1, &uniformIndex, GL_UNIFORM_TYPE, &outParam);
		unif.type = static_cast<GLenum>(outParam);
		glGetActiveUniformsiv(program, 1, &uniformIndex, GL_UNIFORM_SIZE, &outParam);
		unif.size = outParam;
		glGetActiveUniformsiv(program, 1, &uniformIndex, GL_UNIFORM_OFFSET, &outParam);
		unif.offset = outParam;
		glGetActiveUniformsiv(program, 1, &uniformIndex, GL_UNIFORM_ARRAY_STRIDE, &outParam);
		unif.stride = outParam;
		//Hopefully the matrix stride isn't a big deal...

		glGetActiveUniformsiv(program, 1, &uniformIndex, GL_UNIFORM_NAME_LENGTH, &outParam);
		std::vector<char> name(outParam);
		glGetActiveUniformName(program, uniformIndex, outParam, NULL, name.data());
		if (unif.size == 1)
			unif.name.assign(name.begin(), name.end() - 1); //remove null
		else
			unif.name.assign(name.begin(), name.end() - 1 - 3); //remove null and "[0]"

		unif.location = glGetUniformLocation(program, name.data());

		glGetActiveUniformsiv(program, 1, &uniformIndex, GL_UNIFORM_BLOCK_INDEX, &outParam);

		if (outParam == -1) //last block is default block
			ret[activeUniformBlocks].uniforms.push_back(std::move(unif));
		else
			ret[outParam].uniforms.push_back(std::move(unif));
	}

	return ret;
}

Uniforms::Uniforms(GLuint program)
	: blocks(DoQuery(program))
{}

const Uniform& UniformBlock::operator[](const std::string& n) const
{
	auto it = std::find(uniforms.begin(), uniforms.end(), n);
	if (it == uniforms.end())
        throw std::runtime_error("No uniform '" + n + "' in block '" + name + "'");
	return *it;
}

UniformBlock& Uniforms::operator[](const std::string& n)
{
	auto it = std::find(blocks.begin(), blocks.end(), n);
	if (it == blocks.end())
	{
		//?
		blocks.emplace_back();
		blocks.back().name = n;
		return blocks.back();
	}
	return *it;
}

UBO::UBO(ShaderProgram shader, const std::string& block)
	: UBO(shader.resource->uniforms[block], BufferTy(shader.resource->uniforms[block].byte_size))
{}

UBO::UBO(ShaderProgram shader, const std::string& block, UBO::BufferTy data)
	: UBO(shader.resource->uniforms[block], data)
{}

UBO::UBO(UniformBlock block, BufferTy data_)
	: data(std::move(data_))
	, bufferObject(this->data.size())
	, type(block.name == "Common" ? UBO::Common : UBO::Material)
	, block(block)
{
	if (block.byte_size != data.size())
		std::cerr << "Warning: Shader wants " << block.byte_size << " bytes for '"
			<< block.name << "' but there are " << data.size() << '\n';

	Sync();
}

UBO::UBO(UBO&& other)
	: data(std::move(other.data))
	, bufferObject(std::move(other.bufferObject))
	, type(other.type)
	, block(other.block)
{}

void UBO::Bind() const
{
	bufferObject.Bind(type);
}

UBO::BindProxy UBO::GetBindProxy() const
{
	return bufferObject.GetIndexedBindProxy(type);
}

const UBO::BufferTy& UBO::Data() const
{
	return data;
}

std::vector<Uniform> UBO::Uniforms() const
{
	return block.uniforms;
}

UBO::Proxy UBO::operator[](const std::string& name)
{
	return Proxy(*this, block[name]);
}

void UBO::Sync()
{
	bufferObject.Data(data);
}

void UBO::Proxy::CheckType(GLenum scalar, std::ptrdiff_t Rows, std::ptrdiff_t Cols) const
{
	if (unif.type.scalar != scalar)
		throw std::domain_error("Wrong type for uniform '" + ubo.block.name
			+ '.' + unif.name);
	if (unif.type.rows != Rows || unif.type.cols != Cols)
		throw std::runtime_error("Wrong dims for uniform '" + ubo.block.name
			+ '.' + unif.name);
}

#define PROXY_PTR(Type) \
	Type* UBO::Proxy::Ptr(Type) const \
	{\
		return reinterpret_cast<Type*>(ubo.data.data() + unif.offset);\
	}

PROXY_PTR(float)
PROXY_PTR(int)
PROXY_PTR(unsigned int)

UBO::Proxy::operator uint32_t() const
{
	Eigen::Matrix<uint32_t, 1, 1> mat = *this;
	return mat(1,1);
}

UBO::Proxy& UBO::Proxy::operator=(const uint32_t& v)
{
	Eigen::Matrix<uint32_t, 1, 1> mat;
	mat << v;
	return *this = mat;
}

UBO::Proxy UBO::Proxy::operator[](GLuint offset)
{
	if (unif.size <= offset)
		throw std::domain_error(std::to_string(offset) + " is is past the end of " + unif.name);

	Uniform offsetted = unif;
	offsetted.offset = unif.offset + unif.stride * offset;
	return Proxy(ubo, offsetted);
}