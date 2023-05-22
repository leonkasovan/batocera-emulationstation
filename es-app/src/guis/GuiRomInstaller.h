#pragma once

#include "GuiSettings.h"
#include "GuiComponent.h"
#include "components/ComponentGrid.h"
#include "components/ComponentList.h"
#include "components/ComponentTab.h"
#include "ContentInstaller.h"
class Window;
class SystemData;

// reference: PacmanPackage (ApiSystem.h)
struct ROMPackage
{
	std::string no;
	std::string name;
	std::string system;
	std::string title;
	std::string status;
	std::string desc;
	std::string url;

	bool isInstalled() { return status == "installed"; }
};

// reference: class GuiBatoceraStoreEntry
class ROMEntry : public ComponentGrid
{
public:
	ROMEntry(Window* window, ROMPackage& entry);

	bool isInstallPending() { return mIsPending; }
	ROMPackage& getEntry() { return mEntry; }
	virtual void setColor(unsigned int color);

private:
	std::shared_ptr<TextComponent>  mImage;
	std::shared_ptr<TextComponent>  mText;
	std::shared_ptr<TextComponent>  mSubstring;

	ROMPackage mEntry;
	bool mIsPending;
};

class GuiRomDownloader : public GuiComponent, IContentInstalledNotify
{
public:
	GuiRomDownloader(Window* window, std::string last_search_name, std::vector<std::string> systems);
	~GuiRomDownloader();

	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;

	virtual std::vector<HelpPrompt> getHelpPrompts() override;
	virtual void onSizeChanged() override;

	void OnContentInstalled(int contentType, std::string contentName, bool success) override;

private:
	static std::vector<ROMPackage> queryPackages();
	void loadPackagesAsync(bool updatePackageList = false, bool refreshOnly = true);
	void loadList(bool updatePackageList, bool restoreIndex = true);
	void processPackage(ROMPackage package);
	void centerWindow();
	void showSearch();
	int find_csv2(const char *fname, const char *in_category, char **lword, unsigned int start_no);

	int				mReloadList;
	std::vector<ROMPackage> mPackages;
	

	NinePatchComponent				mBackground;
	ComponentGrid					mGrid;

	std::shared_ptr<TextComponent>	mTitle;
	std::shared_ptr<TextComponent>	mSubtitle;

	std::shared_ptr<ComponentList>	mList;

	std::shared_ptr<ComponentGrid>	mHeaderGrid;
	std::shared_ptr<ComponentGrid>	mButtonGrid;

	std::shared_ptr<ComponentTab>	mTabs;

	std::string						mTabFilter;
	std::string						mTextFilter;

	std::string						mArchitecture;
	AsyncNotificationComponent* mWndNotification;
};


class GuiRomInstaller : public GuiSettings
{
public:
	GuiRomInstaller(Window* window);
	~GuiRomInstaller();
	bool isSupportedPlatform(std::string system);
	void pressedStart();
	std::shared_ptr<OptionListComponent<std::string>> mSystems;
	std::shared_ptr<ComponentList>	mList;
	std::string mSupportedSystems;
};
