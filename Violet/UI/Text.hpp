#ifndef TEXT_HPP
#define TEXT_HPP

struct StbTexteditRow;

float STB_TEXTEDIT_GETWIDTH(std::string* str, int n, int i);
void STB_TEXTEDIT_LAYOUTROW(StbTexteditRow* r, std::string* str, int n);

namespace UI
{
	using AlignedBox2i = Eigen::AlignedBox2i;

	enum class TextAlign
	{
		Left, Right, Center
	};

	Vector2i TextDim(const std::string& text);
	Vector2i TextDim(std::string::const_iterator begin, std::string::const_iterator end);
	void DrawText(const std::string& text, Vector2i pos);
	void DrawText(const std::string& text, AlignedBox2i container,
		TextAlign align = TextAlign::Center);
    
    //Internal functions
    void DrawAllText();
    void TextScaling(float scaling);
}

#endif