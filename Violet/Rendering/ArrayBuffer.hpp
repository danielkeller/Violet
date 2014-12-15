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

	template<class Container>
	ArrayBuffer(const Container& data, GLenum usage,
		typename Container::size_type sfinae = 0)
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

	template<class Container>
	void Data(const Container& data)
	{
		Bind();
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
		Bind();
		glBufferSubData(GL_ARRAY_BUFFER, offs, sizeof(T), &data);
	}

	//Is this bad?
	void Bind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, bufferObject);
	}

	//Must be specialized by class user
	static const Schema schema;
private:
	size_t data_len;
	GLuint bufferObject;
	GLenum usage;
};

template<class T>
class MutableArrayBuffer
{
	struct MABResource;
public:
	using container = T;

	MutableArrayBuffer()
		: data(std::make_shared<container>())
		, arrayBuf(0, GL_DYNAMIC_DRAW)
	{}
	MutableArrayBuffer(const MutableArrayBuffer& other)
		: arrayBuf(*other.data, GL_DYNAMIC_DRAW),
		, data(std::make_shared<container>(*other.data))
	{}
	MutableArrayBuffer(MutableArrayBuffer&& other)
		: arrayBuf(std::move(other.arrayBuf)), data(std::move(other.data))
	{}

	container& Container() { return *data; }
	const container& Container() const { return *data; }
	std::shared_ptr<container> ContainerPtr() { return data; }

	void Sync() const { arrayBuf.Data(*data); }
	ArrayBuffer<typename container::value_type>& ArrayBuf()
		{ return arrayBuf; }

private:
	mutable ArrayBuffer<typename container::value_type> arrayBuf;
	std::shared_ptr<container> data;
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
