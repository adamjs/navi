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

#include "NaviMouse.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

using namespace NaviLibrary;

NaviMouse::NaviMouse()
{
	mouseX = mouseY = 0;
	activeCursor = 0;
	defaultCursorName = "";

	// Create the texture
	Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().createManual(
		"NaviMouseTexture", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::TEX_TYPE_2D, 64, 64, 0, Ogre::PF_BYTE_BGRA,
		Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE, 0);

	Ogre::HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD);
	const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();

	Ogre::uint8* pDest = static_cast<Ogre::uint8*>(pixelBox.data);

	// Fill the texture with a transparent color
	for(size_t i = 0; i < (size_t)(64*64*4); i++)
	{
		if((i+1)%4)	
			pDest[i] = 64; // B, G, R
		else 
			pDest[i] = 0; // A
	}

	pixelBuffer->unlock();

	Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().create("NaviMouseMaterial", 
		Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	material->getTechnique(0)->getPass(0)->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
	material->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
	material->getTechnique(0)->getPass(0)->createTextureUnitState("NaviMouseTexture")->setTextureFiltering(Ogre::FO_NONE, Ogre::FO_NONE, Ogre::FO_NONE);

	Ogre::OverlayManager& overlayManager = Ogre::OverlayManager::getSingleton();

	panel = static_cast<Ogre::OverlayContainer*>(overlayManager.createOverlayElement("Panel", "NaviMousePanel"));
	panel->setMetricsMode(Ogre::GMM_PIXELS);
	panel->setPosition(0, 0);
	panel->setDimensions(64, 64);
	panel->setMaterialName("NaviMouseMaterial");

	overlay = overlayManager.create("NaviMouseOverlay");
	overlay->add2D(panel);
	overlay->setZOrder(650);
	overlay->show();

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	ShowCursor(false);
#endif
}

NaviMouse::~NaviMouse()
{
	std::map<std::string, NaviCursor*>::iterator iter = cursors.begin();
	while(iter != cursors.end())
	{
		NaviCursor* toDelete = iter->second;
		iter = cursors.erase(iter);
		delete toDelete;
	}

	if(overlay) Ogre::OverlayManager::getSingletonPtr()->destroy(overlay);

	Ogre::MaterialManager::getSingletonPtr()->remove("NaviMouseMaterial");
	Ogre::TextureManager::getSingletonPtr()->remove("NaviMouseTexture");
}

NaviCursor* NaviMouse::createCursor(std::string cursorName, unsigned short hotspotX, unsigned short hotspotY)
{
	if(cursorName.empty()) 
		OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, 
			"A NaviCursor was attempted to be created with an empty name!", 
			"NaviMouse::createCursor");

	if(cursors.find(cursorName) == cursors.end())
	{
		NaviCursor* newCursor = new NaviCursor(cursorName, hotspotX, hotspotY);
		cursors[cursorName] = newCursor;
		return newCursor;
	}
	else
	{
		std::string errMsg = "A NaviCursor named '";
		errMsg += cursorName;
		errMsg += "' already exists! Could not create a new NaviCursor.";
		OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, 
			errMsg, "NaviMouse::createCursor");
	}

	return 0;
}

void NaviMouse::setDefaultCursor(std::string cursorName)
{
	if(cursorName.empty()) 
		OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, 
			"An attempt was made to set the default cursor with an empty name!", 
			"NaviMouse::setDefaultCursor");

	std::map<std::string, NaviCursor*>::iterator i = cursors.find(cursorName);
	if(i != cursors.end())
	{
		activeCursor = i->second;
		defaultCursorName = cursorName;
	}
	else
	{
		std::string errMsg = "A NaviCursor named '";
		errMsg += cursorName;
		errMsg += "' does not exist! Could not set the default NaviCursor.";
		OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, 
			errMsg, "NaviMouse::setDefaultCursor");
	}

}

void NaviMouse::removeCursor(std::string cursorName)
{
	if(cursorName == defaultCursorName) return;

	std::map<std::string, NaviCursor*>::iterator i = cursors.find(cursorName);
	if(i != cursors.end())
	{
		NaviCursor* cursorToDelete = i->second;
		if(cursorToDelete == activeCursor)
			activateCursor("default");
		cursors.erase(i);
		delete cursorToDelete;
	}
}

void NaviMouse::activateCursor(std::string cursorName)
{
	if(cursorName == "default") cursorName = defaultCursorName;

	std::map<std::string, NaviCursor*>::iterator i = cursors.find(cursorName);
	
	if(i != cursors.end())
	{
		if(activeCursor == i->second) return;
		activeCursor = i->second;
		activeCursor->update(true);
		move(mouseX, mouseY);
	}
}

void NaviMouse::move(int x, int y)
{
	panel->setPosition(x-activeCursor->hsX, y-activeCursor->hsY);
	mouseX = x;
	mouseY = y;
}

void NaviMouse::update()
{
	if(activeCursor) activeCursor->update();
}
