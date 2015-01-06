namespace Render_detail
{
    struct Shape;

	struct T_Material
	{
        Material mat;
        l_bag<Shape>::perma_ref begin;

        MEMBER_EQUALITY(Material, mat)
		
        T_Material& operator=(T_Material&&) = default;

		T_Material(Material mat, l_bag<Shape>::perma_ref begin)
			: mat(mat)
            , begin(begin)
		{}
		T_Material(const T_Material&) = delete;
		T_Material(T_Material&& other)
			: mat(std::move(other.mat))
			, begin(other.begin)
		{}
	};

	struct Shader
	{
		ShaderProgram program;
        l_bag<T_Material>::perma_ref begin;

		Shader(ShaderProgram program, l_bag<T_Material>::perma_ref begin)
			: program(program), begin(begin) {}

		MEMBER_EQUALITY(ShaderProgram, program)

        Shader& operator=(Shader&&) = default;
		Shader(const Shader&) = delete;
		Shader(Shader&& other)
			: program(std::move(other.program))
			, begin(other.begin)
		{}
	};

	struct InstData
	{
		Matrix4f mat;
        Object obj;
        InstData(const Matrix4f& m, Object o) : mat(m), obj(o) {}
        InstData() : mat(), obj(Object::invalid) {}
		InstData& operator=(const Matrix4f& m) { mat = m; return *this; }
	};

    using InstanceVec = l_bag<InstData, Eigen::aligned_allocator<InstData>>;

	struct Shape
	{
		VAO vao;
        InstanceVec::perma_ref begin;
        std::array<std::unordered_set<ShaderProgram>::iterator, NumPasses> passShader;
        std::array<std::unordered_set<Material>::iterator, NumPasses> passMaterial;

		Shape(const VertexData& vertData, const ShaderProgram& program, InstanceVec::perma_ref begin)
			: vao(program, vertData), begin(begin)
		{}

        Shape& operator=(Shape&&) = default;
		Shape(const Shape&) = delete;
		Shape(Shape&& other) //MSVC sucks and can't default this
            : vao(std::move(other.vao))
            , begin(other.begin)
            , passShader(std::move(other.passShader))
            , passMaterial(std::move(other.passMaterial))
        {}

        MEMBER_EQUALITY(VertexData, vao)
	};
}
