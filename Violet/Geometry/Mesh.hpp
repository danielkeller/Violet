#ifndef MESH_HPP
#define MESH_HPP

using vectorVector3f = std::vector<Vector3f, Eigen::aligned_allocator<Vector3f>>;

#include "Core/Resource.hpp"
#include "Shapes.hpp"
#include "Containers/WrappedIterator.hpp"

class VAO;
class ShaderProgram;

struct LineInd
{
	GLint a, b;

	static const GLenum mode = GL_LINES;
};

struct TriInd
{
	GLint a, b, c;

	static const GLenum mode = GL_TRIANGLES;
};

struct TriProxy
{
	vectorVector3f& points;
	TriInd& tri;
	TriProxy(TriInd& tri_, vectorVector3f& points_)
		: points(points_), tri(tri_)
	{}
	TriProxy& operator=(TriProxy& o) { tri = o.tri; return *this; }
	TriProxy& operator=(Triangle& o)
		{ points[tri.a] = o.q; points[tri.b] = o.r; points[tri.c] = o.s; return *this; }
	operator Triangle() const { return{ points[tri.a], points[tri.b], points[tri.c] }; }
};
//hack hack
struct ConstTriProxy
{
	vectorVector3f& points;
	const TriInd& tri;
	ConstTriProxy(const TriInd& tri_, vectorVector3f& points_)
		: points(points_), tri(tri_)
	{}
	operator Triangle() const { return{ points[tri.a], points[tri.b], points[tri.c] }; }
	Triangle Triangle() const { return *this; }
};

/*
template<>
inline void std::swap(TriProxy& l, TriProxy& r)
{
	std::swap(l.a, r.a);
	std::swap(l.b, r.b);
	std::swap(l.c, r.c);
}*/

template<class IterTy, class ValueTy>
class MeshIterBase : public WrappedIterator<MeshIterBase<IterTy, ValueTy>, IterTy, ValueTy>
{
    using Base = WrappedIterator<MeshIterBase<IterTy, ValueTy>, IterTy, ValueTy>;
public:
	typename Base::value_type operator*() { return typename Base::value_type(*Base::it, points); }

	MeshIterBase& operator=(MeshIterBase other) { std::swap(Base::it, other.it); return *this; }
	friend void swap(MeshIterBase&, MeshIterBase&);

	//operator ConstMeshIter() { return{ it, points }; }

private:
	vectorVector3f& points;
	MeshIterBase(IterTy it, vectorVector3f& points)
		: Base(it), points(points)
	{}
	friend class Mesh;
};

using MeshIter = MeshIterBase < std::vector<TriInd>::iterator, TriProxy >;
using ConstMeshIter = MeshIterBase < std::vector<TriInd>::const_iterator, ConstTriProxy >;

class Mesh
{
public:
	Mesh(std::string file);
	Mesh(std::string name, vectorVector3f p, std::vector<TriInd> i);

	//the non-const functions assume you will be modifying the mesh,
	//so they copy the data.
	MeshIter begin();
	ConstMeshIter cbegin() const { return begin(); }
	ConstMeshIter begin() const;
	MeshIter end();
	ConstMeshIter cend() const { return end(); }
	ConstMeshIter end() const;

	AlignedBox3f bound() const;

	size_t size() { return submesh.size() ? submesh.size() : resource->indices.size(); }

	void erase(MeshIter first, MeshIter last) { submesh.erase(first.it, last.it); }

	//Remove all triangles not touching box
	//TODO: return resultant bounding box
	void Chop(AlignedBox3f box);

	void PrintDataSize();
	
private:
	Mesh() = default;

	struct MeshResource : public Resource<MeshResource>
	{
		MeshResource(std::string name, vectorVector3f p, std::vector<TriInd> i)
			: ResourceTy(name), points(p), indices(i)
		{}
		vectorVector3f points;
		std::vector<TriInd> indices;
	};
	
	Mesh(std::shared_ptr<MeshResource> ptr)
		: resource(ptr)
		, submesh()
	{}

	std::shared_ptr<MeshResource> resource;
	std::vector<TriInd> submesh;

	friend struct Wavefront;
};

#endif
