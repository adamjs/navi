/*
	This file is part of Navi, a library that allows developers to embed movable 
	'Navis' (Dynamic, HTML/JS/CSS-Driven GUI Overlays) within an Ogre3D application.

	Copyright (C) 2007 Adam J. Simmons
	http://www.agelessanime.com/Navi/

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "NaviManager.h"
#include "Navi.h"
#include <llmozlib.h>
#include <algorithm>
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <direct.h>
#include <stdlib.h>
#endif

using namespace NaviLibrary;

struct NaviCompare {
  bool operator() (Navi* a, Navi* b) { return (a->overlay->getZOrder() > b->overlay->getZOrder()); }
} cmpZOrder;

NaviPosition::NaviPosition()
{
	usingRelative = false;
	data.abs.left = 0;
	data.abs.top = 0;
}

NaviPosition::NaviPosition(const RelativePosition &relPosition, short offsetLeft, short offsetTop)
{
	usingRelative = true;
	data.rel.position = relPosition;
	data.rel.x = offsetLeft;
	data.rel.y = offsetTop;
}

NaviPosition::NaviPosition(short absoluteLeft, short absoluteTop)
{
	usingRelative = false;
	data.abs.left = absoluteLeft;
	data.abs.top = absoluteTop;
}

NaviManager::NaviManager()
{
	startedUp = false;
	localNaviDirectory = "NaviLocal";
	focusedNavi = 0;
	hiddenWindowID = 0;
	renderWindow = 0;
	mouseXPos = mouseYPos = 0;
	mouseButtonRDown = false;
	zOrderCounter = 5;
	mouse = 0;
}

NaviManager::~NaviManager()
{
	if(startedUp) Shutdown();
}

NaviManager& NaviManager::Get()
{
	static NaviManager instance;

	return instance;
}

void NaviManager::Startup(Ogre::RenderWindow* _renderWindow, const std::string &_localNaviDirectory)
{
	renderWindow = _renderWindow;
	localNaviDirectory = _localNaviDirectory;

	if(!LLMozLib::getInstance()->init(getCurrentWorkingDirectory(), "NaviProfile"))
		OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, 
			"LLMozLib failed initialization, could not Startup NaviManager.", 
			"NaviManager::Startup");

	size_t tempAttr = 0;
	HWND windowHnd;
	renderWindow->getCustomAttribute( "WINDOW", &tempAttr );
	windowHnd = (HWND)tempAttr;

	hiddenWindowID = LLMozLib::getInstance()->createBrowserWindow(windowHnd, 128, 128);
	LLMozLib::getInstance()->setEnabled(hiddenWindowID, true);
	LLMozLib::getInstance()->focusBrowser(hiddenWindowID, false);
	LLMozLib::getInstance()->navigateTo(hiddenWindowID, "about:blank");

	startedUp = true;
}

NaviMouse* NaviManager::StartupMouse(bool visible)
{
	if(mouse) return mouse;

	mouse = new NaviMouse(visible);
	return mouse;
}

NaviMouse* NaviManager::getMouse()
{
	if(mouse)
		return mouse;
	else
		OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED, 
			"An attempt was made to get the mouse before the mouse had been started up!", 
			"NaviManager:getMouse");

	return 0;
}

void NaviManager::Update()
{
	static std::map<std::string,Navi*>::iterator end;
	end = activeNavis.end();
	iter = activeNavis.begin();

	while(iter != end)
	{
		if(iter->second->okayToDelete)
		{
			Navi* naviToDelete = iter->second;
			iter = activeNavis.erase(iter);
			if(focusedNavi == naviToDelete) focusedNavi = 0;
			delete naviToDelete;
		}
		else
		{
			iter->second->update();
			iter++;
		}
	}

	if(mouse) mouse->update();
}

void NaviManager::Shutdown()
{
	if(startedUp)
	{
		iter = activeNavis.begin();
		while(iter != activeNavis.end())
		{
			Navi* toDelete = iter->second;
			iter = activeNavis.erase(iter);
			delete toDelete;
		}

		if(hiddenWindowID)
			LLMozLib::getInstance()->destroyBrowserWindow(hiddenWindowID);

		LLMozLib::getInstance()->clearCache();
		LLMozLib::getInstance()->reset();
	}

	if(mouse) delete mouse;

	startedUp = false;
}

void NaviManager::createNavi(const std::string &naviName, const std::string &homepage,  const NaviPosition &naviPosition, unsigned short width, unsigned short height,
	bool isMovable, bool isVisible, unsigned int maxUpdatesPerSec, bool forceMaxUpdate, unsigned short zOrder, float opacity)
{
	if(!startedUp) 
		OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED, 
		"A Navi was attempted to be created without first calling Startup!", "NaviManager::createNavi");
			
	if(!zOrder) zOrder = zOrderCounter++;
	if(activeNavis.find(naviName) == activeNavis.end())
		activeNavis[naviName] = new Navi(renderWindow, naviName, homepage, naviPosition, width, height, isMovable, isVisible, maxUpdatesPerSec, forceMaxUpdate, zOrder, opacity);
}

std::string NaviManager::createNaviMaterial(const std::string &naviName, const std::string &homepage, unsigned short width, unsigned short height, 
	bool isVisible, unsigned int maxUpdatesPerSec, bool forceMaxUpdate, float opacity, Ogre::FilterOptions texFiltering)
{
	if(!startedUp) 
		OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED, 
		"A Navi was attempted to be created without first calling Startup!", "NaviManager::createNaviMaterial");

	if(activeNavis.find(naviName) == activeNavis.end())
		activeNavis[naviName] = new Navi(renderWindow, naviName, homepage, width, height, isVisible, maxUpdatesPerSec, forceMaxUpdate, opacity, texFiltering);

	if(!Ogre::MaterialManager::getSingleton().getByName(naviName + "Material").isNull())
		return naviName + "Material";

	return "";
}

void NaviManager::navigateNaviTo(const std::string &naviName, const std::string &url)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		iter->second->navigateTo(url);
}

void NaviManager::navigateNaviTo(const std::string &naviName, const std::string &url, const NaviData &naviData)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		iter->second->navigateTo(url, naviData);
}

bool NaviManager::canNavigateBack(const std::string &naviName)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		return LLMozLib::getInstance()->canNavigateBack(iter->second->windowID);

	return false;
}

void NaviManager::navigateNaviBack(const std::string &naviName)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		LLMozLib::getInstance()->navigateBack(iter->second->windowID);
}

bool NaviManager::canNavigateForward(const std::string &naviName)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		return LLMozLib::getInstance()->canNavigateForward(iter->second->windowID);

	return false;
}

void NaviManager::navigateNaviForward(const std::string &naviName)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		LLMozLib::getInstance()->navigateForward(iter->second->windowID);
}

void NaviManager::navigateNaviStop(const std::string &naviName)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		LLMozLib::getInstance()->navigateStop(iter->second->windowID);
}

std::string NaviManager::naviEvaluateJS(const std::string &naviName, const std::string &script)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		return iter->second->evaluateJS(script);

	return "";
}

void NaviManager::destroyNavi(const std::string &naviName)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		iter->second->okayToDelete = true;
}

void NaviManager::setNaviBackgroundColor(const std::string &naviName, float red, float green, float blue)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		iter->second->setBackgroundColor(red, green, blue);
}

void NaviManager::setNaviOpacity(const std::string &naviName, float opacity)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		iter->second->setOpacity(opacity);
}

void NaviManager::setNaviMask(const std::string &naviName, const std::string &maskFileName, const std::string &groupName)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		iter->second->setMask(maskFileName, groupName);
}

void NaviManager::setNaviIgnoreTransparent(const std::string &naviName, bool ignoreTrans, float defineThreshold)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		iter->second->setIgnoreTransparentAreas(ignoreTrans, defineThreshold);
}

void NaviManager::setNaviColorKey(const std::string &naviName, const std::string &keyColor, float keyFillOpacity, const std::string &keyFillColor, float keyFuzziness)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		iter->second->setColorKey(keyColor, keyFillOpacity, keyFillColor, keyFuzziness);
}

void NaviManager::setMaxUpdatesPerSec(const std::string &naviName, unsigned int maxUPS)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		iter->second->maxUpdatePS = maxUPS;
}

void NaviManager::setForceMaxUpdate(const std::string &naviName, bool forceMaxUpdate)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		iter->second->forceMax = forceMaxUpdate;
}

void NaviManager::moveNavi(const std::string &naviName, int deltaX, int deltaY)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		iter->second->moveNavi(deltaX, deltaY);
}

void NaviManager::setNaviPosition(const std::string &naviName, const NaviPosition &naviPosition)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
	{
		if(!iter->second->isMaterialOnly)
		{
			iter->second->position = naviPosition;
			iter->second->setDefaultPosition();
		}
	}
}

void NaviManager::resetNaviPosition(const std::string &naviName)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		if(!iter->second->isMaterialOnly)
			iter->second->setDefaultPosition();
}

void NaviManager::resetAllPositions()
{
	for(iter = activeNavis.begin(); iter != activeNavis.end(); iter++)
	{
		if(!iter->second->isMaterialOnly)
			iter->second->setDefaultPosition();
	}
}

void NaviManager::hideNavi(const std::string &naviName, bool fade, unsigned short fadeDurationMS)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		iter->second->hide(fade, fadeDurationMS);
}

void NaviManager::showNavi(const std::string &naviName, bool fade, unsigned short fadeDurationMS)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		iter->second->show(fade, fadeDurationMS);
}

bool NaviManager::isAnyNaviFocused()
{
	if(focusedNavi)
		return true;

	return false;
}

const std::string & NaviManager::getFocusedNaviName()
{
	if(focusedNavi)
		return focusedNavi->naviName;

	static std::string empty("");

	return empty;
}

std::string NaviManager::getNaviMaterialName(const std::string &naviName)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		return naviName + "Material";

	return "";
}

Ogre::OverlayContainer* NaviManager::getNaviInternalPanel(const std::string &naviName)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
	{
		if(iter->second->isMaterialOnly)
			return 0;
		
		return iter->second->panel;
	}

	return 0;
}

bool NaviManager::getNaviVisibility(const std::string &naviName)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		return iter->second->isVisible;

	return false;
}

bool NaviManager::injectMouseMove(int xPos, int yPos)
{
	bool eventHandled = false;

	if(mouseButtonRDown && focusedNavi)
	{
		focusedNavi->moveNavi(xPos-mouseXPos, yPos-mouseYPos);
		eventHandled = true;
	}
	else
	{
		Navi* tempNavi;
		std::vector<Navi*> possibleNavis = getNavisAtPoint(xPos, yPos);

		if(possibleNavis.size())
		{
			try {
				tempNavi = getNavisAtPoint(xPos, yPos).at(0);
			} catch(...) {
				tempNavi = 0;
			}
		} else tempNavi = 0;

		if(tempNavi)
		{
			int relX = tempNavi->getRelativeX(xPos);
			int relY = tempNavi->getRelativeY(yPos);

			LLMozLib::getInstance()->mouseMove(tempNavi->windowID, relX, relY);
			eventHandled = true;
		}
	}

	mouseXPos = xPos;
	mouseYPos = yPos;

	if(mouse) mouse->move(xPos, yPos);

	return eventHandled;
}

void NaviManager::injectNaviMaterialMouseMove(const std::string &naviName, int xPos, int yPos)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		if(iter->second->isMaterialOnly)
			LLMozLib::getInstance()->mouseMove(iter->second->windowID, xPos, yPos);
}

bool NaviManager::injectMouseWheel(int relScroll)
{
	if(focusedNavi)
	{
		LLMozLib::getInstance()->scrollByLines(focusedNavi->windowID, -(relScroll/30));
		return true;
	}

	return false;
}

void NaviManager::injectNaviMaterialMouseWheel(const std::string &naviName, int relScroll)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		if(iter->second->isMaterialOnly)
			LLMozLib::getInstance()->scrollByLines(iter->second->windowID, -(relScroll/30));
}

bool NaviManager::injectMouseDown(int buttonID)
{
	if(buttonID == LeftMouseButton)
	{
		focusNavi(mouseXPos, mouseYPos);
		if(focusedNavi)
		{
			int relX = focusedNavi->getRelativeX(mouseXPos);
			int relY = focusedNavi->getRelativeY(mouseYPos);
			
			LLMozLib::getInstance()->mouseDown(focusedNavi->windowID, relX, relY);
			return true;
		}
	}
	else if(buttonID == RightMouseButton)
	{
		focusNavi(mouseXPos, mouseYPos);
		mouseButtonRDown = true;
		if(focusedNavi && mouse) if(focusedNavi->movable) mouse->activateCursor("move");
		if(focusedNavi) return true;
	}

	return false;
}

void NaviManager::injectNaviMaterialMouseDown(const std::string &naviName, int buttonID, int xPos, int yPos)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		if(iter->second->isMaterialOnly)
			LLMozLib::getInstance()->mouseDown(iter->second->windowID, xPos, yPos);
}

bool NaviManager::injectMouseUp(int buttonID)
{
	if(buttonID == LeftMouseButton)
	{
		if(focusedNavi)
		{
			int relX = focusedNavi->getRelativeX(mouseXPos);
			int relY = focusedNavi->getRelativeY(mouseYPos);

			LLMozLib::getInstance()->mouseUp(focusedNavi->windowID, relX, relY);
			return true;
		}
	}
	else if(buttonID == RightMouseButton)
	{
		if(focusedNavi && mouseButtonRDown && mouse) if(focusedNavi->movable) mouse->activateCursor(mouse->defaultCursorName);
		mouseButtonRDown = false;
		if(focusedNavi) return true;
	}

	return false;
}

void NaviManager::injectNaviMaterialMouseUp(const std::string &naviName, int buttonID, int xPos, int yPos)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		if(iter->second->isMaterialOnly)
			LLMozLib::getInstance()->mouseUp(iter->second->windowID, xPos, yPos);
}

void NaviManager::addNaviEventListener(const std::string &naviName, NaviEventListener* newListener)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		iter->second->addEventListener(newListener);
}

void NaviManager::removeNaviEventListener(const std::string &naviName, NaviEventListener* removeListener)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		iter->second->removeEventListener(removeListener);
}

void NaviManager::bindNaviData(const std::string &naviName, const std::string &naviDataName, const NaviDelegate &callback)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		iter->second->bindNaviData(naviDataName, callback);
}

void NaviManager::unbindNaviData(const std::string &naviName, const std::string &naviDataName, const NaviDelegate &callback)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		iter->second->unbindNaviData(naviDataName, callback);
}

void NaviManager::focusNavi(int x, int y)
{
	deFocusAllNavis();
	Navi* naviToFocus = 0;

	std::vector<Navi*> possibleNavis = getNavisAtPoint(x, y);

	if(possibleNavis.size())
	{
		try {
			naviToFocus = possibleNavis.at(0);
		} catch(...) {
			naviToFocus = 0;
		}
	} else naviToFocus = 0;

	if(naviToFocus)
	{
		std::vector<Navi*> sortedNavis = getNavis();

		if(sortedNavis.size())
		{
			if(sortedNavis.at(0) != naviToFocus)
			{
				// Find the Navi to pop to the top
				unsigned int popIdx = 0;
				for(; popIdx < sortedNavis.size(); popIdx++)
					if(sortedNavis.at(popIdx) == naviToFocus)
						break;

				unsigned short highestZ = sortedNavis.at(0)->overlay->getZOrder();
				// 'Sink' z-orders of Navis above the one to pop
				for(unsigned int i = 0; i < popIdx; i++)
					sortedNavis.at(i)->overlay->setZOrder(sortedNavis.at(i+1)->overlay->getZOrder());
				
				// Pop Navi to the top
				sortedNavis.at(popIdx)->overlay->setZOrder(highestZ);
			}
		}

		focusedNavi = naviToFocus;
		LLMozLib::getInstance()->focusBrowser(naviToFocus->windowID, true);
	}
}

std::vector<Navi*>& NaviManager::getNavisAtPoint(int x, int y)
{
	static std::vector<Navi*> result;
	if(result.size()) result.clear();

	// Find all Navis that are at a certain position
	for(iter = activeNavis.begin(); iter != activeNavis.end(); iter++)
	{
		Navi* test = iter->second;

		if(test->isPointOverMe(x, y))
			result.push_back(test);
	}

	// Of the result, sort Navis descending by ZOrder
	std::sort(result.begin(), result.end(), cmpZOrder);

	return result;
}

std::vector<Navi*>& NaviManager::getNavis()
{
	static std::vector<Navi*> result;
	if(result.size()) result.clear();

	// Find all real Navis, push each to the result vector
	for(iter = activeNavis.begin(); iter != activeNavis.end(); iter++)
	{
		if(!iter->second->isMaterialOnly)
			result.push_back(iter->second);
	}

	// Of the result, sort Navis descending by ZOrder
	std::sort(result.begin(), result.end(), cmpZOrder);

	return result;
}

void NaviManager::deFocusAllNavis()
{
	for(iter = activeNavis.begin(); iter != activeNavis.end(); iter++)
		LLMozLib::getInstance()->focusBrowser(iter->second->windowID, false);

	focusedNavi = 0;

	// A true and total HACK to get rid of internal Mozilla keyboard focus
	// Double-clicks a hidden browser window
	if(hiddenWindowID)
	{
		LLMozLib::getInstance()->focusBrowser(hiddenWindowID, true);
		LLMozLib::getInstance()->mouseMove(hiddenWindowID, 64, 64);
		LLMozLib::getInstance()->mouseDown(hiddenWindowID, 64, 64);
		LLMozLib::getInstance()->mouseUp(hiddenWindowID, 64, 64);
		LLMozLib::getInstance()->mouseDown(hiddenWindowID, 64, 64);
		LLMozLib::getInstance()->mouseUp(hiddenWindowID, 64, 64);
	}
}

