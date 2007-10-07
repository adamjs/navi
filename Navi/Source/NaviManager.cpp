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
using namespace NaviLibrary::NaviUtilities;

template<> NaviManager* Singleton<NaviManager>::instance = 0;

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

NaviManager::NaviManager(Ogre::RenderWindow* _renderWindow, const std::string &_localNaviDirectory)
{
	focusedNavi = 0;
	hiddenWindowID = 0;
	mouseXPos = mouseYPos = 0;
	mouseButtonRDown = false;
	zOrderCounter = 5;
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
}

NaviManager::~NaviManager()
{
	for(iter = activeNavis.begin(); iter != activeNavis.end();)
	{
		Navi* toDelete = iter->second;
		iter = activeNavis.erase(iter);
		delete toDelete;
	}

	if(hiddenWindowID)
		LLMozLib::getInstance()->destroyBrowserWindow(hiddenWindowID);

	LLMozLib::getInstance()->clearCache();
	LLMozLib::getInstance()->reset();

	if(NaviMouse::GetPointer())
		delete NaviMouse::GetPointer();
}

NaviManager& NaviManager::Get()
{
	if(!instance)
		OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED, 
			"An attempt was made to retrieve the NaviManager Singleton before it has been instantiated! Did you forget to do 'new NaviManager(renderWin)'?", 
			"NaviManager::Get");

	return *instance;
}

NaviManager* NaviManager::GetPointer()
{
	return instance;
}

void NaviManager::Update()
{
	std::map<std::string,Navi*>::iterator end;
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

	if(NaviMouse::GetPointer()) 
		NaviMouse::GetPointer()->update();
}

Navi* NaviManager::createNavi(const std::string &naviName, const std::string &homepage,  const NaviPosition &naviPosition,
							  unsigned short width, unsigned short height, unsigned short zOrder)
{
	if(!zOrder)
		zOrder = zOrderCounter++;

	if(activeNavis.find(naviName) != activeNavis.end())
		OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED, 
			"An attempt was made to create a Navi named '" + naviName + "' when a Navi by the same name already exists!", 
			"NaviManager::createNavi");

	return activeNavis[naviName] = new Navi(renderWindow, naviName, homepage, naviPosition, width, height, zOrder);
}

Navi* NaviManager::createNaviMaterial(const std::string &naviName, const std::string &homepage, unsigned short width, unsigned short height,
									  Ogre::FilterOptions texFiltering)
{
	if(activeNavis.find(naviName) != activeNavis.end())
		OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED, 
			"An attempt was made to create a Navi named '" + naviName + "' when a Navi by the same name already exists!", 
			"NaviManager::createNaviMaterial");

	return activeNavis[naviName] = new Navi(renderWindow, naviName, homepage, width, height, texFiltering);
}

Navi* NaviManager::getNavi(const std::string &naviName)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		return iter->second;

	return 0;
}

void NaviManager::destroyNavi(const std::string &naviName)
{
	iter = activeNavis.find(naviName);
	if(iter != activeNavis.end())
		iter->second->okayToDelete = true;
}

void NaviManager::destroyNavi(Navi* naviToDestroy)
{
	if(naviToDestroy)
		naviToDestroy->okayToDelete = true;
}

void NaviManager::resetAllPositions()
{
	for(iter = activeNavis.begin(); iter != activeNavis.end(); iter++)
		if(!iter->second->isMaterial)
			iter->second->resetPosition();
}

bool NaviManager::isAnyNaviFocused()
{
	if(focusedNavi)
		return true;

	return false;
}

Navi* NaviManager::getFocusedNavi()
{
	return focusedNavi;
}

