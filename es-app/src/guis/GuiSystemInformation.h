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

class GuiPathsInfo : public GuiSettings
{
public:
	GuiPathsInfo(Window* window);
};
