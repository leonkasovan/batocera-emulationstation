#include "GuiRomInstaller.h"
#include "PlatformId.h"
#include "SystemConf.h"
#include "components/OptionListComponent.h"
#include "components/ButtonComponent.h"
#include "components/ComponentTab.h"
#include "ThemeData.h"
#include "guis/GuiMsgBox.h"
#include "SystemData.h"
#include "views/ViewController.h"
#include "HttpReq.h"
#include "components/AsyncNotificationComponent.h"
#include <thread>
#include "Paths.h"
#include "ContentInstaller.h"
#include "utils/FileSystemUtil.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>

using namespace PlatformIds;

void WriteLog(const char *data){
	auto logPath = Paths::getUserEmulationStationPath() + "/my_log.txt";
	FILE *f;

	f = fopen(logPath.c_str(), "a");
	if (!f) return;
	fputs(data, f);
	fputs("\n", f);
	fclose(f);
}

void url_decode(char *url) {
    char *src, *dest;
    src = dest = url;

    while (*src) {
        if (*src == '+') {
            *dest++ = ' ';
        } else if (*src == '%') {
            char code[3] = {0};
            code[0] = *(++src);
            code[1] = *(++src);
            *dest++ = (char) strtol(code, NULL, 16);
        } else {
            *dest++ = *src;
        }
        ++src;
    }
    *dest = '\0';
}

#define MAX_LINE 2048
char *get_system_in_csv_file(const char *path, char *buffer) {
	char line[MAX_LINE];
	char *p, *q;
	FILE *fi;

	fi = fopen(path, "r");
	if (!fi) {
		return NULL;
	}
	fgets(line, MAX_LINE-1, fi);	// Line 1: #category=snes
	p = strchr(line, '=');
	if (!p) {
		return NULL;
	}
	p++;
	q = buffer;
	while ( (*p != '\r') && (*p != '\n') && (*p != '\0')) {
		*q++ = *p++;
	}
	*q = '\0';
	fclose(fi);
	return buffer;
}

std::string getDestinationPath(std::string system, std::string name){
	std::string dest;
	char s_pid[MAX_LINE];
	char *p;
	strcpy(s_pid, name.c_str());
	url_decode(s_pid);
	p = strrchr(s_pid, '/');
	if (p)
		dest = Utils::String::format("/userdata/roms/%s/%s", system.c_str(), p+1);
	else
		dest = Utils::String::format("/userdata/roms/%s/%s", system.c_str(), s_pid);
	
	return dest;
}

bool GuiRomInstaller::isSupportedPlatform(std::string system){
	return (mSupportedSystems.find(","+system+",") != std::string::npos);
}

