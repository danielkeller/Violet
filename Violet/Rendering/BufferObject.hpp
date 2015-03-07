#ifndef BUFFER_OBJECT_HPP
#define BUFFER_OBJECT_HPP

struct IgnoreTypeT {};
static IgnoreTypeT IgnoreType;

template<class T, GLenum target, GLenum usage>
class BufferObject
{
public:
    ~BufferObject()
    {
        glDeleteBuffers(1, &buffer);
    }

    BufferObject(const BufferObject&) = delete;
	BufferObject(BufferObject&& other)
		: BufferObject(std::move(other), IgnoreType)
	{}

	template<class U, GLenum target1, GLenum usage1>
	BufferObject(BufferObject<U, target1, usage1>&& other, IgnoreTypeT)
		: buffer(other.buffer), byte_len(other.byte_len)
	{
		other.buffer = 0;
	}

    BufferObject()
        : byte_len(0)
    {
        glGenBuffers(1, &buffer);
    }

    BufferObject(size_t size)
        : byte_len(size*sizeof(T))
    {
        glGenBuffers(1, &buffer);
        Bind();
        glBufferData(target, byte_len, nullptr, usage);
    }

    template<class U, class Alloc>
    BufferObject(const std::vector<U, Alloc>& data, IgnoreTypeT)
        : BufferObject()
    {
        Data(data, IgnoreType);
    }

    template<class Alloc>
    BufferObject(const std::vector<T, Alloc>& data)
        : BufferObject(data, IgnoreType) {}

	friend void swap(BufferObject& first, BufferObject& second)
	{
		swap(first.byte_len, second.byte_len);
		swap(first.buffer, second.buffer);
	}

	BufferObject& operator=(BufferObject other)
	{
		swap(*this, other);
		return *this;
	}

    template<class U, class Alloc>
    void Data(const std::vector<U, Alloc>& data, IgnoreTypeT)
    {
        Bind();
        //This appears to be slower. TODO: find out exactly when it is
        if (false) //(byte_len == data.size()*sizeof(U))
            glBufferSubData(target, 0, byte_len, data.data());
        else
        {
            byte_len = data.size()*sizeof(U);
            glBufferData(target, byte_len, data.data(), usage);
        }
    }

    template<class Alloc>
    void Data(const std::vector<T, Alloc>& data)
    {
        Data(data, IgnoreType);
    }

    //consider using vector-style allocation (round to nearest power of two)
    void Data(size_t size)
    {
        if (byte_len != size*sizeof(T))
        {
            byte_len = size*sizeof(T);
            Bind();
            glBufferData(target, byte_len, nullptr, usage);
        }
    }

	void Assign(size_t pos, const T& data)
	{
		Bind();
		glBufferSubData(target, pos*sizeof(T), sizeof(T), &data);
	}

	void Insert(size_t pos, const T& data)
	{
		auto newbuf = CopySteal(Size() + 1);
		
		Bind(); newbuf.Bind();
		size_t offset = pos*sizeof(T);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, target, 0, 0, offset); //before chunk
		glCopyBufferSubData(GL_COPY_READ_BUFFER, target, offset,
			offset + sizeof(T), byte_len - offset); //after chunk
		glBufferSubData(target, offset, sizeof(T), &data);
	}

	void Erase(size_t pos)
	{
		auto newbuf = CopySteal(Size() - 1);

		Bind(); newbuf.Bind();
		size_t offset = pos*sizeof(T);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, target, 0, 0, offset); //before chunk
		glCopyBufferSubData(GL_COPY_READ_BUFFER, target, offset + sizeof(T),
			offset, byte_len - offset - sizeof(T)); //after chunk
	}

    void Bind() const
    {
        glBindBuffer(target, buffer);
    }

    size_t Size() const
    {
        return byte_len / sizeof(T);
    }

    class IndexedBindProxy
    {
    public:
        void Bind() const
        {
            glBindBufferBase(target, index, buffer);
        }

        IndexedBindProxy(GLuint index)
            : index(index), buffer(0) {}

        BASIC_EQUALITY(IndexedBindProxy, buffer)

    private:
        GLuint index, buffer;
        IndexedBindProxy(GLuint index, GLuint buffer)
            : index(index), buffer(buffer) {}
        friend class BufferObject;
    };

    IndexedBindProxy GetIndexedBindProxy(GLuint index)
    {
        static_assert(target == GL_UNIFORM_BUFFER, "Only uniform buffers can be bound to an index");
        return {index, buffer};
    }

    class Mapping
    {
    public:
        ~Mapping()
        {
            glBindBuffer(target, buffer);
            glUnmapBuffer(target);
        }
        T* begin() {return begin_ptr;}
        T* end() {return end_ptr;}
    private:
        GLuint buffer;
        T *begin_ptr, *end_ptr;
        Mapping(GLuint buf, T* b, T* e) : buffer(buf), begin_ptr(b), end_ptr(e) {}
        friend class BufferObject;
    };

    Mapping Map(GLenum access)
    {
        Bind();
        T* begin = static_cast<T*>(glMapBuffer(target, access));
        return {buffer, begin, begin + Size()};
    }

private:
    GLuint buffer;
    size_t byte_len;

	//returns a buffer with our data in it, and resizes us to size
	BufferObject<T, GL_COPY_READ_BUFFER, usage> CopySteal(size_t size)
	{
		static_assert(target != GL_COPY_READ_BUFFER);
		BufferObject newbuf(size);
		//now steal the data
		swap(*this, newbuf);
		//convert the type
		return BufferObject(std::move(newbuf), IgnoreType);
	}
};

#endif