bool NaviManager::injectMouseMove(int xPos, int yPos)
{
	bool eventHandled = false;

	if(mouseButtonRDown && focusedNavi)
	{
		if(focusedNavi->movable)
			focusedNavi->moveNavi(xPos-mouseXPos, yPos-mouseYPos);

		eventHandled = true;
	}
	else
	{
		Navi* top = getTopNavi(xPos, yPos);

		if(top)
		{
			top->injectMouseMove(top->getRelativeX(xPos), top->getRelativeY(yPos));
			eventHandled = true;

			for(iter = activeNavis.begin(); iter != activeNavis.end(); ++iter)
				if(iter->second->ignoringBounds)
					if(!(iter->second->isPointOverMe(xPos, yPos) && iter->second->panel->getZOrder() < top->panel->getZOrder()))
						iter->second->injectMouseMove(iter->second->getRelativeX(xPos), iter->second->getRelativeY(yPos));
		}
	}

	mouseXPos = xPos;
	mouseYPos = yPos;

	if(NaviMouse::GetPointer()) NaviMouse::GetPointer()->move(xPos, yPos);

	return eventHandled;
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

bool NaviManager::injectMouseDown(int buttonID)
{
	if(buttonID == LeftMouseButton)
	{
		if(focusNavi(mouseXPos, mouseYPos))
		{
			int relX = focusedNavi->getRelativeX(mouseXPos);
			int relY = focusedNavi->getRelativeY(mouseYPos);
			
			LLMozLib::getInstance()->mouseDown(focusedNavi->windowID, relX, relY);
		}
	}
	else if(buttonID == RightMouseButton)
	{
		mouseButtonRDown = true;
		
		if(focusNavi(mouseXPos, mouseYPos) && NaviMouse::GetPointer())
			if(focusedNavi->movable) NaviMouse::GetPointer()->activateCursor("move");
	}

	if(focusedNavi)
		return true;

	return false;
}

bool NaviManager::injectMouseUp(int buttonID)
{
	if(buttonID == LeftMouseButton && focusedNavi)
		focusedNavi->injectMouseUp(focusedNavi->getRelativeX(mouseXPos), focusedNavi->getRelativeY(mouseYPos));
	else if(buttonID == RightMouseButton)
	{
		if(focusedNavi && mouseButtonRDown && NaviMouse::GetPointer()) 
			if(focusedNavi->movable)
				NaviMouse::GetPointer()->activateCursor(NaviMouse::GetPointer()->defaultCursorName);

		mouseButtonRDown = false;
	}

	if(focusedNavi)
		return true;

	return false;
}

bool NaviManager::focusNavi(int x, int y, Navi* selection)
{
	deFocusAllNavis();
	Navi* naviToFocus = selection? selection : getTopNavi(x, y);

	if(!naviToFocus)
		return false;

	std::vector<Navi*> sortedNavis;

	for(iter = activeNavis.begin(); iter != activeNavis.end(); iter++)
		if(!iter->second->isMaterial)
			sortedNavis.push_back(iter->second);

	struct compare { bool operator()(Navi* a, Navi* b){ return(a->overlay->getZOrder() > b->overlay->getZOrder()); }};
	std::sort(sortedNavis.begin(), sortedNavis.end(), compare());

	if(sortedNavis.size())
	{
		if(sortedNavis.at(0) != naviToFocus)
		{
			unsigned int popIdx = 0;
			for(; popIdx < sortedNavis.size(); popIdx++)
				if(sortedNavis.at(popIdx) == naviToFocus)
					break;

			unsigned short highestZ = sortedNavis.at(0)->overlay->getZOrder();
			for(unsigned int i = 0; i < popIdx; i++)
				sortedNavis.at(i)->overlay->setZOrder(sortedNavis.at(i+1)->overlay->getZOrder());
			
			sortedNavis.at(popIdx)->overlay->setZOrder(highestZ);
		}
	}

	focusedNavi = naviToFocus;
	LLMozLib::getInstance()->focusBrowser(naviToFocus->windowID, true);

	return true;
}

Navi* NaviManager::getTopNavi(int x, int y)
{
	Navi* top = 0;

	for(iter = activeNavis.begin(); iter != activeNavis.end(); iter++)
	{
		if(!iter->second->isPointOverMe(x, y))
			continue;

		if(!top)
			top = iter->second;
		else
			top = top->panel->getZOrder() > iter->second->panel->getZOrder()? top : iter->second;
	}

	return top;
}

void NaviManager::deFocusAllNavis()
{
	for(iter = activeNavis.begin(); iter != activeNavis.end(); iter++)
		LLMozLib::getInstance()->focusBrowser(iter->second->windowID, false);

	focusedNavi = 0;

	// A true and total HACK to get rid of internal Mozilla keyboard focus
	// Clicks a hidden browser window
	if(hiddenWindowID)
	{
		LLMozLib::getInstance()->focusBrowser(hiddenWindowID, true);
		LLMozLib::getInstance()->mouseDown(hiddenWindowID, 64, 64);
		LLMozLib::getInstance()->mouseUp(hiddenWindowID, 64, 64);
	}
}