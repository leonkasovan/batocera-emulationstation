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
struct PortPackage
{
	size_t no;
	std::string type;
	std::string title;
	std::string desc;
	std::string filename;
	std::string size;
	std::string genre;
	std::string status;

	bool isInstalled() { return status == "installed"; }
};

// reference: class GuiBatoceraStoreEntry
class PortEntry : public ComponentGrid
{
public:
	PortEntry(Window* window, PortPackage& entry);

	bool isInstallPending() { return mIsPending; }
	PortPackage& getEntry() { return mEntry; }
	virtual void setColor(unsigned int color);

private:
	std::shared_ptr<TextComponent>  mImage;
	std::shared_ptr<TextComponent>  mText;
	std::shared_ptr<TextComponent>  mSubstring;

	PortPackage mEntry;
	bool mIsPending;
};

class GuiPortDownloader : public GuiComponent, IContentInstalledNotify
{
public:
	GuiPortDownloader(Window* window);
	~GuiPortDownloader();

	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;

	virtual std::vector<HelpPrompt> getHelpPrompts() override;
	virtual void onSizeChanged() override;

	void OnContentInstalled(int contentType, std::string contentName, bool success) override;

private:
	static std::vector<PortPackage> queryPackages();
	void loadPackagesAsync(bool updatePackageList = false, bool refreshOnly = true);
	void loadList();
	void processPackage(PortPackage package);
	void centerWindow();
	void showSearch();

	int				mReloadList;
	int				mTabFilter;
	std::vector<PortPackage> mPackages;
	

	NinePatchComponent				mBackground;
	ComponentGrid					mGrid;

	std::shared_ptr<TextComponent>	mTitle;
	std::shared_ptr<TextComponent>	mSubtitle;

	std::shared_ptr<ComponentList>	mList;

	std::shared_ptr<ComponentGrid>	mHeaderGrid;
	std::shared_ptr<ComponentGrid>	mButtonGrid;

	std::shared_ptr<ComponentTab>	mTabs;

	//std::string						mTabFilter;
	std::string						mTextFilter;

	std::string						mArchitecture;
	AsyncNotificationComponent* mWndNotification;
};
