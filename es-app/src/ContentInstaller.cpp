#include "ContentInstaller.h"
#include "Window.h"
#include "components/AsyncNotificationComponent.h"
#include "utils/StringUtil.h"
#include "ApiSystem.h"
#include "LocaleES.h"
#include <utility>
#include "HttpReq.h"
#include "Paths.h"

#define ICONINDEX _U("\uF019 ")

ContentInstaller*						ContentInstaller::mInstance = nullptr;
std::mutex								ContentInstaller::mLock;
std::list<std::pair<int, std::string>>	ContentInstaller::mQueue;
std::list<std::pair<int, std::string>>	ContentInstaller::mProcessingQueue;
std::list<IContentInstalledNotify*>		ContentInstaller::mNotification;

void ContentInstaller::Enqueue(Window* window, ContentType type, const std::string contentName)
{
	std::unique_lock<std::mutex> lock(mLock);

	for (auto item : mProcessingQueue)
		if (item.first == type && item.second == contentName)
			return;

	for (auto item : mQueue)
		if (item.first == type && item.second == contentName)
			return;

	mQueue.push_back(std::pair<int, std::string>(type, contentName));

	if (mInstance == nullptr)
		mInstance = new ContentInstaller(window);

	mInstance->updateNotificationComponentTitle(true);
}

bool ContentInstaller::IsInQueue(ContentType type, const std::string contentName)
{
	std::unique_lock<std::mutex> lock(mLock);

	for (auto item : mProcessingQueue)
		if (item.first == type && item.second == contentName)
			return true;

	for (auto item : mQueue)
		if (item.first == type && item.second == contentName)
			return true;

	return false;
}

ContentInstaller::ContentInstaller(Window* window)
{
	mInstance = this;

	mCurrent = 0;
	mQueueSize = 0;

	mWindow = window;

	mWndNotification = mWindow->createAsyncNotificationComponent();
	mHandle = new std::thread(&ContentInstaller::threadUpdate, this);
}

ContentInstaller::~ContentInstaller()
{
	mHandle = nullptr;

	mWndNotification->close();
	mWndNotification = nullptr;
}

void ContentInstaller::updateNotificationComponentTitle(bool incQueueSize)
{
	if (incQueueSize)
		mQueueSize++;

	std::string cnt = " " + std::to_string(mCurrent) + "/" + std::to_string(mQueueSize);
	mWndNotification->updateTitle(ICONINDEX + _("DOWNLOADING")+ cnt);
}

void ContentInstaller::updateNotificationComponentContent(const std::string info)
{
	auto pos = info.find(">>>");
	if (pos != std::string::npos)
	{
		std::string percent(info.substr(pos));
		percent = Utils::String::replace(percent, ">", "");
		percent = Utils::String::replace(percent, "%", "");
		percent = Utils::String::replace(percent, " ", "");

		int value = atoi(percent.c_str());

		std::string text(info.substr(0, pos));
		text = Utils::String::trim(text);

		mWndNotification->updatePercent(value);
		mWndNotification->updateText(text);
	}
	else
	{
		mWndNotification->updatePercent(-1);
		mWndNotification->updateText(info);
	}
}

