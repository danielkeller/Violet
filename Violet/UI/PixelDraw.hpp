#ifndef BOX_HPP
#define BOX_HPP

class Window;
struct Layout;

void DrawBox(Vector2i corner, Vector2i size);
void DrawBox(const Layout& l);

void PixelInit(Window& w);
void BindPixelUBO();

#endif