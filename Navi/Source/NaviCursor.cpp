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

#include "NaviCursor.h"
#include <stdlib.h>

using namespace NaviLibrary;
using namespace Ogre;

NaviCursor::NaviCursor(std::string cursorName, unsigned short hotspotX, unsigned short hotspotY)
{
	name = cursorName;
	hsX = hotspotX;
	hsY = hotspotY;
	frameCount = 0;
	curFrame = 0;
	frameStartTime = 0;
	lockedDuration = false;
}

NaviCursor::~NaviCursor()
{
	for(unsigned int i = 0; i < frames.size(); i++)
	{
		Frame* frmToDelete = frames.at(i);
		TextureManager::getSingleton().remove(frmToDelete->textureName);
		delete frmToDelete;
	}
}

NaviCursor* NaviCursor::addFrame(unsigned short durationMS, std::string imageFilename, std::string imageResourceGroup)
{
	Ogre::Image cursorFrameImg;
	cursorFrameImg.load(imageFilename, imageResourceGroup);

	std::string textureName = name;
	textureName += "Frame";
	char buffer[8];
	textureName += itoa(frameCount, buffer, 10);

	Ogre::TexturePtr maskTexture = Ogre::TextureManager::getSingleton().loadImage(
		textureName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		cursorFrameImg, Ogre::TEX_TYPE_2D, 0, 1, false, Ogre::PF_BYTE_BGRA);

	if(maskTexture->getWidth() < 64 || maskTexture->getHeight() < 64)
	OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, 
		"Cursor frame width and height cannot be less than 64.", 
		"NaviCursor::addFrame");

	Frame* newFrame = new Frame();
	newFrame->duration = durationMS;
	newFrame->textureName = textureName;
	newFrame->index = frameCount;

	frames.push_back(newFrame);

	if(!frameCount) curFrame = newFrame;

	frameCount++;

	return this;
}

void NaviCursor::update(bool force)
{
	if(!curFrame || (lockedDuration && !force)) return;
	if(force) lockedDuration = false;

	if(timer.getMilliseconds()-frameStartTime > curFrame->duration || force)
	{
		if(frameStartTime)
		{
			if(curFrame->index == frames.size()-1) curFrame = frames.at(0);
			else curFrame = frames.at(curFrame->index + 1);
		}

		Ogre::TexturePtr mouseTex = Ogre::TextureManager::getSingleton().getByName("NaviMouseTexture");
		Ogre::TexturePtr cursorTex = Ogre::TextureManager::getSingleton().getByName(curFrame->textureName);
		
		if(!mouseTex.isNull() && !cursorTex.isNull())
		{
			HardwarePixelBufferSharedPtr mousePBuff = mouseTex->getBuffer();
			mousePBuff->lock(HardwareBuffer::HBL_DISCARD);
			const PixelBox& mousePBox = mousePBuff->getCurrentLock();
			uint8* mouseData = static_cast<uint8*>(mousePBox.data);

			// It's unfortunate, but we must use an alternate route to read the buffer due to OpenGL problems
			HardwarePixelBufferSharedPtr cursorPBuff = cursorTex->getBuffer();
			uint8* cursorDataBuffer = new uint8[cursorPBuff->getSizeInBytes()];
			PixelBox cursorPBox(cursorPBuff->getWidth(), cursorPBuff->getHeight(), cursorPBuff->getDepth(), cursorPBuff->getFormat(), cursorDataBuffer);
			cursorPBuff->blitToMemory(cursorPBox);
			uint8* cursorData = static_cast<uint8*>(cursorPBox.data);
			
			size_t wOffset = 0;
			if(mouseTex->getWidth()-64 > 0) wOffset = (mouseTex->getWidth()-64)*4;
			size_t pitch = 256;

			for(size_t y = 0; y < (size_t)64; y++)
			{
				for(size_t x = 0; x < pitch; x += 4)
				{
					mouseData[y*pitch+x] = cursorData[(y*pitch)+(y*wOffset)+x];
					mouseData[y*pitch+x+1] = cursorData[(y*pitch)+(y*wOffset)+x+1];
					mouseData[y*pitch+x+2] = cursorData[(y*pitch)+(y*wOffset)+x+2];
					mouseData[y*pitch+x+3] = cursorData[(y*pitch)+(y*wOffset)+x+3];
				}
			}

			delete[] cursorDataBuffer;
			mousePBuff->unlock();

			if(curFrame->duration == 0) lockedDuration = true;
		}
		else
		{
			OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, 
				"When updating the texture of the NaviMouse Cursor, an internally-created dynamic texture that should have been there was not found.", 
				"NaviCursor::update");
		}

		frameStartTime = timer.getMilliseconds();
	}
}