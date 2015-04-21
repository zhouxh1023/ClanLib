/*
**  ClanLib SDK
**  Copyright (c) 1997-2015 The ClanLib Team
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
**  Note: Some of the libraries ClanLib may link to may have additional
**  requirements or restrictions.
**
**  File Author(s):
**
**    Mark Page
*/

#include "precomp.h"

#include "options.h"

Options::Options(clan::Canvas &canvas) : clan::TextureView(canvas)
{
	// Note, when changing these, remember to change the popup menu defaults
	compare_function = clan::compare_greater;
	stencil_pass = clan::stencil_keep;
	stencil_fail = clan::stencil_incr;

	num_balls = 9;
	is_moveballs_set = true;
	is_circle_set = true;
	compare_reference = 1;

	int slider_xpos = 400;
	int slider_label_xpos = slider_xpos + 200;
	int slider_ypos = 250;
	int slider_gap = 24;
	slider_numballs = create_slider(slider_xpos, slider_ypos);
	slider_numballs->set_max_position(9);
	slider_numballs->set_position(num_balls);
	slider_numballs->func_value_changed() = bind_member(this, &Options::slider_numballs_changed);
	label_numballs = create_slider_label(slider_label_xpos, slider_ypos);
	slider_ypos += slider_gap;

	slider_compare_reference = create_slider(slider_xpos, slider_ypos);
	slider_compare_reference->set_max_position(16);
	slider_compare_reference->set_position(compare_reference);
	slider_compare_reference->func_value_changed() = bind_member(this, &Options::slider_compare_reference_changed);
	label_compare_reference = create_slider_label(slider_label_xpos, slider_ypos);
	slider_ypos += slider_gap;

	int checkbox_xpos = slider_xpos;
	int checkbox_ypos = slider_ypos;
	int checkbox_gap = 24;
	checkbox_moveballs = create_checkbox(checkbox_xpos, checkbox_ypos, "Moving Balls", is_moveballs_set);
	checkbox_moveballs->func_state_changed() = bind_member(this, &Options::checkbox_moveballs_changed);
	checkbox_ypos += checkbox_gap + 8;
	checkbox_circle = create_checkbox(checkbox_xpos, checkbox_ypos, "Draw Stencil Circle (Without selected stencil options)", is_circle_set);
	checkbox_circle->func_state_changed() = bind_member(this, &Options::checkbox_circle_changed);
	checkbox_ypos += checkbox_gap + 8;

	//make_compare_menu(combo_compare_menu);
	//combo_comparefunc = create_compare_combo_box(600, 40, combo_compare_menu, 3);
	//label_comparefunc = create_combobox_label(combo_comparefunc, "Compare Function");

	//make_passfail_menu(combo_pass_menu);
	//combo_pass = create_passfail_combo_box(600, 80, combo_pass_menu, 0);
	//label_pass = create_combobox_label(combo_pass, "Pass Operation");

	//make_passfail_menu(combo_fail_menu);
	//combo_fail = create_passfail_combo_box(600, 120, combo_fail_menu, 3);
	//label_fail = create_combobox_label(combo_fail, "Fail Operation");

	update_all_slider_text();
}

Options::~Options()
{

}

float Options::get_value(std::shared_ptr<clan::SliderView> slider)
{
	float value = (float)slider->position();
	value /= (float)slider->max_position();
	return value;
}

std::shared_ptr<clan::SliderView> Options::create_slider(int xpos, int ypos)
{
	std::shared_ptr<clan::SliderView> component = Theme::create_slider();
	add_subview(component);

	component->style()->set("position: absolute; left:%1px; top:%2px; width:%3px; height:auto;", xpos, ypos, 192);
	component->set_horizontal();
	component->set_min_position(0);
	component->set_max_position(1000);
	component->set_tick_count(100);
	component->set_page_step(100);
	component->set_lock_to_ticks(false);
	component->set_position(component->max_position());

	return component;
}

std::shared_ptr<clan::CheckBoxView> Options::create_checkbox(int xpos, int ypos, const std::string &name, bool state)
{
	std::shared_ptr<clan::CheckBoxView> checkbox = Theme::create_checkbox();
	add_subview(checkbox);
	checkbox->style()->set("position: absolute; left:%1px; top:%2px", xpos, ypos);
	checkbox->set_check(state);

	auto label = Theme::create_label(true);
	label->set_text(name);
	label->style()->set("position: absolute; left:%1px; top:%2px", xpos + 16, ypos - 3);
	add_subview(label);

	return checkbox;
}

