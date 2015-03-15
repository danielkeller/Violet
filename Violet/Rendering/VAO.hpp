#ifndef VAO_HPP
#define VAO_HPP

#include "BufferObject.hpp"
#include "Rendering/VertexData.hpp"

class ShaderProgram;

class VAO
{
public:
	VAO(const VAO&) = delete;
	VAO(VAO&&);
	~VAO();

	VAO(const ShaderProgram& program, const VertexData& vertdata);

	VAO& operator=(VAO v);

	friend void swap(VAO& l, VAO& r);

	class Binding
	{
	public:
		~Binding();
	private:
		Binding(GLuint next);
		GLuint prev;
		static GLuint current;
		friend class VAO;
	};

	//This is only an optimization
	Binding Bind() const { return vertexArrayObject; }

	void Draw() const;

	template<class T, GLenum usage>
	void BindInstanceData(const ShaderProgram& program,
		const BufferObject<T, GL_ARRAY_BUFFER, usage>& buf,
		GLsizei offset, GLsizei len)
	{
		auto bound = Bind();
		buf.Bind();
		BindArrayBufToShader(program, AttribTraits<T>::schema, sizeof(T), offset, true);
		numInstances = len;
	}

	GLsizei NumInstances() const { return numInstances; }
	VertexData GetVertexData() const { return vertexData; }

	MEMBER_EQUALITY(VertexData, vertexData)

private:
	void BindArrayBufToShader(const ShaderProgram& program, const Schema& schema,
		GLsizei stride, GLsizei offset = 0, bool instanced = false);

	//the GL vertex array object assocated with this object
	GLuint vertexArrayObject;

	//GL buffer objects for vertex and vertex index data
	VertexData vertexData;

	//how many vertex indices we have
	GLsizei numVertecies;

	//how many instances we have
	GLsizei numInstances;

	//draw mode
	GLenum mode;
};

#endif
