#ifndef Vector_HPP
#define Vector_HPP

#include <xmmintrin.h>

#ifdef __APPLE__
//Library bugs!
#undef _mm_shuffle_ps
#define _mm_shuffle_ps(a, b, mask) __extension__ \
    (__m128)__builtin_shufflevector((__m128)a, (__m128)b, \
    (mask) & 0x3, ((mask) & 0xc) >> 2, \
    (((mask) & 0x30) >> 4) + 4, \
    (((mask) & 0xc0) >> 6) + 4);

#endif

#define WRAPPER(name) \
struct name \
{ \
    name() = default; \
    name(float val) : reg(_mm_set1_ps(val)) {} \
    name(float x, float y, float z, float w = 0) : reg(_mm_set_ps(w,z,y,x)) {} \
    name(__m128 val) : reg(val) {} \
    operator __m128() const {return reg;} \
    __m128 reg; \
}

WRAPPER(XmmWrap3);
WRAPPER(XmmWrap4);

union Vector3
{
    __m128 xmm;
    struct {
        float x, y, z;
    };
    float row[3];
    
    Vector3() = default;
    Vector3(float v) : xmm(_mm_set1_ps(v)) {}
    constexpr Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    Vector3(XmmWrap3 reg) : xmm(reg) {} //_mm_empty?
    operator XmmWrap3() const {return xmm;}
    
    float& operator[](int pos) { return row[pos]; }
};

union Vector4
{
    __m128 xmm;
    struct {
        float x, y, z, w;
    };
    float row[4];
    
    Vector4() = default;
    Vector4(float v) : xmm(_mm_set1_ps(v)) {}
    constexpr Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    
    Vector4(XmmWrap4 reg) : xmm(reg) {}
    operator XmmWrap4() const {return xmm;}
    
    float& operator[](int pos) { return row[pos]; }
};

inline XmmWrap3 operator+(XmmWrap3 a, XmmWrap3 b)
{
    return _mm_add_ps(a, b);
}

inline XmmWrap3 operator-(XmmWrap3 a, XmmWrap3 b)
{
    return _mm_sub_ps(a, b);
}

inline XmmWrap3 operator*(XmmWrap3 a, XmmWrap3 b)
{
    return _mm_mul_ps(a, b);
}

inline XmmWrap4 operator*(XmmWrap4 a, XmmWrap4 b)
{
    return _mm_mul_ps(a, b);
}

inline XmmWrap3 operator-(XmmWrap3 a)
{
    return 0 - a;
}

inline XmmWrap3 operator<=(XmmWrap3 a, XmmWrap3 b)
{
    return _mm_cmple_ps(a, b);
}

inline XmmWrap3 operator>=(XmmWrap3 a, XmmWrap3 b)
{
    return _mm_cmpge_ps(a, b);
}

inline XmmWrap3 operator<(XmmWrap3 a, XmmWrap3 b)
{
    return _mm_cmplt_ps(a, b);
}

inline XmmWrap3 operator>(XmmWrap3 a, XmmWrap3 b)
{
    return _mm_cmpgt_ps(a, b);
}

inline XmmWrap3 operator*(XmmWrap3 a, float b)
{
    return _mm_mul_ps(a, _mm_set1_ps(b));
}

inline XmmWrap3 operator*(float a, XmmWrap3 b)
{
    return b * a;
}

inline XmmWrap3 operator/(XmmWrap3 a, float b)
{
    return _mm_div_ps(a, _mm_set1_ps(b));
}

inline XmmWrap3 min(XmmWrap3 a, XmmWrap3 b)
{
    return _mm_min_ps(a, b);
}

inline XmmWrap3 max(XmmWrap3 a, XmmWrap3 b)
{
    return _mm_max_ps(a, b);
}

inline Vector3& operator*=(Vector3& a, float b)
{
    return a = a * b;
}

//Boolean operations
//True is NaN

#define SSE_REDUCE4(var, fn) \
    __m128 REDUCE_b = _mm_movehl_ps(__m128{}, var); \
    __m128 REDUCE_c = fn(var, REDUCE_b); \
    REDUCE_b = _mm_shuffle_ps(REDUCE_c, __m128{}, _MM_SHUFFLE(0, 0, 0, 1)); \
    var = fn(REDUCE_b, REDUCE_c)
