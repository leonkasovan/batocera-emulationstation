#include "GuiRunExternal.h"
#include "AudioManager.h"
#include "VolumeControl.h"
#include "components/OptionListComponent.h"
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <unistd.h>

bool isExecutable(const char* filename) {
    struct stat fileStat;

    // Retrieve file information
    if (stat(filename, &fileStat) == -1) {
        return false;
    }

    // Check if the file is executable
    if ((fileStat.st_mode & S_IXUSR) && S_ISREG(fileStat.st_mode)) {
        return true;
    } else {
        return false;
    }
}

#define BIN_DIR "/userdata/roms/bin"

// Run any file that have x mode in specific folder "/userdata/roms/bin"
// With defined argument in /userdata/roms/bin/*.txt
GuiRunExternal::GuiRunExternal(Window* window) : GuiSettings(window, _("Run External").c_str())
{
	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->TextSmall.font;
	unsigned int color = theme->Text.color;

	programList = std::make_shared<OptionListComponent<std::string>>(mWindow, _(""), false);
	argumentList = std::make_shared<OptionListComponent<std::string>>(mWindow, _(""), false);
	argumentList->add(_("none"), _("none"), true);

	DIR *dir;
	char buffer[2048];
	char fullpath[1024];
	struct dirent *entry;
    std::string cmd;
	//std::string run_external_path = SystemConf::getInstance()->get("run.external.path");
	std::string run_external_path = BIN_DIR;
	if ( (dir = opendir(run_external_path.c_str())) ) {
		entry = readdir(dir);
		do {
			sprintf(fullpath, "%s/%s", run_external_path.c_str(), entry->d_name);
			if (strstr(entry->d_name, ".txt")) {
				argumentList->add(entry->d_name, fullpath, false);
			}else if(isExecutable(fullpath)){
				programList->add(entry->d_name, fullpath, true);
			}
		} while ((entry = readdir(dir)) != NULL);
		closedir(dir);
	}

	addWithLabel(_("RUN PROGRAM"), programList);
	addWithLabel(_("ARGUMENT"), argumentList);
	mMenu.clearButtons();
	mMenu.addButton(_("RUN"), _("Run"), std::bind(&GuiRunExternal::runExternal, this, window));
	mMenu.addButton(_("BACK"), _("go back"), [this] { close(); });
}

int GuiRunExternal::runExternal(Window* window)
{
    int rc;
    std::string cmd, cwd;
    char *args;

	AudioManager::getInstance()->deinit();
	VolumeControl::getInstance()->deinit();
    bool hideWindow = Settings::getInstance()->getBool("HideWindow");
	window->deinit(hideWindow);
    if (argumentList->getSelected() == "none"){
        args = strdup("");
    }else{
        std::ifstream inputFile(argumentList->getSelected());
        if (!inputFile) {
            args = strdup("");
        }else{
            std::string line;
            if (std::getline(inputFile, line)) {
                args = strdup(line.c_str());
            } else {
                args = strdup("");
            }
            inputFile.close();
        }
    }
    cwd = Utils::FileSystem::getCWDPath();
    chdir(BIN_DIR);
    cmd = Utils::String::format("%s \"%s\" > /userdata/system/runExternal.log",programList->getSelected().c_str(),args);
    rc =  system(cmd.c_str());
    free(args);
    chdir(cwd.c_str());
    if (!hideWindow && Settings::getInstance()->getBool("HideWindowFullReinit")){
		ResourceManager::getInstance()->reloadAll();
		window->deinit();
		window->init();
	}else
		window->init(hideWindow);

	VolumeControl::getInstance()->init();
	AudioManager::getInstance()->init();
	window->normalizeNextUpdate();
    return rc;
}
