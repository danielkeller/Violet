#ifndef ELEMENTS_HPP
#define ELEMENTS_HPP

#include "Containers/l_map.hpp"

#include "PixelDraw.hpp"
#include "Layout.hpp"
#include "Core/Time.hpp"

#define STB_TEXTEDIT_CHARTYPE char32_t
#include "stb/stb_textedit.h"

std::string to_upper(std::string str);

namespace UI
{
	using AlignedBox2i = Eigen::AlignedBox2i;

	static const int DEFAULT_WIDTH = 80;

	struct Focusable
	{
		Focusable();
		bool focused, stealing, tabbedIn;
		static bool anyFocused;
		//returns true if focus was just lost
		bool Draw(AlignedBox2i box);
		void Unfocus();
        void StealFocus();
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
    
    struct IconButton
    {
        IconButton(Tex tex, int width = LINEH);
        int width;
        Tex tex;
        Button button;
        bool Draw();
    };
    
    struct CheckBox
    {
        CheckBox(std::string text, int width = 80);
        int width;
        std::string text;
        Button button;
        void Draw(bool& checked);
    };

	struct LineEdit
	{
		LineEdit(int width = DEFAULT_WIDTH);
		int width;
		Focusable focus;
		STB_TexteditState state;
		std::u32string lastText;

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

	enum class Ease
	{
		In, Out, InOut
	};

	class Animation
	{
	public:
		Animation(Ease ease, Time::clock::duration time = 100ms);
		void Start();
		float Continue() const;
		float Run();
		bool Started() const;

	private:
		Ease ease;
		Time::clock::duration time;
		Time::clock::duration start;
		mutable Time::clock::duration last;
	};

	class SlideInOut
	{
	public:
		SlideInOut(Time::clock::duration time = 100ms);
		//return true on close. This backs up the layout and so should be
		//called before drawing anything
		bool Draw(int size);
		void Close();

	private:
		Animation open, close;
	};

	class ModalBox
	{
		struct RAII;
	public:
		ModalBox();
		RAII Draw(Layout::Dir dir, AlignedBox2i initBox);
		//true if the box is ready to contain UI
		bool Ready() const;
		bool Closed() const;

	private:
		Animation open, close;

		struct RAII
		{
			~RAII();
			ModalBox& box;
		};
	};
}

#endif