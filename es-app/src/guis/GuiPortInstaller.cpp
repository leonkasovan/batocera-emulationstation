/*
TODO:
1. Auto update dan download portmaster
2. Try mono games
3. Uninstall 
https://github.com/PortsMaster/PortMaster-Releases/releases/latest/download/PortMaster.zip
https://github.com/PortsMaster/PortMaster-Releases/releases/latest/download/mono-6.12.0.122-aarch64.squashfs


*/
#include "GuiPortInstaller.h"
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
#include "ApiSystem.h"
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

#define MAXLENGTH 4000
#define MAXTOKEN 6
#define WINDOW_WIDTH (float)Math::min(Renderer::getScreenHeight() * 1.125f, Renderer::getScreenWidth() * 0.95f)
#define LOCAL_VERSION "/userdata/roms/ports/PortMaster/version"
#define ONLINE_VERSION "https://github.com/PortsMaster/PortMaster-Releases/releases/latest/download/version"

static int parseLine(char *line, char *tokens[MAXTOKEN]){
    char *token = NULL;
    int ntoken = 0;

    token = strtok(line, "|");
    while (token != NULL && ntoken < MAXTOKEN) {
        tokens[ntoken++] = token;
		token = strtok(NULL, "|");	//	Subsequent calls to strtok() use NULL as the input, which tells the function to continue splitting from where it left off
	}
    return ntoken;
}

GuiPortDownloader::GuiPortDownloader(Window* window)
	: GuiComponent(window), mGrid(window, Vector2i(1, 4)), mBackground(window, ":/frame.png")
{
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
	mTitle = std::make_shared<TextComponent>(mWindow, _("Port DOWNLOADER"), theme->Title.font, theme->Title.color, ALIGN_CENTER);
	mSubtitle = std::make_shared<TextComponent>(mWindow, _("Select Port to download"), theme->TextSmall.font, theme->TextSmall.color, ALIGN_CENTER);
	mHeaderGrid->setEntry(mTitle, Vector2i(0, 1), false, true);
	mHeaderGrid->setEntry(mSubtitle, Vector2i(0, 3), false, true);

	mGrid.setEntry(mHeaderGrid, Vector2i(0, 0), false, true);

	// Tabs
	mTabs = std::make_shared<ComponentTab>(mWindow);
	mTabs->addTab("Ready to Run");
	mTabs->addTab("Mono Required");
	mTabs->addTab("Original Game Data Required");

	mTabs->setCursorChangedCallback([&](const CursorState&)
	{
		if (mTabFilter != mTabs->getCursorIndex())
		{
			mTabFilter = mTabs->getCursorIndex();
			mReloadList = 3;
		}
	});

	mGrid.setEntry(mTabs, Vector2i(0, 1), false, true);

	// Entries
	mList = std::make_shared<ComponentList>(mWindow);
	mList->setUpdateType(ComponentListFlags::UpdateType::UPDATE_ALWAYS);

	mGrid.setEntry(mList, Vector2i(0, 2), true, true);

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

	// check PortMaster version
	if (Utils::FileSystem::exists(LOCAL_VERSION)){
		std::string localVersion = Utils::FileSystem::readAllText(LOCAL_VERSION);
		if (ApiSystem::getInstance()->downloadFile(ONLINE_VERSION, _(LOCAL_VERSION)+"-online", "check version", nullptr)){
			std::string onlineVersion = Utils::FileSystem::readAllText(_(LOCAL_VERSION)+"-online");
			if (onlineVersion != localVersion){
				mWindow->displayNotificationMessage(_U("\uF019 ") + Utils::String::format("Start updating PortMaster %s to %s ", localVersion.c_str(), onlineVersion.c_str()));
				ContentInstaller::Enqueue(mWindow, ContentInstaller::CONTENT_PORT_INSTALL, "/userdata/roms/ports/PortMaster.zip|https://github.com/PortsMaster/PortMaster-Releases/releases/latest/download/PortMaster.zip");
			}
		}
	}
}

GuiPortDownloader::~GuiPortDownloader()
{
	ContentInstaller::UnregisterNotify(this);
}

void GuiPortDownloader::OnContentInstalled(int contentType, std::string contentName, bool success)
{
	if (contentType == ContentInstaller::CONTENT_PORT_INSTALL || contentType == ContentInstaller::CONTENT_PORT_UNINSTALL)
		mReloadList = 2;
}

