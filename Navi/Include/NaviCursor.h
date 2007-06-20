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

#ifndef __NaviCursor_H__
#define __NaviCursor_H__
#if _MSC_VER > 1000
#pragma once
#endif

#include <string>
#include <vector>
#include <Ogre.h>

namespace NaviLibrary
{
	class NaviCursor
	{
		friend class NaviMouse;
		std::string name;
		unsigned short hsX;
		unsigned short hsY;
		struct Frame { unsigned short duration; std::string textureName; unsigned short index; };
		std::vector<Frame*> frames;
		Frame* curFrame;
		unsigned short frameCount;
		Ogre::Timer timer;
		unsigned long frameStartTime;
		bool lockedDuration;

		void update(bool force = false);
		NaviCursor(std::string cursorName, unsigned short hotspotX = 0, unsigned short hotspotY = 0);
		~NaviCursor();
	public:
		NaviCursor* addFrame(unsigned short duration, std::string imageFilename, 
			std::string imageResourceGroup = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	};

}

#endif