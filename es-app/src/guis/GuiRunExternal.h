#pragma once

#include "GuiSettings.h"

class Window;

class GuiRunExternal : public GuiSettings
{
public:
	GuiRunExternal(Window* window);
	int runExternal();

	std::shared_ptr<OptionListComponent<std::string>> programList;
	std::shared_ptr<OptionListComponent<std::string>> argumentList;
};
