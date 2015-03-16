namespace Render_detail
{
	struct InstData
	{
		Matrix4f mat;
        Object obj;
        
		InstData(const InstData&) = default;
		InstData(Object o) : mat(), obj(o) {}
		InstData(Object o, const Matrix4f& m) : mat(m), obj(o) {}
        InstData() : mat(), obj(Object::invalid) {}
		InstData& operator=(const Matrix4f& m) { mat = m; return *this; }
        
        MEMBER_EQUALITY(Object, obj);
		BASIC_EQUALITY(InstData, obj);
	};

    using InstanceVec = l_bag<InstData, Eigen::aligned_allocator<InstData>>;
}