void GuiPortDownloader::update(int deltaTime)
{
	GuiComponent::update(deltaTime);
	loadList();
}

void GuiPortDownloader::onSizeChanged()
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

void GuiPortDownloader::centerWindow()
{
	if (Renderer::isSmallScreen())
		setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
	else
		setSize(WINDOW_WIDTH, Renderer::getScreenHeight() * 0.875f);

	setPosition((Renderer::getScreenWidth() - getSize().x()) / 2, (Renderer::getScreenHeight() - getSize().y()) / 2);
}

static bool sortPackagesByGroup(PortPackage& sys1, PortPackage& sys2)
{
	if (sys1.type == sys2.type)
		return Utils::String::compareIgnoreCase(sys1.title, sys2.title) < 0;		

	return sys1.type.compare(sys2.type) < 0;
}

void GuiPortDownloader::loadList()
{
	int idx = mList->getCursorIndex();
	mList->clear();

	//std::string db_path = SystemConf::getInstance()->get("rom_downloader.db.path");
	std::string db_path = "/userdata/system/portmaster.db";
	size_t n_port = 0;
	PortPackage package;
	FILE *f;
    char line[MAXLENGTH];
	char *tokens[MAXTOKEN];
    int ntoken;

	// mTabFilter: global var for filtering list based on selected tab (index=0[Ready to run] index=1[Need mono] index=2[Need data original game])
	if ((f = fopen(db_path.c_str(), "r")) == NULL) {
		mWindow->pushGui(new GuiMsgBox(mWindow, Utils::String::format("Error opendir for path %s", db_path.c_str())));
	}else{
		while (fgets(line, MAXLENGTH, f)) {
			line[strcspn(line, "\n")] = '\0'; // Remove newline character if present
			if (line[0] != '\0'){
				ntoken = parseLine(line, tokens);
				if (ntoken == MAXTOKEN){
					if (mTabFilter == 0){
						if (strcmp(tokens[0], "rtr")) continue;
					}else if (mTabFilter == 1){
						if (strcmp(tokens[0], "mon")) continue;
					}else if (mTabFilter == 2){
						if (strcmp(tokens[0], "ext")) continue;
					}
					package.no = n_port + 1;
					package.type = tokens[0];
					package.title = tokens[1];
					package.desc = tokens[2];
					package.filename = tokens[3];
					package.size = tokens[4];
					package.genre = tokens[5];
					package.status = "";
					auto grid = std::make_shared<PortEntry>(mWindow, package);
					ComponentListRow row;
					row.addElement(grid, true);
					row.makeAcceptInputHandler([this, package] { processPackage(package); });
					mList->addRow(row, idx == n_port);
					n_port++;
				}
			}
		}	
		fclose(f);
	}

	if (n_port == 0){
		auto theme = ThemeData::getMenuTheme();
		ComponentListRow row;
		row.selectable = false;
		auto text = std::make_shared<TextComponent>(mWindow, _("No items"), theme->TextSmall.font, theme->Text.color, ALIGN_CENTER);
		row.addElement(text, true);
		mList->addRow(row, false, false);
	}else{
		mSubtitle->setText(Utils::String::format("Select %d Port to download",n_port));
	}

	centerWindow();
	mReloadList = 0;
}

std::vector<PortPackage> GuiPortDownloader::queryPackages()
{
	std::vector<PortPackage> copy;
	return copy;
}

void GuiPortDownloader::loadPackagesAsync(bool updatePackageList, bool refreshOnly)
{
}

