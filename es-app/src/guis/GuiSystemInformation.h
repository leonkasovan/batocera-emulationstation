#pragma once

#include "GuiSettings.h"

class Window;

class GuiSystemInformation : public GuiSettings
{
public:
	GuiSystemInformation(Window* window);
};

class GuiLogViewer : public GuiSettings
{
public:
	GuiLogViewer(Window* window);
};
