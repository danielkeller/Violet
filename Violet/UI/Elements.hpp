#ifndef ELEMENTS_HPP
#define ELEMENTS_HPP

#include "Containers/l_map.hpp"

#include "PixelDraw.hpp"
#include "Layout.hpp"
#include "Core/Time.hpp"

#define STB_TEXTEDIT_CHARTYPE unsigned int
#include "stb/stb_textedit.h"

std::string to_upper(std::string str);

namespace UI
{
	using AlignedBox2i = Eigen::AlignedBox2i;

	static const int DEFAULT_WIDTH = 80;

	struct Focusable
	{
		Focusable();
		bool focused, tabbedIn;
		static bool anyFocused;
		//returns true if focus was just lost
		bool Draw(AlignedBox2i box);
		void Unfocus();
	};

	struct Button
	{
		Button();
		bool Behavior(AlignedBox2i box);
		bool Draw(AlignedBox2i box);
		bool Draw(AlignedBox2i box, const std::string& text);
		bool Draw(AlignedBox2i box, const std::string& text, Color color);

		Color GetColor() const;

		bool hovered, active;
	};

	struct TextButton
	{
		TextButton(std::string text, int width = 80);
		int width;
		std::string text;
		Button button;
		bool Draw();
	};

	struct LineEdit
	{
		LineEdit(int width = DEFAULT_WIDTH);
		int width;
		Focusable focus;
		STB_TexteditState state;
		std::string lastText;

		//returns true if the textedit just lost focus or enter is pressed
		bool Draw(std::string& text);
	};

	struct FloatEdit
	{
		FloatEdit(int width = DEFAULT_WIDTH);
		LineEdit edit;
		int prec;
		bool editing;
		std::string editStr;
		//returns true if the textedit just lost focus or enter is pressed
		//and the number is changed
		bool Draw(float& val);
	private:
		void SetDispVal(float val);
	};

	template<class Key>
	struct SelectList
	{
		SelectList(int width = DEFAULT_WIDTH) : width(width) {}
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

				if (selected == it->first)
					buttons[idx].Draw(iteml, it->second, Colors::secondary);
				else if (buttons[idx].Draw(iteml, it->second))
					selected = it->first;
			}
			l.Pop();
		}
	};

	struct ModalBoxRAII
	{
		ModalBoxRAII(UI::Layout::Dir dir);
		~ModalBoxRAII();
		//true if the box was closed
		bool closed;
	};

	enum class Ease
	{
		In, Out, InOut
	};

	class Animation
	{
	public:
		Animation();
		void Start();
		int Continue(int initial, int final, Ease ease, Time::clock::duration time = 100ms);
		int Run(int initial, int final, Ease ease, Time::clock::duration time = 100ms);
		bool Running() const;

	private:
		Time::clock::duration start, last;
	};

	class SlideInOut
	{
	public:
		//return true on close. This backs up the layout and so should be
		//called before drawing anything
		bool Draw(int size, Time::clock::duration time = 100ms);
		void Close();

	private:
		Animation open, close;
	};
}

#endif