void GuiPortDownloader::processPackage(PortPackage package)
{	
	if (package.filename.empty())
		return;

	if (!Utils::FileSystem::exists("/userdata/roms/ports/PortMaster/PortMaster.sh")){
		mWindow->displayNotificationMessage(_U("\uF019 ") + Utils::String::format("Start downloading PortMaster"));
		ContentInstaller::Enqueue(mWindow, ContentInstaller::CONTENT_PORT_INSTALL, "/userdata/roms/ports/PortMaster.zip|https://github.com/PortsMaster/PortMaster-Releases/releases/latest/download/PortMaster.zip");
	}

	std::string port_data = "/userdata/roms/ports/" + package.filename + "|" + "https://github.com/PortsMaster/PortMaster-Releases/releases/latest/download/" + package.filename;

	GuiSettings* msgBox = new GuiSettings(mWindow, package.title);
	msgBox->setSubTitle(package.desc);
	msgBox->setTag("popup");
	
	if (package.isInstalled())
	{
		msgBox->addEntry(_U("\uF019 ") + _("UPDATE"), false, [this, msgBox, port_data, package]
		{
			mWindow->displayNotificationMessage(_U("\uF019 ") + Utils::String::format("Downloading %s start", package.filename.c_str()));
			ContentInstaller::Enqueue(mWindow, ContentInstaller::CONTENT_PORT_INSTALL, port_data);
			mReloadList = 2;
			msgBox->close();
		});

		msgBox->addEntry(_U("\uF014 ") + _("REMOVE"), false, [this, msgBox, port_data, package]
		{
			mWindow->displayNotificationMessage(_U("\uF014 ") + _("UNINSTALLATION ADDED TO QUEUE"));
			ContentInstaller::Enqueue(mWindow, ContentInstaller::CONTENT_PORT_UNINSTALL, package.filename);			
			mReloadList = 2;
			msgBox->close();
		});
	}
	else
	{
		msgBox->addEntry(_U("\uF019 ") + _("INSTALL: ") + package.size, false, [this, msgBox, port_data, package]
		{
			mWindow->displayNotificationMessage(_U("\uF019 ") + Utils::String::format("Downloading %s start", package.filename.c_str()));
			ContentInstaller::Enqueue(mWindow, ContentInstaller::CONTENT_PORT_INSTALL, port_data);
			mReloadList = 2;
			msgBox->close();
		});
	}

	mWindow->pushGui(msgBox);
}

bool GuiPortDownloader::input(InputConfig* config, Input input)
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

std::vector<HelpPrompt> GuiPortDownloader::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mList->getHelpPrompts();
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	prompts.push_back(HelpPrompt("y", _("SEARCH")));
	return prompts;
}

//////////////////////////////////////////////////////////////
// PortEntry
//////////////////////////////////////////////////////////////

PortEntry::PortEntry(Window* window, PortPackage& entry) :
	ComponentGrid(window, Vector2i(4, 3))
{
	mEntry = entry;

	auto theme = ThemeData::getMenuTheme();

	bool isInstalled = entry.isInstalled();
	mImage = std::make_shared<TextComponent>(mWindow);
	mImage->setColor(theme->Text.color);
	if (!isInstalled)
		mImage->setOpacity(192);

	mImage->setHorizontalAlignment(Alignment::ALIGN_CENTER);
	mImage->setVerticalAlignment(Alignment::ALIGN_TOP);
	mImage->setFont(theme->TextSmall.font);
	mImage->setText(Utils::String::format("%s %d.",isInstalled ? _U("\uF058") : _U("\uF019"),entry.no));
	mImage->setSize(theme->Text.font->getLetterHeight() * 1.5f, 0);
	std::string label = Utils::String::format("%s (%s / %s)", entry.title.c_str(), entry.size.c_str(), entry.genre.c_str());
	mText = std::make_shared<TextComponent>(mWindow, label, theme->Text.font, theme->Text.color);
	mText->setLineSpacing(1.5);
	mText->setVerticalAlignment(ALIGN_TOP);
	mSubstring = std::make_shared<TextComponent>(mWindow, entry.desc, theme->TextSmall.font, theme->Text.color);
	mSubstring->setSize(Renderer::getScreenWidth() * 0.88f, 0);
	mSubstring->setOpacity(192);

	setEntry(mImage, Vector2i(0, 0), false, true, Vector2i(1, 3));
	setEntry(mText, Vector2i(2, 0), false, true);
	setEntry(mSubstring, Vector2i(2, 1), false, true);

	float h = mText->getSize().y() * 1.1f + mSubstring->getSize().y();
	float sw = WINDOW_WIDTH;

	setColWidthPerc(0, 50.0f / sw, false);
	setColWidthPerc(1, 0.015f, false);
	setColWidthPerc(3, 0.002f, false);

	setRowHeightPerc(0, mText->getSize().y() / h, false);
	setRowHeightPerc(1, mSubstring->getSize().y() / h, false);
	setSize(Vector2f(0, h));
}

void PortEntry::setColor(unsigned int color)
{
	mImage->setColor(color);
	mText->setColor(color);
	mSubstring->setColor(color);
}