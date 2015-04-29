#ifndef ELEMENTS_HPP
#define ELEMENTS_HPP

#include "Containers/l_map.hpp"

#include "PixelDraw.hpp"
#include "Layout.hpp"

#define STB_TEXTEDIT_CHARTYPE unsigned int
#include "stb/stb_textedit.h"

std::string to_upper(std::string str);

namespace UI
{
	using AlignedBox2i = Eigen::AlignedBox2i;

	struct Button
	{
		Button();
		bool Draw(AlignedBox2i box);
		bool Draw(AlignedBox2i box, const std::string& text);

		bool hovered, active;
	};

	struct TextButton
	{
		TextButton(std::string text);
		std::string text;
		Button button;
		bool Draw();
	};

	struct LineEdit
	{
		LineEdit();
		int width;
		STB_TexteditState state;
		bool focused;
		std::string lastText;

		//returns true if the textedit just lost focus
		bool Draw(std::string& text);
	};

	template<class Key>
	struct SelectList
	{
		//in
		l_map<Key, std::string> items;
		int width;
		//internal
		std::vector<Button> buttons;

		void Draw(Key& selected)
		{
			buttons.resize(items.size());
			auto& l = CurLayout();

			l.PushNext(Layout::Dir::Down);
			l.EnsureWidth(width);

			size_t idx = 0;
			for (auto it = items.begin(); it < items.end(); ++it, ++idx)
			{
				auto iteml = l.PutSpace(UI::LINEH);

				if (buttons[idx].Draw(iteml,
					selected == it->first ? to_upper(it->second) : it->second))
					selected = it->first;
			}
			l.Pop();
		}
	};
}

#endif