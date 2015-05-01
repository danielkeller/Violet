#include "stdafx.h"
#include "Elements.hpp"
#include "Window.hpp"
#include "Layout.hpp"
#include "PixelDraw.hpp"
#include "Text.hpp"

#include <iostream>
#include <iomanip>
#include <cstring>

using namespace UI;

std::string to_upper(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);
	return str;
}

Button::Button()
	: hovered(false), active(false)
{}

bool Button::Draw(AlignedBox2i box)
{
	auto mouse = FrameEvents().MousePosPxl().cast<int>();

	//the button is clicked if it was active last frame and the mouse was released
	if (active && FrameEvents().MouseRelease(GLFW_MOUSE_BUTTON_LEFT))
	{
		//we handled the mouse up event
		FrameEvents().PopMouse();
		return true;
	}

	//hovered state changes only when the mouse is up
	if (!FrameEvents().MouseButton(GLFW_MOUSE_BUTTON_LEFT))
		hovered = box.contains(mouse);

	//the button is active if it's clicked and hovered
	active = hovered && box.contains(mouse) && FrameEvents().MouseButton(GLFW_MOUSE_BUTTON_LEFT);

	//while active we handle mouse events
	//if (active)
	//	FrameEvents().PopMouse();

	return false;
}

bool Button::Draw(AlignedBox2i box, const std::string& text)
{
	auto mouse = FrameEvents().MousePosPxl().cast<int>();

	DrawBox(box);
	if (active)
		;
	else if (hovered)
	{
		DrawText(to_upper(text), box);
	}
	else
		DrawText(text, box);

	return Draw(box);
}

TextButton::TextButton(std::string text, int width)
	: width(width), text(text)
{}

bool TextButton::Draw()
{
	Layout l = CurLayout().PutSpace({ width, LINEH });
	return button.Draw(l, text);
}

RadioGroup::RadioGroup(std::vector<TextButton> buttons)
	: buttons(buttons)
{}

bool RadioGroup::Draw(int& selected)
{
	bool ret = false;
	int curButton = 0;
	for (auto& b : buttons)
	{
		if (b.button.Draw(CurLayout().PutSpace({ b.width, LINEH }),
			selected == curButton ? to_upper(b.text) : b.text))
		{
			if (selected != curButton)
			{
				selected = curButton;
				ret = true;
			}
		}
		++curButton;
	}
	return ret;
}

#define STB_TEXTEDIT_STRING std::string
#define STB_TEXTEDIT_STRINGLEN(str) (static_cast<int>(str->size()))
#define STB_TEXTEDIT_GETCHAR(str, i) ((*str)[i])
#define STB_TEXTEDIT_NEWLINE '\n'
#define STB_TEXTEDIT_DELETECHARS(str, i, n) (str->erase(i, n))
#define STB_TEXTEDIT_INSERTCHARS(str, i, cs, n) (str->insert(str->begin() + i, cs, cs+n), true)
#define STB_TEXTEDIT_K_SHIFT (1<<31)
#define STB_TEXTEDIT_K_CTRL (1<<30)
#define STB_TEXTEDIT_K_LEFT GLFW_KEY_LEFT
#define STB_TEXTEDIT_K_RIGHT GLFW_KEY_RIGHT
#define STB_TEXTEDIT_K_UP GLFW_KEY_UP
#define STB_TEXTEDIT_K_DOWN GLFW_KEY_DOWN
#define STB_TEXTEDIT_K_LINESTART GLFW_KEY_HOME
#define STB_TEXTEDIT_K_LINEEND GLFW_KEY_END
#define STB_TEXTEDIT_K_TEXTSTART (GLFW_KEY_HOME | STB_TEXTEDIT_K_CTRL)
#define STB_TEXTEDIT_K_TEXTEND (GLFW_KEY_END | STB_TEXTEDIT_K_CTRL)
#define STB_TEXTEDIT_K_DELETE GLFW_KEY_DELETE
#define STB_TEXTEDIT_K_BACKSPACE GLFW_KEY_BACKSPACE
#define STB_TEXTEDIT_K_UNDO (GLFW_KEY_Z | STB_TEXTEDIT_K_CTRL)
#define STB_TEXTEDIT_K_REDO (GLFW_KEY_Y | STB_TEXTEDIT_K_CTRL)
#define STB_TEXTEDIT_K_INSERT GLFW_KEY_INSERT
#define STB_TEXTEDIT_IS_SPACE isspace
#define STB_TEXTEDIT_K_WORDLEFT (GLFW_KEY_LEFT | STB_TEXTEDIT_K_CTRL)
#define STB_TEXTEDIT_K_WORDRIGHT (GLFW_KEY_RIGHT | STB_TEXTEDIT_K_CTRL)