GuiRomInstaller::GuiRomInstaller(Window* window) : GuiSettings(window, _("ROMS DOWNLOAD").c_str())
{
	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	std::shared_ptr<TextComponent>  text;
	unsigned int color = theme->Text.color;
	
	mSupportedSystems = SystemConf::getInstance()->get("rom_downloader.supported");
	addInputTextRow(_("SEARCH GAME NAME"), "rom_downloader.last_search_name", false);

	mSystems = std::make_shared<OptionListComponent<std::string>>(mWindow, _("SYSTEMS INCLUDED"), true);
	DIR *dir;
	char buffer[2048];
	char fullpath[MAX_LINE];
	struct dirent *entry;
	std::string db_path = SystemConf::getInstance()->get("rom_downloader.db.path");
	if ( (dir = opendir(db_path.c_str())) ) {
		entry = readdir(dir);
		do {
			if (strstr(entry->d_name, ".csv")) {
				sprintf(fullpath, "%s/%s", db_path.c_str(), entry->d_name);
				get_system_in_csv_file(fullpath, buffer);
				mSystems->add(buffer, buffer, true);
			}
		} while ((entry = readdir(dir)) != NULL);
		closedir(dir);
	}
	addWithLabel(_("SYSTEMS INCLUDED"), mSystems);

	mMenu.clearButtons();
	mMenu.addButton(_("SEARCH"), _("START"), std::bind(&GuiRomInstaller::pressedStart, this));
	mMenu.addButton(_("BACK"), _("go back"), [this] { close(); });
	if (Renderer::isSmallScreen())
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);
	else
		mMenu.setPosition((mSize.x() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

GuiRomInstaller::~GuiRomInstaller()
{
	ViewController::reloadAllGames(mWindow, false);
}

void GuiRomInstaller::pressedStart()
{
	std::string last_search_name = SystemConf::getInstance()->get("rom_downloader.last_search_name");
	std::vector<std::string> systems = mSystems->getSelectedObjects();
	mWindow->pushGui(new GuiRomDownloader(mWindow, last_search_name, systems));
}

// Split and allocate memory
// After using the result, free it with 'free_word'
char **split_word(const char *keyword) {
	char *internal_keyword, *word = NULL, *next = NULL, **p, **lword;
	const char *start;
	int nword = 0;

	// Skip space in keyword
	start = keyword;
	while (*start == ' ')
		start++;

	// Count word
	internal_keyword = strdup(start);
	word = strtok(internal_keyword, " ");
	while (word != NULL) {
		nword++;
		word = strtok(NULL, " ");	//	Subsequent calls to strtok() use NULL as the input, which tells the function to continue splitting from where it left off
	}
	free(internal_keyword);

	// Allocate list of word
	internal_keyword = strdup(start);
	next = NULL;
	lword = (char **)malloc(sizeof(char *) * (nword + 1));
	p = lword;
	word = strtok(internal_keyword, " ");
	while (word != NULL) {
		*p = word;
		word = strtok(NULL, " ");	//	Subsequent calls to strtok() use NULL as the input, which tells the function to continue splitting from where it left off
		p++;
	}
	*p = NULL;
	return lword;
}

// Free memory used by internal_keyword and list of word
void free_word(char **lword) {
	free(*lword);	// free internal_keyword (1st entry)
	free(lword);	// free list of word
}

// Find keyword(char **) in string line
int find_keyword2(char *line, char **lword) {
	char **p = lword;
	char *word, *in_line;
	int found = 1;

	in_line = strdup(line);
	SDL_strlwr(in_line);	// make 'input line' lower
	while (*p) {
		word = *p;
		if (*word == '-') {
			word++;
			if (strstr(in_line, word)) {
				found = found & 0;
				break;
			}else{
				found = found & 1;
			}
		}else{
			if (strstr(in_line, word)) {
				found = found & 1;
			}else{
				found = found & 0;
				break;
			}
		}
		p++;
	}
	free(in_line);
	return found;
}

int GuiRomDownloader::find_csv2(const char *fname, const char *in_category, char **lword, unsigned int start_no) {
	FILE *f;
	char line[MAX_LINE];
	char *category = NULL, *url = NULL, *p;
	char *a_name, *a_title, *a_desc, *next;

	f = fopen(fname, "r");
	if (!f) {
		return start_no;
	}
	
	fgets(line, MAX_LINE, f);	// Line 1: #category=snes\r\n
	if ( (p = strchr(line, '=') )) {
		category = strdup(p);
		*category = ',';	// category = ",snes\r\n"
		p = strrchr(category, '\r'); if (p) *p = '\0';	// replace \r to \0
		p = strrchr(category, '\n'); if (p) *p = '\0';	// replace \n to \0
		if (in_category){
			if (!strstr(in_category, category)) {	// skip category if not requested
				fclose(f); free(category);
				return start_no;
			}
		}
	}
	
	fgets(line, MAX_LINE, f);		// Line 2: #url=https://archive.org/download/cylums or #url=https://archive.org/download/cylums_collection.zip/ 
	if ((p = strchr(line, '='))) {
		url = strdup(p + 1);
		p = strrchr(url, '\r'); // windows end of line
		if (p) {
			*p = '\0';	// replace \r to \0
			if (*(p-1) == '/') *(p-1) = '\0';	// remove trailing /
		}else{
			p = strrchr(url, '\n');
			*p = '\0';	// replace \n to \0
			if (*(p-1) == '/') *(p-1) = '\0';	// remove trailing /
		}
	}

	if (!url) {
		fclose(f);
		if (category) free(category);
		return start_no;
	}
	while (fgets(line, MAX_LINE, f)) { // Process next line: the real csv data
		a_name = strtok(line, "|");
		a_title = strtok(NULL, "|");
		a_desc = strtok(NULL, "|");
		if (find_keyword2(a_title, lword)) {
			start_no++;
			ROMPackage package;
			
			// remove trailing end-of-line in a_desc
			p = strrchr(a_desc, '\r'); // windows end of line
			if (p) {
				*p = '\0';	// replace \r to \0
			}else{
				p = strrchr(a_desc, '\n');
				if (p) *p = '\0';	// replace \n to \0
			}
			package.no = std::to_string(start_no);
			package.desc = a_desc;	// 257MB - 2p - (c)1998 Capcom
			package.name = a_name;	//airbustr.zip avsp.zip roms%2Favsp.zip
			package.system = (char *)(category+1);	// gba,snes,mame,megadrive	+1 = skip leading ,
			package.title = a_title;	// "Tetris Attack (US)", "Super Mario Bros (US)"
			package.url = url;
			package.url = package.url+ "/" + package.name;
			package.status = Utils::FileSystem::exists(getDestinationPath(package.system, package.name))?"installed":"";
			auto grid = std::make_shared<ROMEntry>(mWindow, package);
			ComponentListRow row;
			row.addElement(grid, true);
			if (!grid->isInstallPending()){
				row.makeAcceptInputHandler([this, package] { processPackage(package); });
			}
			mList->addRow(row, false, false);
		}
	}
	fclose(f);
	if (category) free(category);
	if (url) free(url);
	return start_no;
}

#define WINDOW_WIDTH (float)Math::min(Renderer::getScreenHeight() * 1.125f, Renderer::getScreenWidth() * 0.95f)
GuiRomDownloader::GuiRomDownloader(Window* window, std::string last_search_name, std::vector<std::string> systems)
	: GuiComponent(window), mGrid(window, Vector2i(1, 4)), mBackground(window, ":/frame.png")
{
	mReloadList = 1;

	addChild(&mBackground);
	addChild(&mGrid);

	// Form background
	auto theme = ThemeData::getMenuTheme();
	mBackground.setImagePath(theme->Background.path);
	mBackground.setEdgeColor(theme->Background.color);
	mBackground.setCenterColor(theme->Background.centerColor);
	mBackground.setCornerSize(theme->Background.cornerSize);

	// Title
	mHeaderGrid = std::make_shared<ComponentGrid>(mWindow, Vector2i(1, 5));
	mTitle = std::make_shared<TextComponent>(mWindow, _("ROM DOWNLOADER"), theme->Title.font, theme->Title.color, ALIGN_CENTER);
	mSubtitle = std::make_shared<TextComponent>(mWindow, _("Select ROM to download"), theme->TextSmall.font, theme->TextSmall.color, ALIGN_CENTER);
	mHeaderGrid->setEntry(mTitle, Vector2i(0, 1), false, true);
	mHeaderGrid->setEntry(mSubtitle, Vector2i(0, 3), false, true);

	mGrid.setEntry(mHeaderGrid, Vector2i(0, 0), false, true);

	// Tabs
	mTabs = std::make_shared<ComponentTab>(mWindow);
	mTabs->addTab("ROMs");
	mGrid.setEntry(mTabs, Vector2i(0, 1), false, true);	

	// Entries
	mList = std::make_shared<ComponentList>(mWindow);
	mGrid.setEntry(mList, Vector2i(0, 2), true, true);

	std::string db_path = SystemConf::getInstance()->get("rom_downloader.db.path");
	unsigned int n_found = 0; // total result found
	struct dirent *entry;
	DIR *dir;
	char *in_keyword = NULL;
	std::string in_systems = ",";
	char **lword;
	char fullpath[MAX_LINE];
	
	in_keyword = strdup(last_search_name.c_str());
	SDL_strlwr(in_keyword);
	for(auto system : systems){
		in_systems += system + ",";
	}

	if ((dir = opendir(db_path.c_str())) == NULL) {
		mWindow->pushGui(new GuiMsgBox(mWindow, Utils::String::format("Error opendir for path %s", db_path.c_str())));
		free(in_keyword);
	}else{
		lword = split_word(in_keyword);
		entry = readdir(dir);
		do {
			if (strstr(entry->d_name, ".csv")) {
				sprintf(fullpath, "%s%s", db_path.c_str(), entry->d_name);
				n_found = find_csv2(fullpath, in_systems.c_str(), lword, n_found);
			}
		} while ((entry = readdir(dir)) != NULL);
		free_word(lword);
		free(in_keyword);
		closedir(dir);
	}

	if (n_found == 0){
		ComponentListRow row;
		row.selectable = false;
		auto text = std::make_shared<TextComponent>(mWindow, _("No items"), theme->TextSmall.font, theme->Text.color, ALIGN_CENTER);
		row.addElement(text, true);
		mList->addRow(row, false, false);
	}else{
		mSubtitle->setText(Utils::String::format("Select %d ROM to download",n_found));
	}

	// Buttons
	std::vector< std::shared_ptr<ButtonComponent> > buttons;
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("BACK"), _("BACK"), [this] { delete this; }));
	mButtonGrid = makeButtonGrid(mWindow, buttons);
	mGrid.setEntry(mButtonGrid, Vector2i(0, 3), true, false);

	mGrid.setUnhandledInputCallback([this](InputConfig* config, Input input) -> bool
	{
		if (config->isMappedLike("down", input)) { mGrid.setCursorTo(mList); mList->setCursorIndex(0); return true; }
		if (config->isMappedLike("up", input)) { mList->setCursorIndex(mList->size() - 1); mGrid.moveCursor(Vector2i(0, 1)); return true; }
		return false;
	});
	centerWindow();
	ContentInstaller::RegisterNotify(this);
}

