namespace Render_detail
{
	struct InstData
	{
		Matrix4f mat;
        Object obj;
        
		InstData(const InstData&) = default;
        InstData(Object o) : mat(), obj(o) {}
        InstData() : mat(), obj(Object::invalid) {}
		InstData& operator=(const Matrix4f& m) { mat = m; return *this; }
        
        MEMBER_EQUALITY(Object, obj);
	};

    using InstanceVec = l_bag<InstData, Eigen::aligned_allocator<InstData>>;

	struct Shape
	{
		VAO vao;
        std::array<std::unordered_set<ShaderProgram>::iterator, NumPasses> passShader;
        std::array<std::unordered_set<Material>::iterator, NumPasses> passMaterial;

		Shape(const std::tuple<ShaderProgram&, VertexData&>& tuple)
			: vao(std::get<0>(tuple), std::get<1>(tuple))
		{}

		Shape(const Shape&) = delete;
		Shape(Shape&& other) //MSVC sucks and can't default this
            : vao(std::move(other.vao))
            , passShader(std::move(other.passShader))
            , passMaterial(std::move(other.passMaterial))
		{}

		Shape& operator=(Shape other)
		{
			swap(*this, other);
			return *this;
		}

		friend void swap(Shape& l, Shape& r)
		{
			swap(l.vao, r.vao);
			swap(l.passShader, r.passShader);
			swap(l.passMaterial, r.passMaterial);
		}

		MEMBER_EQUALITY(VertexData, vao)
			bool operator==(const std::tuple<ShaderProgram&, VertexData&>& tuple) const
		{
			return vao == std::get<1>(tuple);
		}

		bool operator!=(const std::tuple<ShaderProgram&, VertexData&>& tuple) const
		{
			return vao != std::get<1>(tuple);
		}
	};
}