/*
#define SSE_REDUCE3(var, fn) \
__m128 REDUCE_b = _mm_movehl_ps(__m128{}, var); \
__m128 REDUCE_c = _mm_shuffle_ps(var, __m128{}, _MM_SHUFFLE(0, 0, 0, 1)); \
var = fn(fn(var, REDUCE_b), REDUCE_c)
*/
#define SSE_REDUCE3(var, fn) \
__m128 REDUCE_b = _mm_movehl_ps(var, var); \
__m128 REDUCE_c = _mm_shuffle_ps(var, var, _MM_SHUFFLE(0, 0, 0, 1)); \
var = fn(fn(var, REDUCE_b), REDUCE_c)
//NaN == 1 and 0 != 1

#define SSE_BOOL(reg) _mm_comieq_ss(reg, _mm_set1_ps(1))

inline bool any(XmmWrap4 a)
{
    SSE_REDUCE4(a, _mm_or_ps);
    return SSE_BOOL(a);
}

inline bool any(XmmWrap3 a)
{
    SSE_REDUCE3(a, _mm_or_ps);
    return SSE_BOOL(a);
}

inline bool all(XmmWrap4 a)
{
    SSE_REDUCE4(a, _mm_and_ps);
    return SSE_BOOL(a);
}

inline bool all(XmmWrap3 a)
{
    SSE_REDUCE3(a, _mm_and_ps);
    return SSE_BOOL(a);
}

inline XmmWrap3 operator&(XmmWrap3 a, XmmWrap3 b)
{
    return _mm_and_ps(a, b);
}

inline XmmWrap3 operator|(XmmWrap3 a, XmmWrap3 b)
{
    return _mm_or_ps(a, b);
}

inline float product(XmmWrap4 a)
{
    SSE_REDUCE4(a, _mm_mul_ps);
    return _mm_cvtss_f32(a);
}

inline float product(XmmWrap3 a)
{
    SSE_REDUCE3(a, _mm_mul_ps);
    return _mm_cvtss_f32(a);
}

inline float sum(XmmWrap4 a)
{
    SSE_REDUCE4(a, _mm_add_ps);
    return _mm_cvtss_f32(a);
}

inline float sum(XmmWrap3 a)
{
    SSE_REDUCE3(a, _mm_add_ps);
    return _mm_cvtss_f32(a);
}

inline float max(Vector3 v, int& i)
{
    if (v.x >= v.y && v.x >= v.z)
        return i = 0, v.x;
    else if (v.y >= v.z)
        return i = 1, v.y;
    else
        return i = 2, v.z;
}

inline float dot(XmmWrap4 a, XmmWrap4 b)
{
    return sum(a * b);
}

inline float dot(XmmWrap3 a, XmmWrap3 b)
{
    return sum(a * b);
}

inline float sqrNorm(XmmWrap4 a)
{
    return dot(a, a);
}

inline float sqrNorm(XmmWrap3 a)
{
    return dot(a, a);
}

inline float norm(XmmWrap4 a)
{
    return std::sqrtf(sqrNorm(a));
}

inline float norm(XmmWrap3 a)
{
    return std::sqrtf(sqrNorm(a));
}

//for Matrix
typedef const Vector4 (&ColsRef4)[4];
typedef const Vector3 (&ColsRef3)[3];

inline XmmWrap3 min(ColsRef3 vs)
{
    return min(min(vs[0], vs[1]), vs[2]);
}

inline XmmWrap3 max(ColsRef3 vs)
{
    return max(max(vs[0], vs[1]), vs[2]);
}

inline XmmWrap4 sqrNorm(ColsRef4 vs)
{
    return{ sqrNorm(vs[0]), sqrNorm(vs[1]), sqrNorm(vs[2]), sqrNorm(vs[3]) };
}

inline XmmWrap3 sqrNorm(ColsRef3 vs)
{
    return{ sqrNorm(vs[0]), sqrNorm(vs[1]), sqrNorm(vs[2]) };
}

inline XmmWrap3 abs(XmmWrap3 a)
{
    XmmWrap3 mask = a >= 0;
    return mask & a | _mm_andnot_ps(mask, -a);
}

