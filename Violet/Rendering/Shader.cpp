#include "stdafx.h"
#include "Shader.hpp"
#include <iostream>
#include <fstream>
#include <vector>

struct Uniforms
{
	Uniforms() = default;
	Uniforms(GLuint program);

	struct Uniform
	{
		std::string name;
		GLenum type;
		GLuint size;
		GLuint offset;
		GLint stride;
		GLint location;
	};

	struct Block
	{
		std::string name;
		size_t byte_size;
		std::vector<Uniform> uniforms;

		const Uniform& operator[](const std::string& name) const;
		Uniform& operator[](const std::string& name)
		{
			return const_cast<Uniform&>(const_cast<const Block&>(*this)[name]);
		}
	};
	std::vector<Block> blocks;

	const Block& operator[](const std::string& name) const;
	Block& operator[](const std::string& name)
	{
		return const_cast<Block&>(const_cast<const Uniforms&>(*this)[name]);
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
	void init(std::istream &vert, std::istream &frag, const std::string& vertName, const std::string& fragName);

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

struct UBO::UBOResource : public Resource<UBOResource>
{
    std::vector<BufferTy> data;
	BufferObjTy bufferObject;
	//Type allows us to distinguish between UBOs that are shared (ie, camera)
	//with those that are not (ie, material).
	Type type;
	Uniforms::Block block;
	const Uniforms::Block& Block() { return block; }
	UBOResource(const std::string& name, const Uniforms::Block& block);
};

ShaderProgram::ShaderResource::ShaderResource(std::string path)
	: ResourceTy(path)
{
    std::ifstream vert(path + ".vert");
    std::ifstream frag(path + ".frag");
    init(vert, frag, path + ".vert", path + ".frag");
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

UBO ShaderProgram::MakeUBO(const std::string& block, const std::string& name) const
{
    std::string qualName = block + "#" + name;
	auto res = UBO::UBOResource::FindResource(qualName);
	if (res)
		return res;
	return UBO::UBOResource::MakeShared(qualName, resource->uniforms[block]);
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

GLuint CreateShader(GLenum eShaderType, std::istream &t, const std::string& name)
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
    if (!status) //compile failed
    {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::vector<GLchar> infoLog(infoLogLength + 1);

        //print error message
        glGetShaderInfoLog(shader, infoLogLength, NULL, infoLog.data());
        throw std::runtime_error(name + ':' + infoLog.data());
    }

    return shader;
}

void ShaderProgram::ShaderResource::init(std::istream &vert, std::istream &frag,
    const std::string& vertName, const std::string& fragName)
{
    //create our empty program Render
    program = glCreateProgram();

    GLuint vertShdr = CreateShader(GL_VERTEX_SHADER, vert, vertName);
    GLuint fragShdr = CreateShader(GL_FRAGMENT_SHADER, frag, fragName);

    //attach vertex and fragment shaders
    glAttachShader(program, vertShdr);
    glAttachShader(program, fragShdr);

    for (const auto& pair : standardAttribs)
        glBindAttribLocation(program, pair.second, pair.first.c_str());
    
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
        throw std::runtime_error("Linker failure: " + Name() + infoLog.data());
    }
	else
		uniforms = Uniforms(program);

    //shaders are no longer used now that the program is linked
    glDetachShader(program, vertShdr);
    glDeleteShader(vertShdr);
    glDetachShader(program, fragShdr);
    glDeleteShader(fragShdr);
}

std::vector<Uniforms::Block> DoQuery(GLuint program)
{
	GLint activeUniforms;
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &activeUniforms);

	GLint activeUniformBlocks;
	glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCKS, &activeUniformBlocks);
	std::vector<Uniforms::Block> ret(activeUniformBlocks + 1);
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
		Uniforms::Uniform unif;
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
		unif.name.assign(name.begin(), name.end() - 1); //remove null

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

const Uniforms::Uniform& Uniforms::Block::operator[](const std::string& n) const
{
	auto it = std::find_if(uniforms.begin(), uniforms.end(), [&n](const Uniforms::Uniform& u)
	{return u.name == n; });
	if (it == uniforms.end())
        throw std::runtime_error("No uniform '" + n + "' in block '" + name + "'");
	return *it;
}

const Uniforms::Block& Uniforms::operator[](const std::string& n) const
{
	auto it = std::find_if(blocks.begin(), blocks.end(), [&n](const Uniforms::Block& u)
	{return u.name == n; });
	if (it == blocks.end())
        throw std::runtime_error("No block '" + n + "'");
	return *it;
}

UBO::UBOResource::UBOResource(const std::string& name, const Uniforms::Block& block)
	: Resource(name), data(block.byte_size)
    , bufferObject(block.byte_size)
	, type(block.name == "Common" ? UBO::Common : UBO::Material), block(block)
{}

UBO::UBO(std::shared_ptr<UBOResource> r)
	: bindProxy(r->bufferObject.GetIndexedBindProxy(r->type))
	, resource(r)
{}

void UBO::Sync() const
{
	resource->bufferObject.Data(resource->data);
}

void UBO::Bind() const
{
	bindProxy.Bind();
}

template<typename T, GLenum ty>
T UBO::Proxy::ConvertOpHelper() const
{
	const Uniforms::Uniform& unif = ubo.resource->Block()[name];
	assert(unif.type == ty);
	return Eigen::Map<const T>(reinterpret_cast<const typename T::Scalar*>(
        ubo.resource->data.data() + unif.offset));
}

template<GLenum ty, typename T>
UBO::Proxy& UBO::Proxy::AssignOpHelper(const T& val)
{
	const Uniforms::Uniform& unif = ubo.resource->Block()[name];
	assert(unif.type == ty);
	Eigen::Map<T>(reinterpret_cast<typename T::Scalar*>(
        ubo.resource->data.data() + unif.offset))
        = val;
	return *this;
}

template<typename T, GLenum ty>
T UBO::Proxy::ScalarConvertOpHelper() const
{
	const Uniforms::Uniform& unif = ubo.resource->Block()[name];
	assert(unif.type == ty);
	return *reinterpret_cast<const T*>(ubo.resource->data.data() + unif.offset);
}

template<GLenum ty, typename T>
UBO::Proxy& UBO::Proxy::ScalarAssignOpHelper(const T& val)
{
	const Uniforms::Uniform& unif = ubo.resource->Block()[name];
	assert(unif.type == ty);
	*reinterpret_cast<T*>(ubo.resource->data.data() + unif.offset) = val;
	return *this;
}

UBO::Proxy::operator Matrix3f() const
{
	return ConvertOpHelper<Matrix3f, GL_FLOAT_MAT3>();
}

UBO::Proxy& UBO::Proxy::operator=(const Matrix3f& v)
{
	return AssignOpHelper<GL_FLOAT_MAT3>(v);
}

UBO::Proxy::operator Matrix4f() const
{
	return ConvertOpHelper<Matrix4f, GL_FLOAT_MAT4>();
}

UBO::Proxy& UBO::Proxy::operator=(const Matrix4f& v)
{
	return AssignOpHelper<GL_FLOAT_MAT4>(v);
}

UBO::Proxy::operator uint32_t() const
{
	return ScalarConvertOpHelper<uint32_t, GL_UNSIGNED_INT>();
}

UBO::Proxy& UBO::Proxy::operator=(const uint32_t& v)
{
	return ScalarAssignOpHelper<GL_UNSIGNED_INT>(v);
}