#define STB_TEXTEDIT_IMPLEMENTATION
#include "stb/stb_textedit.h"

LineEdit::LineEdit(int width)
	: width(width), focused(false)
{
	stb_textedit_initialize_state(&state, true);
}

bool LineEdit::Draw(std::string& text)
{
	static const int LEFT_PAD = 4;

	auto l = CurLayout().PutSpace({ width, LINEH });
	auto box = l.Box();
	//as far as stb_textedit is concerned
	Vector2i origin = box.min() + Vector2i{ LEFT_PAD, BASELINE_DEPTH };

	auto mouse = FrameEvents().MousePosPxl().cast<int>();
	Vector2f mouseOffs = (mouse - origin).cast<float>();

	if (text != lastText)
		stb_textedit_clear_state(&state, true);

	bool ret = false;

	if (FrameEvents().MouseClick(GLFW_MOUSE_BUTTON_LEFT))
	{
		if (box.contains(mouse))
		{
			focused = true;
			stb_textedit_click(&text, &state, mouseOffs.x(), mouseOffs.y());
		}
		else
		{
			ret = focused;
			focused = false;
		}
	}
	else if (focused && FrameEvents().MouseButton(GLFW_MOUSE_BUTTON_LEFT))
		stb_textedit_drag(&text, &state, mouseOffs.x(), mouseOffs.y());

	if (focused)
	{
		//loses focus on enter key
		if (FrameEvents().PopKeyEvent({ { GLFW_KEY_ENTER, 0 }, GLFW_RELEASE }))
		{
			ret = focused;
			focused = false;
		}

		for (auto key : FrameEvents().keyEvents)
		{
			if (key.action == GLFW_PRESS)
				continue; //wait for release or repeat

			//translate from glfw to stb
			int stb_key = key.key.key; //lol
			if (key.key.mods & GLFW_MOD_CONTROL)
				stb_key |= STB_TEXTEDIT_K_CTRL;
			if (key.key.mods & GLFW_MOD_SHIFT)
				stb_key |= STB_TEXTEDIT_K_SHIFT;

			stb_textedit_key(&text, &state, stb_key);
		}
		FrameEvents().keyEvents.clear();

		for (auto ch : FrameEvents().charEvents)
			stb_textedit_char(&text, &state, ch);
		FrameEvents().charEvents.clear();
	}

	lastText = text;

	DrawText(text, origin);

	StbFindState find;

	long timeHalfSec = std::chrono::duration_cast<
		std::chrono::duration<long, std::ratio<1,2>>>(FrameEvents().simTime).count();

	if (focused && timeHalfSec & 1) //blink
	{
		stb_textedit_find_charpos(&find, &text, state.cursor, true);
		DrawText("|", origin + Vector2i{ find.x - 3, find.y });
	}

	if (state.select_start != state.select_end)
	{
		int start = std::min(state.select_start, state.select_end);
		stb_textedit_find_charpos(&find, &text, start, true);
		std::string uline(std::abs(state.select_end - state.select_start), '_');
		DrawText(uline, origin + Vector2i{ find.x, find.y });
	}

	return ret;
}

FloatEdit::FloatEdit(int width)
	: edit(width), prec(3), editing(false)
{}

bool FloatEdit::Draw(float& val)
{
	if (!editing)
	{
		std::ostringstream ss;
		ss << std::fixed << std::setprecision(prec) << val;
		editStr = ss.str();
	}

	std::string dispStr = editStr;

	bool ret;
	if ((ret = edit.Draw(dispStr)))
		editing = false;

	if (dispStr != editStr) //user changed it
	{
		editing = true;
		editStr = dispStr;

		std::istringstream ss(editStr);
		float typedVal;
		ss >> std::noskipws >> typedVal;
		if (!ss.fail())
			val = typedVal;
	}

	return ret;
}