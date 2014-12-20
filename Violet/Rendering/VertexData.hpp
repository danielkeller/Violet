#ifndef VERTEX_DATA_HPP
#define VERTEX_DATA_HPP

#include "Resource.hpp"
#include "BufferObject.hpp"

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

//TODO: just make it an inner class
namespace VertexData_detail
{
struct VertexDataResource : public Resource<VertexDataResource>
{
    template<class V, class VAlloc, class I, class IAlloc>
    VertexDataResource(const std::string& name,
        const std::vector<V, VAlloc>& verts, const std::vector<I, IAlloc>& inds)
        : ResourceTy(name), vertexBufferSchema(V::schema),
        vertexBufferStride(sizeof(V)), mode(I::mode),
        indexBuffer(inds, IgnoreType),
        vertexBuffer(verts, IgnoreType)
    {
        numVertecies = indexBuffer.Size();
    }

    Schema vertexBufferSchema;
    GLsizei vertexBufferStride;
    GLenum mode;
	GLsizei numVertecies;
    BufferObject<GLint, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW> indexBuffer;
    BufferObject<char, GL_ARRAY_BUFFER, GL_STATIC_DRAW> vertexBuffer;
};
}

class VertexData
{
public:
    VertexData() = default;
    BASIC_EQUALITY(VertexData, resource)
private:
    std::shared_ptr<VertexData_detail::VertexDataResource> resource;
    VertexData(std::shared_ptr<VertexData_detail::VertexDataResource> r)
        : resource(r) {}
    friend struct Wavefront;
    friend class VAO;
    friend struct ShowAABB;
};

#endif