GuiRomDownloader::~GuiRomDownloader()
{
	ContentInstaller::UnregisterNotify(this);
}

void GuiRomDownloader::OnContentInstalled(int contentType, std::string contentName, bool success)
{
	if (contentType == ContentInstaller::CONTENT_ROM_INSTALL || contentType == ContentInstaller::CONTENT_ROM_UNINSTALL)
		mReloadList = 2;
}

void GuiRomDownloader::update(int deltaTime)
{
	GuiComponent::update(deltaTime);
}

void GuiRomDownloader::onSizeChanged()
{
	GuiComponent::onSizeChanged();
	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));
	mGrid.setSize(mSize);
	const float titleHeight = mTitle->getFont()->getLetterHeight();
	const float subtitleHeight = mSubtitle->getFont()->getLetterHeight();
	const float titleSubtitleSpacing = mSize.y() * 0.03f;
	mGrid.setRowHeightPerc(0, (titleHeight + titleSubtitleSpacing + subtitleHeight + TITLE_VERT_PADDING) / mSize.y());
	if (mTabs->size() == 0)
		mGrid.setRowHeightPerc(1, 0.00001f);
	else 
		mGrid.setRowHeightPerc(1, (titleHeight + titleSubtitleSpacing) / mSize.y());

	mGrid.setRowHeightPerc(3, mButtonGrid->getSize().y() / mSize.y());
	mHeaderGrid->setRowHeightPerc(1, titleHeight / mHeaderGrid->getSize().y());
	mHeaderGrid->setRowHeightPerc(2, titleSubtitleSpacing / mHeaderGrid->getSize().y());
	mHeaderGrid->setRowHeightPerc(3, subtitleHeight / mHeaderGrid->getSize().y());
}

