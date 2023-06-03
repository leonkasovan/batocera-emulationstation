#include "GuiSystemInformation.h"
#include "SystemConf.h"
#include "components/SwitchComponent.h"
#include "ThemeData.h"
#include "ApiSystem.h"
#include "views/UIModeController.h"

GuiSystemInformation::GuiSystemInformation(Window* window) : GuiSettings(window, _("INFORMATION").c_str())
{
	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	bool warning = ApiSystem::getInstance()->isFreeSpaceLimit();

	addGroup(_("INFORMATION"));

	addWithLabel(_("VERSION"), std::make_shared<TextComponent>(window, ApiSystem::getInstance()->getVersion(), font, color));
	addWithLabel(_("USER DISK USAGE"), std::make_shared<TextComponent>(window, ApiSystem::getInstance()->getFreeSpaceUserInfo(), font, warning ? 0xFF0000FF : color));
	addWithLabel(_("SYSTEM DISK USAGE"), std::make_shared<TextComponent>(window, ApiSystem::getInstance()->getFreeSpaceSystemInfo(), font, color));
	
	std::vector<std::string> infos = ApiSystem::getInstance()->getSystemInformations();
	if (infos.size() > 0)
	{
		addGroup(_("SYSTEM"));

		for (auto info : infos)
		{
			std::vector<std::string> tokens = Utils::String::split(info, ':');
			if (tokens.size() >= 2)
			{
				// concatenat the ending words
				std::string vname;
				for (unsigned int i = 1; i < tokens.size(); i++)
				{
					if (i > 1) vname += " ";
					vname += tokens.at(i);
				}

				addWithLabel(_(tokens.at(0).c_str()), std::make_shared<TextComponent>(window, vname, font, color));
			}
		}
	}

	addGroup(_("VIDEO DRIVER"));
	for (auto info : Renderer::getDriverInformation())
		addWithLabel(_(info.first.c_str()), std::make_shared<TextComponent>(window, info.second, font, color));
}

#define MAX_LINE 100

GuiLogViewer::GuiLogViewer(Window* window) : GuiSettings(window, _("Log ES").c_str())
{
	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->TextSmall.font;
	unsigned int color = theme->Text.color;
	FILE *f;
	char lines[MAX_LINE][2000];
    int lineCount = 0;

	f = fopen("/userdata/system/configs/emulationstation/es_log.txt","r");
	if (!f){
		addWithLabel("Error open file /userdata/system/configs/emulationstation/es_log.txt", std::make_shared<TextComponent>(window, "", font, color));
	}else{
		while (fgets(lines[lineCount % MAX_LINE], 1999, f)){
			lineCount++;
		}

		// Calculate the starting index in the circular buffer
		int start = lineCount > MAX_LINE ? lineCount % MAX_LINE : 0;
		int i, j;
		// Print the lines starting from the calculated index
		for (i = start; i < lineCount; i++) {
			j = i % MAX_LINE;
			strtok(lines[j], "\n"); //strip end-of-line
			addWithLabel((char *)(lines[j]+20), std::make_shared<TextComponent>(window, "", font, color));
		}
		fclose(f);
	}
}
