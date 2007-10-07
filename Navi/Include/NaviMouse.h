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

#ifndef __NaviMouse_H__
#define __NaviMouse_H__
#if _MSC_VER > 1000
#pragma once
#endif

#include "NaviPlatform.h"
#include <map>
#include "NaviCursor.h"
#include "NaviSingleton.h"

namespace NaviLibrary
{
	/**
	* A simple little class that displays a mouse cursor using an Ogre Overlay that follows
	* the mouse coordinates that are injected into NaviManager.
	*/
	class _NaviExport NaviMouse : public Singleton<NaviMouse>
	{
		friend class NaviManager;
		int mouseX, mouseY;
		Ogre::Overlay* overlay;
		Ogre::OverlayContainer* panel;
		std::map<std::string, NaviCursor*> cursors;
		std::map<std::string, NaviCursor*>::iterator iter;
		NaviCursor* activeCursor;
		std::string defaultCursorName;
		bool visible;
		void move(int x, int y);
		void update();
		
	public:
		NaviMouse(bool visibility = true);

		~NaviMouse();

		static NaviMouse& Get();

		static NaviMouse* GetPointer();

		/**
		* Creates a cursor for use with this NaviMouse.
		*
		* @param	cursorName	The name of the cursor, used to activate or remove the cursor later.
		*
		* @param	hotspotX	Every cursor has a hot spot, which is used to define the single pixel a cursor 
		*						is pointing to when the mouse is clicked. This is the X-value of the hotspot.
		*
		* @param	hotspotY	The Y-value of the hotspot.
		*/
		NaviCursor* createCursor(std::string cursorName, unsigned short hotspotX = 0, unsigned short hotspotY = 0);

		/**
		* This should be called before NaviManager begins updating the mouse. Sets the default cursor for the mouse.
		*
		* @param	cursorName	The name of the cursor, from here on you may refer to this cursor by this cursorName
		*						or by "default", both will work. You may not remove a default cursor.
		*/
		void setDefaultCursor(std::string cursorName);

		/**
		* Removes a cursor from this NaviMouse.
		*
		* @param	cursorName	The cursor to remove. You may not remove a default cursor. If you try to remove a cursor
		*						that is currently active, the cursor will change to the default cursor after removal.
		*/
		void removeCursor(std::string cursorName);

		/**
		* Changes the active cursor to a specified cursor.
		*
		* @param	cursorName	The name of the cursor to change to. To change to the default cursor, simply specify "default".
		*						If the cursor cannot be found, nothing will happen.
		*/
		void activateCursor(std::string cursorName);

		/**
		* Displays the mouse cursor (if it is hidden via NaviMouse::hide).
		*/
		void show();

		/**
		* Hides the mouse cursor. Show it again via NaviMouse::show.
		*/
		void hide();
	};

}

#endif