void GuiRomDownloader::centerWindow()
{
	if (Renderer::isSmallScreen())
		setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
	else
		setSize(WINDOW_WIDTH, Renderer::getScreenHeight() * 0.875f);

	setPosition((Renderer::getScreenWidth() - getSize().x()) / 2, (Renderer::getScreenHeight() - getSize().y()) / 2);
}

static bool sortPackagesByGroup(ROMPackage& sys1, ROMPackage& sys2)
{
	if (sys1.system == sys2.system)
		return Utils::String::compareIgnoreCase(sys1.title, sys2.title) < 0;		

	return sys1.system.compare(sys2.system) < 0;
}

void GuiRomDownloader::loadList(bool updatePackageList, bool restoreIndex)
{
	mList->clear();
		std::vector<std::string> gps;
		std::sort(gps.begin(), gps.end());
		for (auto group : gps)
			mTabs->addTab(group);

		mTabFilter = gps[0];

		mTabs->setCursorChangedCallback([&](const CursorState& /*state*/){
			if (mTabFilter != mTabs->getSelected()){
				mTabFilter = mTabs->getSelected();
				mWindow->postToUiThread([this]() 
				{ 
					mReloadList = 3;					
				});
			}
		});
	int i = 0;
	centerWindow();
	mReloadList = 0;
}

std::vector<ROMPackage> GuiRomDownloader::queryPackages()
{
	std::vector<ROMPackage> copy;
	return copy;
}

void GuiRomDownloader::loadPackagesAsync(bool updatePackageList, bool refreshOnly)
{
}

void GuiRomDownloader::processPackage(ROMPackage package)
{	
	if (package.name.empty())
		return;

	std::string url, filename, rom_data;
	url = package.url;
	filename = getDestinationPath(package.system, package.name);
	rom_data = filename;
	rom_data += "|";
	rom_data += url;

	GuiSettings* msgBox = new GuiSettings(mWindow, package.title);
	msgBox->setSubTitle("Destination:\n" + filename);
	msgBox->setTag("popup");
	
	if (package.isInstalled())
	{
		msgBox->addEntry(_U("\uF019 ") + _("UPDATE"), false, [this, msgBox, rom_data, filename]
		{
			mWindow->displayNotificationMessage(_U("\uF019 ") + Utils::String::format("Downloading %s start", filename.c_str()));
			ContentInstaller::Enqueue(mWindow, ContentInstaller::CONTENT_ROM_INSTALL, rom_data);
			mReloadList = 2;
			msgBox->close();
		});

		msgBox->addEntry(_U("\uF014 ") + _("REMOVE"), false, [this, msgBox, rom_data, filename]
		{
			mWindow->displayNotificationMessage(_U("\uF014 ") + _("UNINSTALLATION ADDED TO QUEUE"));
			ContentInstaller::Enqueue(mWindow, ContentInstaller::CONTENT_ROM_UNINSTALL, filename);			
			mReloadList = 2;
			msgBox->close();
		});
	}
	else
	{
		msgBox->addEntry(_U("\uF019 ") + _("INSTALL"), false, [this, msgBox, rom_data, filename]
		{
			mWindow->displayNotificationMessage(_U("\uF019 ") + Utils::String::format("Downloading %s start", filename.c_str()));
			ContentInstaller::Enqueue(mWindow, ContentInstaller::CONTENT_ROM_INSTALL, rom_data);
			mReloadList = 2;
			msgBox->close();
		});
	}

	mWindow->pushGui(msgBox);
}

