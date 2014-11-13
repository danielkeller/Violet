#ifndef ARRAY_BUFFER_HPP
#define ARRAY_BUFFER_HPP
#include <vector>
#include <iostream>
#include "Shader.hpp"

struct AttribProperties
{
	std::string name;
	GLint numComponents;
	GLenum glType;
	size_t offset;
	GLint numMatrixComponents;
	size_t matrixStride;
};
typedef std::vector<AttribProperties> Schema;

template<class T>
class ArrayBuffer
{
public:
	ArrayBuffer()
		: data_len(0), bufferObject(0)
	{}

	ArrayBuffer(size_t size, GLenum usage)
		: data_len(size), usage(usage)
	{
		glGenBuffers(1, &bufferObject);
		glBindBuffer(GL_ARRAY_BUFFER, bufferObject);
		glBufferData(GL_ARRAY_BUFFER, data_len, nullptr, usage);
	}

	template<class Alloc>
	ArrayBuffer(const std::vector<T, Alloc>& data, GLenum usage)
		: ArrayBuffer(data.size()*sizeof(T), usage)
	{
		glBufferSubData(GL_ARRAY_BUFFER, 0, data_len, data.data());
	}

	ArrayBuffer(ArrayBuffer<T>&& other)
		: data_len(other.data_len), bufferObject(other.bufferObject), usage(other.usage)
	{
		other.bufferObject = 0;
	}

	~ArrayBuffer()
	{
		glDeleteBuffers(1, &bufferObject);
	}

	ArrayBuffer(const ArrayBuffer&) = delete;
	void operator=(ArrayBuffer other)
	{
		data_len = other.data_len;
		bufferObject = other.bufferObject;
		usage = other.usage;
		other.bufferObject = 0;
	}

	//perform type erasure
	template<typename U>
	friend ArrayBuffer<char> EraseType(ArrayBuffer<U>&&);

	template<class Alloc>
	void Data(const std::vector<T, Alloc>& data)
	{
		glBindBuffer(GL_ARRAY_BUFFER, bufferObject);
		if (data.size()*sizeof(T) != data_len)
		{
			data_len = data.size()*sizeof(T);
			glBufferData(GL_ARRAY_BUFFER, data_len, data.data(), usage);
		}
		else
			glBufferSubData(GL_ARRAY_BUFFER, 0, data_len, data.data());
	}

	void SubData(size_t offs, const T& data)
	{
		glBindBuffer(GL_ARRAY_BUFFER, bufferObject);
		glBufferSubData(GL_ARRAY_BUFFER, offs, sizeof(T), &data);
	}

	void BindToShader(const ShaderProgram& program)
	{
		glBindBuffer(GL_ARRAY_BUFFER, bufferObject);
		for (const auto& props : schema)
		{
			//enable generic attribute array vertAttrib in the current vertex array object (VAO)
			GLint vertAttrib = program.GetAttribLocation(props.name.c_str());
			if (vertAttrib == -1)
			{
				std::cerr << "Warning: Vertex attribute '" << props.name << "' is not defined or active in '"
					<< program.Name() << "'\n";
				continue;
			}
			
			//GL makes you specify matrices in this goofball way
			for (int offs = 0; offs < props.numMatrixComponents; ++offs)
			{
				glEnableVertexAttribArray(vertAttrib + offs);

				//associate the buffer data bound to GL_ARRAY_BUFFER with the attribute in index 0
				//the final argument to this call is an integer offset, cast to pointer type. don't ask me why.
				glVertexAttribPointer(
					vertAttrib + offs, props.numComponents, props.glType, GL_FALSE, sizeof(T),
					static_cast<const char*>(nullptr) + props.offset + offs*props.matrixStride);

				//This attribute is hardwired for instancing
				if (props.name == "transform")
					glVertexAttribDivisor(vertAttrib + offs, 1);
			}
		}
	}

private:
	//Must be specialized by class user
	static Schema schema;
	size_t data_len;
	GLuint bufferObject;
	GLenum usage;
};

template<class T, class Alloc>
class MutableArrayBuffer
{
	struct MABResource;
public:
	MutableArrayBuffer()
		: resource(std::make_shared<MABResource>())
	{}

	std::vector<T, Alloc>& Vector() { return resource->data; }
	const std::vector<T, Alloc>& Vector() const { return resource->data; }

	void Sync() const { resource->arrayBuf.Data(resource->data); }

	void BindToShader(const ShaderProgram& program) { resource->arrayBuf.BindToShader(program); }

	bool operator==(const MutableArrayBuffer& other) const { return resource == other.resource; }
	bool operator<(const MutableArrayBuffer& other) const { return resource < other.resource; }

private:
	mutable std::shared_ptr<MABResource> resource;

	struct MABResource
	{
		MABResource()
			: arrayBuf(0, GL_DYNAMIC_DRAW)
		{}
		ArrayBuffer<T> arrayBuf;
		std::vector<T, Alloc> data;
	};
};

template<class T>
inline ArrayBuffer<char> EraseType(ArrayBuffer<T>&& other)
{
	ArrayBuffer<char> ret;
	ret.bufferObject = other.bufferObject;
	ret.data_len = other.data_len;
	ret.usage = other.usage;
	other.bufferObject = 0;
	return std::move(ret);
}

#endif
