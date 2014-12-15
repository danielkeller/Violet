#ifndef VAO_HPP
#define VAO_HPP

#include "ArrayBuffer.hpp"
#include "Geometry/Mesh.hpp"
#include "Permavector.hpp"

#include <vector>
#include <memory>

class ShaderProgram;
class Mesh;

struct LineInd
{
	GLint a, b;
	static const int dim = 2;
	static const GLenum mode = GL_LINES;
};

class ConstVAO
{
protected:
	struct VAOResource;
	class VAOBinding;

public:
	//initialize to invalid state
	ConstVAO()
		: vertexArrayObject(0)
		, resource(nullptr)
	{}

	//This is only an optimization
	VAOBinding bind() const
	{ return vertexArrayObject;	}

	void draw(GLsizei instances) const;

	bool operator==(const ConstVAO& other) const
	{
		return vertexArrayObject == other.vertexArrayObject;
	}
	bool operator!=(const ConstVAO& other) const
	{
		return !(*this == other);
	}

	using InstanceBuf = Permavector<Matrix4f, Eigen::aligned_allocator<Matrix4f>>;

	const MutableArrayBuffer<InstanceBuf>& InstanceBuffer() const
	{ return resource->instanceBuffer; }

protected:
	GLuint vertexArrayObject;
	GLsizei numVertecies;
	GLenum mode;
	std::shared_ptr<VAOResource> resource;

	ConstVAO(std::shared_ptr<VAOResource> ptr)
		: vertexArrayObject(ptr->vertexArrayObject)
		, numVertecies(ptr->numVertecies)
		, mode(ptr->mode)
		, resource(ptr)
	{}

	friend class AABB; // std::tuple<VAO, ShaderProgram> AABB::Show();

	class VAOBinding
	{
	public:
		~VAOBinding();
	private:
		VAOBinding(GLuint next);
		GLuint prev;
		static GLuint current;
		friend class ConstVAO;
	};

	struct VAOResource : public Resource<VAOResource>
	{
		VAOResource(const VAOResource&) = delete;
		VAOResource(VAOResource&&);

		//clear VAO's resources
		~VAOResource();

		template<class T, class I>
		VAOResource(
			const std::string name,
			const ShaderProgram& program,
			ArrayBuffer<T>&& data,
			const std::vector<I>& indices)
			: ResourceTy(name), vertexBuffer()
		{
			glGenVertexArrays(1, &vertexArrayObject);
			glBindVertexArray(vertexArrayObject);

			//gets saved in the VAO
			BindArrayBufToShader(program, data);
			vertexBuffer = EraseType(std::move(data));
			BufferIndices(indices);
			BindArrayBufToShader(program, instanceBuffer.ArrayBuf());
		}

		template<class I>
		void BufferIndices(const std::vector<I>& indices)
		{
			glGenBuffers(1, &indexBufferObject);

			//GL_ELEMENT_ARRAY_BUFFER binding is part of the current VAO state, therefore we do not unbind it
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferObject);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				indices.size()*sizeof(GLint) * I::dim, indices.data(), GL_STATIC_DRAW);

			numVertecies = static_cast<GLsizei>(indices.size() * I::dim);
			mode = I::mode;
		}

		template<class T>
		void BindArrayBufToShader(const ShaderProgram& program, const ArrayBuffer<T>& buf)
		{
			buf.Bind();
			for (const auto& props : buf.schema)
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

		//the GL vertex array object assocated with this object
		GLuint vertexArrayObject;

		//GL buffer objects for vertex and vertex index data
		GLuint indexBufferObject;
		ArrayBuffer<char> vertexBuffer;
		MutableArrayBuffer<ConstVAO::InstanceBuf> instanceBuffer;

		//how many vertex indices we have
		GLsizei numVertecies;

		//draw mode
		GLenum mode;
	};
};

class VAO : public ConstVAO
{
public:
	VAO() = default;
	VAO(std::shared_ptr<VAOResource> ptr)
		: ConstVAO(ptr) {}

	MutableArrayBuffer<InstanceBuf>& InstanceBuffer() { return resource->instanceBuffer; }
	using ConstVAO::InstanceBuffer;

	friend std::tuple<VAO, Mesh, ShaderProgram> LoadWavefront(std::string filename);
};

#endif