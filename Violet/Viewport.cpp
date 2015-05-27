#include "stdafx.h"
#include "Viewport.hpp"

//Both of these also convert from our z-up world to GL's y-up world

inline Matrix4f perspective(float fovx, float aspect, float zNear, float zFar)
{
	assert(aspect > 0);
	assert(zFar > zNear);

	float tanHalfFovx = std::tan(fovx / 2.f);
	Matrix4f res = Matrix4f::Zero();
	res(0, 0) = 1.f / (tanHalfFovx);
	res(1, 2) = aspect / tanHalfFovx;
	res(2, 1) = -(zFar + zNear) / (zFar - zNear);
	res(3, 1) = -1.f;
	res(2, 3) = -(2.f * zFar * zNear) / (zFar - zNear);
	return res;
}

inline Matrix4f ortho(float width, float aspect, float zNear, float zFar)
{
	assert(aspect > 0);
	assert(zFar > zNear);

	Matrix4f res = Matrix4f::Zero();
	res(0, 0) = 2.f / width;
	res(1, 2) = 2.f * aspect / width;
	res(2, 1) = -2.f / (zFar - zNear);
	res(2, 3) = (zFar + zNear) / (zFar - zNear);
	res(3, 3) = 1.f;
	return res;
}

Viewport::Viewport(Eigen::AlignedBox2i sb)
	: screenBox(sb) {}

//FIXME?
Vector3f Viewport::ApparentPos(Vector2f posPixel, const Matrix4f& modelview) const
{
	Matrix4f screenAxes = PerspMat() * modelview;
	Vector4f screenVec;
	screenVec << Pixel2Scr(posPixel)*screenAxes(3, 3), screenAxes(2, 3), screenAxes(3, 3);
	Vector4f worldVec(screenAxes.householderQr().solve(screenVec));
	return worldVec.block<3, 1>(0, 0) / worldVec[3];
}

Matrix4f Viewport::PerspMat() const
{
	Vector2f dim = screenBox.sizes().cast<float>();
	return perspective((float)M_PI / 2.f, dim.x() / dim.y(), .1f, 100.f);
}

Matrix4f Viewport::OrthoMat() const
{
	Vector2f dim = screenBox.sizes().cast<float>();
	return ortho(2.f, dim.x() / dim.y(), .1f, 100.f);
}

Vector2f Viewport::Pixel2View(Vector2f posPixel) const
{
	Eigen::AlignedBox2f vp = screenBox.cast<float>();
	Eigen::Array2f posView = (posPixel - vp.min()).cwiseQuotient(vp.sizes());

	return posView;
}

Vector2f Viewport::Pixel2Scr(Vector2f posPixel) const
{
	return Pixel2View(posPixel).array() * 2.f - 1.f;
}

void Viewport::GlViewport() const
{
	glViewport(screenBox.min().x(), screenBox.min().y(), screenBox.sizes().x(), screenBox.sizes().y());
}