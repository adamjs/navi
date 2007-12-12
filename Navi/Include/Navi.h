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

#ifndef __Navi_H__
#define __Navi_H__
#if _MSC_VER > 1000
#pragma once
#endif

#include "NaviPlatform.h"
#include "NaviManager.h"
#include <llmozlib.h>

namespace NaviLibrary
{
	class _NaviExport Navi : public LLEmbeddedBrowserWindowObserver, public Ogre::WindowEventListener, public Ogre::ManualResourceLoader
	{
		friend NaviManager;

		std::string naviName;
		unsigned short naviWidth;
		unsigned short naviHeight;
		unsigned int winWidth;
		unsigned int winHeight;
		Ogre::RenderWindow* renderWindow;
		bool isWinFocused;
		NaviPosition position;
		bool movable;
		int windowID;
		Ogre::Overlay* overlay;
		Ogre::PanelOverlayElement* panel;
		bool needsUpdate;
		unsigned int maxUpdatePS;
		bool forceMax;
		Ogre::Timer timer;
		unsigned long lastUpdateTime;
		float opacity;
		bool usingMask;
		bool ignoringTrans;
		float transparent;
		bool ignoringBounds;
		bool usingColorKeying;
		float keyFuzziness;
		unsigned char keyR, keyG, keyB;
		float keyFOpacity;
		unsigned char keyFillR, keyFillG, keyFillB;
		unsigned char* naviCache;
		bool isMaterial;
		std::vector<NaviEventListener*> eventListeners;
		std::multimap<std::string, NaviDelegate> delegateMap;
		std::multimap<std::string, NaviDelegate>::iterator delegateIter;
		std::pair<std::multimap<std::string, NaviDelegate>::iterator, std::multimap<std::string, NaviDelegate>::iterator> dmBounds;
		std::map<std::string, std::vector<std::string>> ensureKeysMap;
		std::map<std::string, std::vector<std::string>>::iterator ensureKeysMapIter;
		bool okayToDelete;
		bool isVisible;
		bool fadingOut;
		unsigned long fadingOutStart;
		unsigned long fadingOutEnd;
		bool fadingIn;
		unsigned long fadingInStart;
		unsigned long fadingInEnd;
		bool compensateNPOT;
		unsigned short texWidth;
		unsigned short texHeight;
		size_t texPixelSize;
		size_t texPitch;

		Navi(Ogre::RenderWindow* renderWin, std::string name, std::string homepage, const NaviPosition &naviPosition,
			unsigned short width, unsigned short height, unsigned short zOrder);

		Navi(Ogre::RenderWindow* renderWin, std::string name, std::string homepage, unsigned short width, unsigned short height, 
			Ogre::FilterOptions texFiltering);

		~Navi();

		void createOverlay(unsigned short zOrder);

		void createBrowser(Ogre::RenderWindow* renderWin, std::string homepage);

		void createMaterial(Ogre::FilterOptions texFiltering = Ogre::FO_NONE);

		void loadResource(Ogre::Resource* resource);

		void update();

		bool isPointOverMe(int x, int y);

		void onPageChanged(const EventType& eventIn);
		void onNavigateBegin(const EventType& eventIn);
		void onNavigateComplete(const EventType& eventIn);
		void onUpdateProgress(const EventType& eventIn);
		void onStatusTextChange(const EventType& eventIn);
		void onLocationChange(const EventType& eventIn);
		void onClickLinkHref(const EventType& eventIn);

		void windowMoved(Ogre::RenderWindow* rw);
		void windowResized(Ogre::RenderWindow* rw);
		void windowClosed(Ogre::RenderWindow* rw);
		void windowFocusChange(Ogre::RenderWindow* rw);
	public:

		/**
		* Navigates this Navi to a certain URL.
		*
		* @param	url		The URL (Web Address) to navigate to.
		*
		* @note	You may use local:// and resource:// specifiers for the URL.
		*/
		void navigateTo(std::string url);

		/**
		* Navigates this Navi to a certain URL along with encoded NaviData.
		*
		* @param	url		The URL (Web Address) to navigate to.
		*
		* @param	naviData	The NaviData to send to the page.
		*
		* @note	You may use local:// and resource:// specifiers for the URL.
		*
		* @note	This method of sending NaviData has been deprecated, use JS evaluation instead.
		*/
		void navigateTo(std::string url, const NaviData &naviData);

