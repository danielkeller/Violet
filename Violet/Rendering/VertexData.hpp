#ifndef VERTEX_DATA_HPP
#define VERTEX_DATA_HPP

#include "Core/Resource.hpp"
#include "BufferObject.hpp"

struct ResourcePersistTag;

struct AttribProperties
{
	std::string name;
	GLenum glType;
    bool integer;
    size_t offset;
    Eigen::Vector2i dims; //(rows, cols)
	size_t matrixStride;
};
typedef std::vector<AttribProperties> Schema;

template<class T>
struct AttribTraits
{
    static const Schema schema;
}; //will cause linker errors if not specialized

struct UnitBoxT {};
static UnitBoxT UnitBox;

class VertexData
{
public:
    VertexData(UnitBoxT);
    
	VertexData(const std::string& file);

    template<class V, class VAlloc, class I, class IAlloc>
    VertexData(const std::string& name, const std::vector<V, VAlloc>& verts, const std::vector<I, IAlloc>& inds)
    {
        resource = VertexDataResource::FindResource(name);
        if (!resource)
            resource = VertexDataResource::MakeShared(name, verts, inds);
	}

	BASIC_EQUALITY(VertexData, resource);

	std::string Name() const;

	using PersistCategory = ResourcePersistTag;

private:
    struct VertexDataResource : public Resource<VertexDataResource>
    {
        template<class V, class VAlloc, class I, class IAlloc>
        VertexDataResource(const std::string& name,
            const std::vector<V, VAlloc>& verts, const std::vector<I, IAlloc>& inds)
            : ResourceTy(name), vertexBufferSchema(AttribTraits<V>::schema),
            vertexBufferStride(sizeof(V)), mode(I::mode),
            indexBuffer(inds, IgnoreType),
            vertexBuffer(verts, IgnoreType)
        {
            numVertecies = static_cast<GLsizei>(indexBuffer.Size());
        }

        Schema vertexBufferSchema;
        GLsizei vertexBufferStride;
        GLenum mode;
        GLsizei numVertecies;
        BufferObject<GLint, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW> indexBuffer;
        BufferObject<char, GL_ARRAY_BUFFER, GL_STATIC_DRAW> vertexBuffer;
    };

    VertexData() = default;
    std::shared_ptr<VertexDataResource> resource;
    VertexData(std::shared_ptr<VertexDataResource> r)
        : resource(r) {}
    friend struct Wavefront;
    friend class VAO;
    friend struct ShowAABB;

	HAS_HASH
};

MEMBER_HASH(VertexData, resource)

#endif
