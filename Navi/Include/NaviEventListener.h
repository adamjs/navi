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

#ifndef __NaviEventListener_H__
#define __NaviEventListener_H__
#if _MSC_VER > 1000
#pragma once
#endif

#include "NaviData.h"

namespace NaviLibrary
{
	/**
	* To receive event notifications about your Navis from the NaviManager, inherit from
	* this abstract class and invoke NaviManager::addNaviEventListener.
	*/
	class NaviEventListener
	{
	public:
		virtual ~NaviEventListener() {}

		/**
		* This is invoked when a NaviData-capable page sends NaviData
		*
		* @param	naviName	The name of the Navi invoking this event
		*
		* @param	naviData	The NaviData sent
		*/
		virtual void onNaviDataEvent(const std::string &naviName, const NaviData &naviData) = 0;

		/**
		* This is invoked when a user clicks a link on a Navi page
		*
		* @param	naviName	The name of the Navi invoking this event
		*
		* @param	linkHref	The value of the link clicked on
		*/
		virtual void onNaviLinkClicked(const std::string &naviName, const std::string &linkHref) = 0;
	};
}

#endif