bool GuiRomDownloader::input(InputConfig* config, Input input)
{
	if(GuiComponent::input(config, input))
		return true;
	
	if(input.value != 0 && config->isMappedTo(BUTTON_BACK, input))
	{
		delete this;
		return true;
	}

	if(config->isMappedTo("start", input) && input.value != 0)
	{
		// close everything
		Window* window = mWindow;
		while(window->peekGui() && window->peekGui() != ViewController::get())
			delete window->peekGui();

		return true;
	}

	if (mTabs->input(config, input))
		return true;

	return false;
}

std::vector<HelpPrompt> GuiRomDownloader::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mList->getHelpPrompts();
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	prompts.push_back(HelpPrompt("y", _("SEARCH")));
	return prompts;
}

//////////////////////////////////////////////////////////////
// ROMEntry
//////////////////////////////////////////////////////////////

ROMEntry::ROMEntry(Window* window, ROMPackage& entry) :
	ComponentGrid(window, Vector2i(4, 3))
{
	mEntry = entry;

	auto theme = ThemeData::getMenuTheme();

	bool isInstalled = entry.isInstalled();
	mIsPending = false;
	mImage = std::make_shared<TextComponent>(mWindow);
	mImage->setColor(theme->Text.color);
	if (!isInstalled)
		mImage->setOpacity(192);

	mImage->setHorizontalAlignment(Alignment::ALIGN_CENTER);
	mImage->setVerticalAlignment(Alignment::ALIGN_TOP);
	mImage->setFont(theme->TextSmall.font);
	//mImage->setText(isInstalled ? _U("\uF058") : _U("\uF019"));
	mImage->setText(Utils::String::format("%s %s.",isInstalled ? _U("\uF058") : _U("\uF019"),entry.no.c_str()));
	mImage->setSize(theme->Text.font->getLetterHeight() * 1.5f, 0);
	std::string label = entry.title;
	mText = std::make_shared<TextComponent>(mWindow, label, theme->Text.font, theme->Text.color);
	mText->setLineSpacing(1.5);
	mText->setVerticalAlignment(ALIGN_TOP);

	std::string details = _U("\uf0A0 ") + entry.desc;
	details = details + _U("   \uf114 ") + entry.system;

	if (mIsPending)
	{
		char trstring[MAX_LINE+1];
		snprintf(trstring, MAX_LINE, _("%s CURRENTLY IN DOWNLOAD QUEUE").c_str(), entry.name.c_str());
		details = trstring;
	}

	mSubstring = std::make_shared<TextComponent>(mWindow, details, theme->TextSmall.font, theme->Text.color);
	mSubstring->setOpacity(192);

	setEntry(mImage, Vector2i(0, 0), false, true, Vector2i(1, 3));
	setEntry(mText, Vector2i(2, 0), false, true);
	setEntry(mSubstring, Vector2i(2, 1), false, true);

	float h = mText->getSize().y() * 1.1f + mSubstring->getSize().y()/* + mDetails->getSize().y()*/;
	float sw = WINDOW_WIDTH;

	setColWidthPerc(0, 50.0f / sw, false);
	setColWidthPerc(1, 0.015f, false);
	setColWidthPerc(3, 0.002f, false);

	setRowHeightPerc(0, mText->getSize().y() / h, false);
	setRowHeightPerc(1, mSubstring->getSize().y() / h, false);

	if (mIsPending)
	{		
		mImage->setText(_U("\uF04B"));
		mImage->setOpacity(120);
		mText->setOpacity(150);
		mSubstring->setOpacity(120);		
	}
	setSize(Vector2f(0, h));
}

void ROMEntry::setColor(unsigned int color)
{
	mImage->setColor(color);
	mText->setColor(color);
	mSubstring->setColor(color);
}