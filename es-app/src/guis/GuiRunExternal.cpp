#include "GuiRunExternal.h"
#include "AudioManager.h"
#include "VolumeControl.h"
#include "components/OptionListComponent.h"
#include "CLog.hpp"
#include "SystemConf.h"
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <unistd.h>
#include "guis/GuiMsgBox.h"

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

// #define BIN_DIR "/userdata/roms/bin"

// Run any file that have x mode in specific folder "/userdata/roms/bin/"
// With defined argument in /userdata/roms/bin/*.arg
GuiRunExternal::GuiRunExternal(Window* window) : GuiSettings(window, _("Run External").c_str())
{
	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->TextSmall.font;
	unsigned int color = theme->Text.color;

	programList = std::make_shared<OptionListComponent<std::string>>(mWindow, _("RUN PROGRAM"), false); // 2nd Argumen must be supplied with string len > 0 !!!
	argumentList = std::make_shared<OptionListComponent<std::string>>(mWindow, _("ARGUMENT"), false); // 2nd Argumen must be supplied with string len > 0 !!!
	argumentList->add(_("none"), _("none"), true);

	DIR *dir;
	char fullpath[2048];
	struct dirent *entry;
    std::string cmd;
	std::string run_external_path = SystemConf::getInstance()->get("run.external.path");
	// std::string run_external_path = BIN_DIR;
	if ( (dir = opendir(run_external_path.c_str())) ) {
		entry = readdir(dir);
		do {
			snprintf(fullpath, 2047, "%s%s", run_external_path.c_str(), entry->d_name);
			if (strstr(entry->d_name, ".arg")) {
				argumentList->add(entry->d_name, fullpath, false);
			}else if(isExecutable(fullpath)){
				programList->add(entry->d_name, fullpath, false);
			}
		} while ((entry = readdir(dir)) != NULL);
		closedir(dir);
        argumentList->selectFirstItem();    // must be selected one!!
        programList->selectFirstItem();     // must be selected one!!
	}
    setSubTitle("Execute any executable from path:\n"+run_external_path);
	addWithLabel(_("RUN PROGRAM"), programList);
	addWithLabel(_("ARGUMENT"), argumentList);
	mMenu.clearButtons();
	mMenu.addButton(_("RUN"), _("Run"), std::bind(&GuiRunExternal::runExternal, this));
    mMenu.addButton(_("LOG"), _("Log"), std::bind(&GuiRunExternal::viewLog, this));
	mMenu.addButton(_("BACK"), _("go back"), [this] { close(); });
}

int GuiRunExternal::runExternal()
{
    int rc = 0;
    std::string cmd, cwd;
    char *args;
    
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
	AudioManager::getInstance()->deinit();
	VolumeControl::getInstance()->deinit();
    bool hideWindow = Settings::getInstance()->getBool("HideWindow");
    mWindow->deinit(hideWindow);
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
    chdir(SystemConf::getInstance()->get("run.external.path").c_str());
    cmd = Utils::String::format("%s %s",programList->getSelected().c_str(),args);
    rc =  system(cmd.c_str());
    free(args);
    chdir(cwd.c_str());
    if (!hideWindow && Settings::getInstance()->getBool("HideWindowFullReinit")){
		ResourceManager::getInstance()->reloadAll();
		mWindow->deinit();
		mWindow->init();
	}else
		mWindow->init(hideWindow);

	VolumeControl::getInstance()->init();
	AudioManager::getInstance()->init();
	mWindow->normalizeNextUpdate();
    return rc;
}

#define MAX_LINE 100
int GuiRunExternal::viewLog(){
    FILE *f;
	char lines[MAX_LINE][2000];
    int lineCount = 0;
    std::string err,log;
    std::string run_external_path = SystemConf::getInstance()->get("run.external.path");

    GuiSettings* msgBox = new GuiSettings(mWindow, "View Log");
    // err = Utils::FileSystem::readAllText(run_external_path + "err.txt");
    // log = Utils::FileSystem::readAllText(run_external_path + "log.txt");
	// msgBox->setSubTitle("\nERROR:\n"+err+"\nLOG:\n"+log);
    // msgBox->setTag("Tag");

    msgBox->addGroup(_("LOG:"));
    f = fopen((run_external_path + "log.txt").c_str(),"r");
	if (!f){
		msgBox->addEntry("Error open file log.txt");
	}else{
        lineCount = 0;
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
			msgBox->addEntry((char *)(lines[j]), false, [this, lines, j] {
				mWindow->pushGui(new GuiMsgBox(mWindow, (char *)(lines[j])));
			});
		}
		fclose(f);
        if (!lineCount){
		    msgBox->addEntry("Nothing.");
	    }
	}

    msgBox->addGroup(_("ERROR:"));
    f = fopen((run_external_path + "err.txt").c_str(),"r");
	if (!f){
		msgBox->addEntry("Error open file err.txt");
	}else{
        lineCount = 0;
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
			msgBox->addEntry((char *)(lines[j]), false, [this, lines, j] {
				mWindow->pushGui(new GuiMsgBox(mWindow, (char *)(lines[j])));
			});
		}
		fclose(f);

        if (!lineCount){
		    msgBox->addEntry("No error.");
	    }
	}
    mWindow->pushGui(msgBox);
    return 0;
}