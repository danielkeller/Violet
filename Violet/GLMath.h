#include "stdafx.h"

Matrix4f perspective(float fovy, float aspect, float zNear, float zFar)
{
    assert(aspect > 0);
    assert(zFar > zNear);

    float tanHalfFovy = std::tan(fovy / 2.f);
    Matrix4f res = Matrix4f::Zero();
    res(0,0) = 1.f / (aspect * tanHalfFovy);
    res(1,1) = 1.f / (tanHalfFovy);
    res(2,2) = - (zFar + zNear) / (zFar - zNear);
    res(3,2) = - 1.f;
    res(2,3) = - (2.f * zFar * zNear) / (zFar - zNear);
    return res;
}

Matrix4f lookAt(Vector3f eye, Vector3f center, Vector3f up)
{
    Vector3f f = (center - eye).normalized();
    Vector3f u = up.normalized();
    Vector3f s = f.cross(u).normalized();
    u = s.cross(f);

    Matrix4f res;
    res <<  s.x(), s.y(), s.z(), -s.dot(eye),
            u.x(), u.y(), u.z(), -u.dot(eye),
            -f.x(), -f.y(), -f.z(), f.dot(eye),
            0,0,0,1;

    return res;
}