// name: "/userdata/roms/snes/tetris.zip|http://www.archieve.com/download/tetris.zip"
std::pair<std::string, int> ContentInstaller::installBatoceraRomPackage(std::string name){
	char filename[1024];
	char url[1024];
	const char *src;
	char *dst;

	src = name.c_str();
	dst = filename;
	while(*src && *src != '|'){
		*dst++ = *src++;
	}
	*dst = 0;

	if (*src != '|'){
		return std::pair<std::string, int>("INVALID FORMAT INPUT. MISSING SEPARATOR", -1);
	}

	src++;
	dst = url;
	while(*src){
		*dst++ = *src++;
	}
	*dst = 0;

	HttpReq httpreq(url, filename);
	int curPos = -1;
	while (httpreq.status() == HttpReq::REQ_IN_PROGRESS)
	{
		double pos = httpreq.getPosition();
		if (pos > 0 && curPos != pos)
		{
			mWndNotification->updateText(Utils::String::format("%d%%", httpreq.getPercent()));
			mWndNotification->updatePercent(httpreq.getPercent());
			curPos = pos;
		}
		// std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		// std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	if (httpreq.status() == HttpReq::REQ_SUCCESS)
		return std::pair<std::string, int>(_("DOWNLOAD ROM SUCCESS"),0);
	else
		return std::pair<std::string, int>(_("DOWNLOAD ROM FAILED"),-1);
}

// name: "/userdata/roms/snes/tetris.zip"
std::pair<std::string, int> ContentInstaller::uninstallBatoceraRomPackage(std::string name){
	if (Utils::FileSystem::removeFile(name)){
		return std::pair<std::string, int>("Removing ROM success", 0);
	}else{
		return std::pair<std::string, int>("Removing ROM failed", -1);
	}
}

void ContentInstaller::threadUpdate()
{
	mCurrent = 0;

	// Wait for an event to say there is something in the queue
	std::unique_lock<std::mutex> lock(mLock);

	while (true)
	{
		if (mQueue.empty())
			break;

		mCurrent++;
		updateNotificationComponentTitle(false);

		auto data = mQueue.front();
		mQueue.pop_front();
		mProcessingQueue.push_back(data);

		lock.unlock();

		std::pair<std::string, int> updateStatus;
		bool success = false;

		if (data.first == ContentType::CONTENT_THEME_INSTALL)
		{
			updateStatus = ApiSystem::getInstance()->installBatoceraTheme(data.second, [this](const std::string info)
			{
				updateNotificationComponentContent(info);
			});

			if (updateStatus.second == 0)
			{
				success = true;
				mWindow->displayNotificationMessage(ICONINDEX + data.second + " : " + _("THEME INSTALLED SUCCESSFULLY"));
			}
			else
			{
				std::string error = _("AN ERROR OCCURRED") + std::string(": ") + updateStatus.first;
				mWindow->displayNotificationMessage(ICONINDEX + error);
			}

		}
		else if (data.first == ContentType::CONTENT_THEME_UNINSTALL)
		{
			updateStatus = ApiSystem::getInstance()->uninstallBatoceraTheme(data.second, [this](const std::string info)
			{
				updateNotificationComponentContent(info);
			});

			if (updateStatus.second == 0)
			{
				success = true;
				mWindow->displayNotificationMessage(ICONINDEX + data.second + " : " + _("THEME UNINSTALLED SUCCESSFULLY"));
			}
			else
			{
				std::string error = _("AN ERROR OCCURRED") + std::string(": ") + updateStatus.first;
				mWindow->displayNotificationMessage(ICONINDEX + error);
			}

		}
		else if (data.first == ContentType::CONTENT_BEZEL_INSTALL)
		{
			updateStatus = ApiSystem::getInstance()->installBatoceraBezel(data.second, [this](const std::string info)
			{
				updateNotificationComponentContent(info);
			});

			if (updateStatus.second == 0)
			{
				success = true;
				mWindow->displayNotificationMessage(ICONINDEX + data.second + " : " + _("BEZELS INSTALLED SUCCESSFULLY"));
			}
			else
			{
				std::string error = _("AN ERROR OCCURRED") + std::string(": ") + updateStatus.first;
				mWindow->displayNotificationMessage(ICONINDEX + error);
			}
		}
		else if (data.first == ContentType::CONTENT_BEZEL_UNINSTALL)
		{
			updateStatus = ApiSystem::getInstance()->uninstallBatoceraBezel(data.second, [this](const std::string info)
			{
				updateNotificationComponentContent(info);
			});

			if (updateStatus.second == 0)
			{
				success = true;
				mWindow->displayNotificationMessage(ICONINDEX + data.second + " : " + _("BEZELS UNINSTALLED SUCCESSFULLY"));
			}
			else
			{
				std::string error = _("AN ERROR OCCURRED") + std::string(": ") + updateStatus.first;
				mWindow->displayNotificationMessage(ICONINDEX + error);
			}
		}
		else if (data.first == ContentType::CONTENT_STORE_INSTALL)
		{
			updateStatus = ApiSystem::getInstance()->installBatoceraStorePackage(data.second, [this](const std::string info)
			{
				updateNotificationComponentContent(info);
			});

			if (updateStatus.second == 0)
			{
				success = true;
				mWindow->displayNotificationMessage(ICONINDEX + data.second + " : " + _("PACKAGE INSTALLED SUCCESSFULLY"));
			}
			else
			{
				std::string error = _("AN ERROR OCCURRED") + std::string(": ") + updateStatus.first;
				mWindow->displayNotificationMessage(ICONINDEX + error);
			}
		}
		else if (data.first == ContentType::CONTENT_STORE_UNINSTALL)
		{
			updateStatus = ApiSystem::getInstance()->uninstallBatoceraStorePackage(data.second, [this](const std::string info)
			{
				updateNotificationComponentContent(info);
			});

			if (updateStatus.second == 0)
			{
				success = true;
				mWindow->displayNotificationMessage(ICONINDEX + data.second + " : " + _("PACKAGE REMOVED SUCCESSFULLY"));
			}
			else
			{
				std::string error = _("AN ERROR OCCURED") + std::string(": ") + updateStatus.first;
				mWindow->displayNotificationMessage(ICONINDEX + error);
			}
		}else if (data.first == ContentType::CONTENT_ROM_INSTALL)
		{
			updateStatus = installBatoceraRomPackage(data.second);

			if (updateStatus.second == 0)
			{
				success = true;
				mWindow->displayNotificationMessage(ICONINDEX + _(" : ROM INSTALLED SUCCESSFULLY"));
			}
			else
			{
				std::string error = _("AN ERROR OCCURED") + std::string(": ") + updateStatus.first;
				mWindow->displayNotificationMessage(ICONINDEX + error);
			}
		}
		else if (data.first == ContentType::CONTENT_ROM_UNINSTALL)
		{
			updateStatus = uninstallBatoceraRomPackage(data.second);

			if (updateStatus.second == 0)
			{
				success = true;
				mWindow->displayNotificationMessage(ICONINDEX + data.second + " : " + _("ROM REMOVED SUCCESSFULLY"));
			}
			else
			{
				std::string error = _("AN ERROR OCCURED") + std::string(": ") + updateStatus.first;
				mWindow->displayNotificationMessage(ICONINDEX + error);
			}
		}

		OnContentInstalled(data.first, data.second, success);

		lock.lock();
		mProcessingQueue.remove(data);
	}

	delete this;
	mInstance = nullptr;
}

void ContentInstaller::OnContentInstalled(int contentType, std::string contentName, bool success)
{
	std::unique_lock<std::mutex> lock(mLock);

	for (IContentInstalledNotify* n : mNotification)
		n->OnContentInstalled(contentType, contentName, success);
}

void ContentInstaller::RegisterNotify(IContentInstalledNotify* instance)
{
	std::unique_lock<std::mutex> lock(mLock);
	mNotification.push_back(instance);
}

void ContentInstaller::UnregisterNotify(IContentInstalledNotify* instance)
{
	std::unique_lock<std::mutex> lock(mLock);

	for (auto it = mNotification.cbegin(); it != mNotification.cend(); it++)
	{
		if ((*it) == instance)
		{
			mNotification.erase(it);
			return;
		}
	}
}
