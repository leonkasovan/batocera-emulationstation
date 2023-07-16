#include "GuiSystemInformation.h"
#include "SystemConf.h"
#include "components/SwitchComponent.h"
#include "ThemeData.h"
#include "ApiSystem.h"
#include "Paths.h"
#include "views/UIModeController.h"
#include "guis/GuiMsgBox.h"

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
			addEntry((char *)(lines[j]+20), false, [this, lines, j] {
				mWindow->pushGui(new GuiMsgBox(mWindow, (char *)(lines[j]+20)));
			});
		}
		fclose(f);
	}
}

GuiPathsInfo::GuiPathsInfo(Window* window) : GuiSettings(window, _("Paths Info").c_str())
{
	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	addGroup(_("SYSTEM PATH:"));
	addWithDescription("Root Path", Paths::getRootPath(),std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("System Conf File Path", Paths::getSystemConfFilePath(),std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("Log Path", Paths::getLogPath(),std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("ScreenShot Path", Paths::getScreenShotPath(),std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("Saves Path", Paths::getSavesPath(),std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("Music Path", Paths::getMusicPath(),std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("Themes Path", Paths::getThemesPath(),std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("Keyboard Mappings Path", Paths::getKeyboardMappingsPath(),std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("Decorations Path", Paths::getDecorationsPath(),std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("Shaders Path", Paths::getShadersPath(),std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("Retroachivement Sounds", Paths::getRetroachivementSounds(),std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("EmulationStation Path", Paths::getEmulationStationPath(),std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("TimeZones Path", Paths::getTimeZonesPath(),std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("Version Info Path", Paths::getVersionInfoPath(),std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("User Manual Path", Paths::getUserManualPath(),std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("Home Path", Paths::getHomePath(),std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("Exe Path", Paths::getExePath(),std::make_shared<TextComponent>(window, "", font, color));

	addGroup(_("USER PATH:"));
	addWithDescription("User Music Path", Paths::getUserMusicPath(), std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("User Themes Path", Paths::getUserThemesPath(), std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("User Keyboard Mappings Path", Paths::getUserKeyboardMappingsPath(), std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("User Decorations Path", Paths::getUserDecorationsPath(), std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("User Shaders Path", Paths::getUserShadersPath(), std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("User Retroachivement Sounds", Paths::getUserRetroachivementSounds(), std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("User EmulationStation Path", Paths::getUserEmulationStationPath(), std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("User EmulationStation Setting Path", Paths::getUserEmulationStationPath() + "/es_settings.cfg", std::make_shared<TextComponent>(window, "", font, color));
	addWithDescription("User EmulationStation Script Path", Paths::getUserEmulationStationPath() + "/script", std::make_shared<TextComponent>(window, "", font, color));
}