struct Matrix3
{
    Vector3 col[3];
    
    Matrix3() = default;
    //explicit Matrix3(float val) : col{val, val, val} {}
    constexpr Matrix3(Vector3 a, Vector3 b, Vector3 c) : col{a, b, c} {}
    
    float& operator()(int r, int c) { return col[c][r]; }
    
    Matrix3 scale(Vector3 v) const { return{ col[0] * v.x, col[1] * v.y, col[2] * v.z }; }
};

inline constexpr Matrix3 Identity()
{
    return{ {1,0,0}, {0,1,0}, {0,0,1}, };
}

inline XmmWrap3 operator*(const Matrix3& m, const Vector3& v)
{
    return m.col[0] * v.x + m.col[1] * v.y + m.col[2] * v.z;
}

inline Matrix3 operator*(const Matrix3& m, float f)
{
    return{ m.col[0] * f, m.col[1] * f, m.col[2] * f };
}

inline Matrix3 operator*(float f, const Matrix3& m)
{
    return m * f;
}

inline Matrix3 operator+(const Matrix3& l, const Matrix3& r)
{
    return{ l.col[0] + r.col[0], l.col[1] + r.col[1], l.col[2] + r.col[2] };
}

inline Matrix3 operator*(const Matrix3& l, const Matrix3& r)
{
    return{ l*r.col[0], l*r.col[1], l*r.col[2] };
}

inline Matrix3 abs(const Matrix3& m)
{
    return{ abs(m.col[0]), abs(m.col[1]), abs(m.col[2]) };
}
/*
inline Matrix affine(Matrix rot, const Vector& pos)
{
    rot.col[3] = pos;
    rot.col[0].w = rot.col[1].w = rot.col[2].w = 0;
    rot.col[3].w = 1;
    return rot;
}*/

struct Transpose3
{
    Matrix3 orig;
    explicit Transpose3(Matrix3 m) : orig(m) {}
};

inline XmmWrap3 operator*(const Transpose3& m, XmmWrap3 v)
{
    auto& row = m.orig.col;
    return { dot(v, row[0]), dot(v, row[1]), dot(v, row[2]) };
}

inline Matrix3 operator*(const Transpose3& l, const Matrix3& r)
{
    return{ l*r.col[0], l*r.col[1], l*r.col[2] };
}
/*
inline Matrix Transpose(Matrix m)
{
    _MM_TRANSPOSE4_PS(m.col[0].xmm, m.col[1].xmm, m.col[2].xmm, m.col[3].xmm);
    return m;
}*/

struct Box3
{
    Vector3 min, max;
    
    Vector3 size() const { return max - min; }
    Vector3 center() const { return (min + max) / 2; }
    
    Box3 merged(Box3 other) const { return{ ::min(min, other.min), ::max(max, other.max) }; }
    bool contains(Box3 other) const { return all(min <= other.min) && all(max >= other.max); }
    bool intersects(Box3 other) const { return all(max >= other.min & other.max >= min); }
};

union Quat
{
    XmmWrap4 xmm;
    struct {
        float w, x, y, z;
    };
    
    constexpr Quat(float w, float x, float y, float z) : w(w), x(x), y(y), z(z) {}
    Quat(__m128 reg) : xmm(reg) {}
    operator __m128() const {return xmm;}
    
    Matrix3 matrix() const;
};

inline Matrix3 Quat::matrix() const
{
    Vector3 col0 = Vector3(1, 0, 0) + 2*Vector3(-y*y - z*z, x*y, x*z) + 2*w*Vector3(0, z, -y);
    Vector3 col1 = Vector3(0, 1, 0) + 2*Vector3(x*y, -x*x - z*z, y*z) + 2*w*Vector3(-z, 0, x);
    Vector3 col2 = Vector3(0, 0, 1) + 2*Vector3(x*z, y*z, -x*x - y*y) + 2*w*Vector3(y, -x, 0);
    
    return{ col0, col1, col2 };
}

//delete me
inline Vector3 ToV(Vector3f v)
{
    return{v.x(), v.y(), v.z()};
}
inline Matrix3 ToM(Matrix3f m)
{
    return{ToV(m.col(0)), ToV(m.col(1)), ToV(m.col(2)) };
}

#endif
