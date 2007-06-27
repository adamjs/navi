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

#ifndef __NaviUtilities_H__
#define __NaviUtilities_H__
#if _MSC_VER > 1000
#pragma once
#endif

#include <string>
#include <OgreResourceGroupManager.h>

namespace NaviLibrary
{
	/**
	* Gets the current working directory for the executable.
	*
	* @return	A string containing the current working directory.
	*/
	std::string getCurrentWorkingDirectory();

	/**
	* Converts a String containing HTML into an equivalent Data URI, replaces all 'local://'
	* and 'resource://group/filename' instances with their correct equivalent.
	*
	* @param	htmlString	The string containing the HTML to convert.
	*
	* @return	A string containing a Data URI with 'text/html' mime-type, data encoded in Base64.
	*/
	std::string htmlToDataURI(std::string htmlString);

	/**
	* Converts an Ogre Resource into an equivalent Data URI. Additional 'local://' and 'resource://'
	* specifier translation will occur on the following filetypes: '.html, .htm, .shtml, .php, .asp,
	* .xhtml, .xml, .js, .cgi, .css'
	*
	* @param	resFileName		The filename of the Ogre Resource
	*
	* @param	resourceGroupName	The name of the Ogre Resource Group to search in.
	*
	* @return	A string containing a Data URI with an automatically-identified mime-type, data encoded in Base64.
	*/
	std::string resourceToDataURI(const std::string &resFileName, const std::string &resourceGroupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

	/**
	* Replaces all 'local://' instances within a String with the correct NaviLocal directory.
	* (Ex: local://filename.html --> file:///C:\My Application\NaviLocal\filename.html)
	*/
	void translateLocalProtocols(std::string &strToTranslate);

	/**
	* Replaces all 'resource://Group/Filename.ext' with an equivalent Data URI of the Ogre Resource.
	* Syntax of 'resource://Filename.ext' assumes the Resource Group is DEFAULT_RESOURCE_GROUP_NAME.
	* If the Ogre Resource is not found, the 'resource://' specifier instance is skipped.
	*/
	void translateResourceProtocols(std::string &strToTranslate);
	
	/**
	* This is a C++ mirror implementation that I wrote for the Javascript function 'encodeURIComponent'.
	* For more info: http://developer.mozilla.org/en/docs/Core_JavaScript_1.5_Reference:Global_Functions:encodeURIComponent
	*
	* @param	strToEncode		The wide string to encode. You may use toWide() to convert a standard string to wide.
	*
	* @return	A standard string containing the encoded (UTF-8 percent-escaped) version of strToEncode.
	*/
	std::string encodeURIComponent(std::wstring strToEncode);

	/**
	* This is a C++ mirror implementation that I wrote for the Javascript function 'decodeURIComponent'.
	* For more info: http://developer.mozilla.org/en/docs/Core_JavaScript_1.5_Reference:Global_Functions:decodeURIComponent
	*
	* @param	strToDecode		The standard string to decode. Should be previously encoded by 'encodeURIComponent'.
	*
	* @return	A wide string containing the decoded version of strToDecode. You may use toMultibyte() to convert this to
	*			a standard string.
	*/
	std::wstring decodeURIComponent(std::string strToDecode);

	/**
	* Used internally. Converts a string into its lower-case equivalent.
	*/
	std::string lowerString(std::string strToLower);

	/**
	* Converts a multibyte string (standard string) to a wide string.
	*/
	std::wstring toWide(const std::string &stringToConvert);

	/**
	* Converts a wide string to a multibyte string (standard string), based on the current locale.
	*/
	std::string toMultibyte(const std::wstring &wstringToConvert);

	/**
	* Sets the current locale, used for 'toMultibyte()'. If you never call this, the default is 
	* usually "English'. Pass an empty string to set this to the current locale of the Operating System.
	*/
	void setLocale(const std::string &localeLanguage = "");

	/**
	* Used internally. Inside a string, replaces all instances of 'replaceWhat' with 'replaceWith'.
	*/
	void replaceAll(std::string &sourceStr, const std::string &replaceWhat, const std::string &replaceWith, bool avoidCyclic = false);

	/**
	* Used internally. Converts a Hex RGB String (in format "#XXXXXX") to R, G, B values. Returns whether
	* or not the conversion was successful.
	*/
	bool hexStringToRGB(const std::string& hexString, unsigned char &R, unsigned char &G, unsigned char &B);

	/**
	* Used internally. Returns a rough color distance between two RGB colors.
	*/
	int colorDistanceRGB(const unsigned char &R1, const unsigned char &G1, const unsigned char &B1, const unsigned char &R2, const unsigned char &G2, const unsigned char &B2);

	/** 
	* Used internally. Encodes a string in Base64.
	*/
	std::string encodeBase64(const std::string &strToEncode);
}

#endif