/*
void Options::on_compare_selected(int value, clan::ComboBox *combo)
{
	switch (value)
	{
		case 0:
			compare_function = clan::compare_lequal;
			break;
		case 1:
			compare_function = clan::compare_gequal;
			break;
		case 2:
			compare_function = clan::compare_less;
			break;
		case 3:
			compare_function = clan::compare_greater;
			break;
		case 4:
			compare_function = clan::compare_equal;
			break;
		case 5:
			compare_function = clan::compare_notequal;
			break;
		case 6:
			compare_function = clan::compare_always;
			break;
		case 7:
			compare_function = clan::compare_never;
			break;
	}
}

void Options::on_passfail_selected(int value, clan::ComboBox *combo)
{
	clan::StencilOp selected;
	switch (value)
	{
		case 0:
			selected = clan::stencil_keep;
			break;
		case 1:
			selected = clan::stencil_zero;
			break;
		case 2:
			selected = clan::stencil_replace;
			break;
		case 3:
			selected = clan::stencil_incr;
			break;
		case 4:
			selected = clan::stencil_decr;
			break;
		case 5:
			selected = clan::stencil_invert;
			break;
		case 6:
			selected = clan::stencil_incr_wrap;
			break;
		case 7:
			selected = clan::stencil_decr_wrap;
			break;
		default:
			throw clan::Exception("ERROR");
	}
	if (combo == combo_pass)
	{
		stencil_pass = selected;
	}
	else
	{
		stencil_fail = selected;
	}
}
*/

std::shared_ptr<clan::LabelView> Options::create_slider_label(int xpos, int ypos)
{
	std::shared_ptr<clan::LabelView> component = Theme::create_label(true);
	add_subview(component);
	component->style()->set("position: absolute; left:%1px; top:%2px", xpos, ypos);
	component->set_text("##################");
	return component;
}

void Options::slider_numballs_changed()
{
	num_balls = slider_numballs->position();
	std::string text(clan::string_format("Number of Balls : %1", num_balls));
	label_numballs->set_text(text);
}

void Options::slider_compare_reference_changed()
{
	compare_reference = slider_compare_reference->position();
	std::string text(clan::string_format("Stencil Reference : %1", compare_reference));
	label_compare_reference->set_text(text);
}

void Options::update_all_slider_text()
{
	slider_numballs_changed();
	slider_compare_reference_changed();
}

void Options::checkbox_moveballs_changed()
{
	is_moveballs_set = checkbox_moveballs->checked();
}

void Options::checkbox_circle_changed()
{
	is_circle_set = checkbox_circle->checked();
}

/*

clan::ComboBox *Options::create_compare_combo_box(int xpos, int ypos, clan::PopupMenu &menu, int selected_item)
{
	clan::ComboBox *combo = new clan::ComboBox(this);
	combo->set_geometry(clan::Rect(xpos, ypos, clan::Size(180, 21)));
	combo->set_editable(false);
	combo->set_dropdown_height(128);
	combo->set_dropdown_minimum_width(64);
	combo->set_popup_menu(menu);
	combo->set_selected_item(selected_item);
	combo->func_item_selected() = [=](int value){on_compare_selected(value, combo); };

	return combo;
}

clan::ComboBox *Options::create_passfail_combo_box(int xpos, int ypos, clan::PopupMenu &menu, int selected_item)
{
	clan::ComboBox *combo = new clan::ComboBox(this);
	combo->set_geometry(clan::Rect(xpos, ypos, clan::Size(180, 21)));
	combo->set_editable(false);
	combo->set_dropdown_height(128);
	combo->set_dropdown_minimum_width(64);
	combo->set_popup_menu(menu);
	combo->set_selected_item(selected_item);
	combo->func_item_selected() = [=](int value){on_passfail_selected(value, combo); };

	return combo;
}

void Options::make_compare_menu(clan::PopupMenu &menu)
{
	menu.insert_item("lequal");
	menu.insert_item("gequal");
	menu.insert_item("less");
	menu.insert_item("greater");
	menu.insert_item("equal");
	menu.insert_item("notequal");
	menu.insert_item("always");
	menu.insert_item("never");
}

void Options::make_passfail_menu(clan::PopupMenu &menu)
{
	menu.insert_item("keep");
	menu.insert_item("zero");
	menu.insert_item("replace");
	menu.insert_item("incr");
	menu.insert_item("decr");
	menu.insert_item("invert");
	menu.insert_item("incr_wrap");
	menu.insert_item("decr_wrap");
}

std::shared_ptr<clan::LabelView> Options::create_combobox_label(clan::ComboBox *combo, const char *text)
{
	std::shared_ptr<clan::LabelView> component = new clan::Label(this);
	clan::Rect combo_geometry = combo->get_geometry();
	component->set_geometry(clan::Rect(combo_geometry.left, combo_geometry.top - 20, clan::Size(256, 17)));
	component->set_text(text);
	return component;
}

*/

