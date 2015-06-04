#ifndef VERTEX_DATA_HPP
#define VERTEX_DATA_HPP

#include "Core/Resource.hpp"
#include "BufferObject.hpp"
#include "Containers/WrappedIterator.hpp"

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

struct WireCubeT {};
static WireCubeT WireCube;

class VertexData
{
public:
    VertexData(UnitBoxT);
	VertexData(WireCubeT);
    
	VertexData(const std::string& file);
	VertexData(const char* file) : VertexData(std::string(file)) {}

    template<class V, class VAlloc, class I, class IAlloc>
    VertexData(const std::string& name,
		const std::vector<V, VAlloc>& verts, const std::vector<I, IAlloc>& inds,
		bool cache = false)
    {
        resource = VertexDataResource::FindResource(name);
        if (!resource)
            resource = VertexDataResource::MakeShared(name, verts, inds);

		if (cache)
			resource->WriteCache(
				{reinterpret_cast<const char*>(verts.data()),
				reinterpret_cast<const char*>(verts.data() + verts.size())},
				{reinterpret_cast<const char*>(inds.data()),
				reinterpret_cast<const char*>(inds.data() + inds.size())});
	}

	BASIC_EQUALITY(VertexData, resource)

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

		//read from cache
		VertexDataResource(const std::string& name);

		void WriteCache(range<const char*> verts, range<const char*> inds);

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