		/**
		* Navigates the internal browser of this Navi backwards, if possible.
		*/
		void navigateBack();

		/**
		* Navigates the internal browser of this Navi forwards, if possible.
		*/
		void navigateForward();

		/**
		* Immediately halts the loading of the current page, if one is loading.
		*/
		void navigateStop();

		/**
		* Returns whether or not the internal browser of this Navi can navigate backwards.
		*/
		bool canNavigateBack();

		/**
		* Returns whether or not the internal browser of this Navi can navigate backwards.
		*/
		bool canNavigateForward();

		/**
		* Evaluates Javascript in the context of the current page and returns the result.
		*
		* @param	script	The Javascript to evaluate/execute.
		*
		* @param	args	An optional vector of MultiValues that will be used in the translation
		*					of a templated string of Javascript.
		*
		* @return	If the action succeeds, this will return the result as a string (regardless
		*			of internal Javascript datatype), otherwise this returns an empty string.
		*
		* @note
		*	For example:
		*	\code
		*	myNavi->evaluateJS("$('myElement').setHTML('<b>Hello!</b>')");
		*
		*	// The following are examples of templated evaluation:
		*	myNavi->evaluateJS("newCharacter(?, ?, ?)", Args(name)(age)(naviData["motto"]));
		*	myNavi->evaluateJS("addChatMessage(?, ?)", Args(nickname)(someWideString));
		*	myNavi->evaluateJS("$(?).setHTML(?)", Args("helloLabel")("Hello world!"));
		*	myNavi->evaluateJS("?(?, ?)", Args("myFunction")(firstVar)(secondVar));
		*	myNavi->evaluateJS("$(?).SetVariable(?, ?)", Args("myFlashID")("flashVariable")(favoriteColor));
		*	\endcode
		*
		* @note	
		*	Strings in the Args of templated evaluation will automatically be quoted/escaped.
		*	Wide Strings will be encoded with NaviUtilities::encodeURIComponent and then wrapped in
		*	the Javascript decoding function: "decodeURIComponent(xxx)".
		*/
		std::string evaluateJS(std::string script, const NaviUtilities::Args &args = NaviUtilities::Args());

		Navi* addEventListener(NaviEventListener* newListener);

		Navi* removeEventListener(NaviEventListener* removeListener);

		Navi* bind(const std::string &naviDataName, const NaviDelegate &callback, const NaviUtilities::Strings &keys = NaviUtilities::Strings());

		Navi* unbind(const std::string &naviDataName, const NaviDelegate &callback = NaviDelegate());

		Navi* setBackgroundColor(float red, float green, float blue);

		Navi* setBackgroundColor(const std::string& hexColor);

		Navi* setColorKey(const std::string &keyColor, float keyFillOpacity = 0.0, const std::string &keyFillColor = "#000000", float keyFuzzy = 0.0);

		Navi* setForceMaxUpdate(bool forceMaxUpdate);

		Navi* setIgnoreBounds(bool ignoreBounds = true);

		Navi* setIgnoreTransparent(bool ignoreTrans, float threshold = 0.05);

		Navi* setMask(std::string maskFileName, std::string groupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

		Navi* setMaxUPS(unsigned int maxUPS = 0);

		Navi* setMovable(bool isMovable = true);

		Navi* setOpacity(float opacity);

		Navi* setPosition(const NaviPosition &naviPosition);

		Navi* resetPosition();

		Navi* hide(bool fade = false, unsigned short fadeDurationMS = 300);

		Navi* show(bool fade = false, unsigned short fadeDurationMS = 300);

		Navi* focus();

		Navi* moveNavi(int deltaX, int deltaY);

		void getExtents(unsigned short &width, unsigned short &height);

		int getRelativeX(int absX);

		int getRelativeY(int absY);

		bool isMaterialOnly();

		Ogre::PanelOverlayElement* getInternalPanel();

		std::string getName();

		std::string getMaterialName();

		bool getVisibility();

		void getDerivedUV(Ogre::Real& u1, Ogre::Real& v1, Ogre::Real& u2, Ogre::Real& v2);

		void injectMouseMove(int xPos, int yPos);

		void injectMouseWheel(int relScroll);

		void injectMouseDown(int xPos, int yPos);

		void injectMouseUp(int xPos, int yPos);
	};

}

#endif