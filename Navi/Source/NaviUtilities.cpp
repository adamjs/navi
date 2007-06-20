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

#include "NaviUtilities.h"
#include "NaviManager.h"
#include <ctype.h>
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <direct.h>
#include <stdlib.h>
#endif

std::string NaviLibrary::getCurrentWorkingDirectory()
{
	std::string currentWorkingDirectory = "";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	char currentPath[_MAX_PATH];
	getcwd(currentPath, _MAX_PATH);
	currentWorkingDirectory = currentPath;
#endif

	return currentWorkingDirectory;
}

void NaviLibrary::translateLocalProtocols(std::string &strToTranslate)
{
	std::string localPath = "file:///";
	localPath += getCurrentWorkingDirectory();
	localPath += "/";

	std::string localNaviDir = NaviManager::Get().localNaviDirectory;
	if(localNaviDir.length())
	{
		localPath += localNaviDir;
		localPath += "/";
	}
	
	NaviLibrary::replaceAll(strToTranslate, "local://", localPath);
}

void NaviLibrary::translateResourceProtocols(std::string &strToTranslate)
{
	std::string resourceProtocol = "\"resource://";
	std::string resourceDataURI = "";
	std::string fileName = "";
	std::string groupName = "";
	size_t beginIdx = std::string::npos;
	size_t endIdx = std::string::npos;
	size_t idx = std::string::npos;
	size_t testIdx = std::string::npos;

	beginIdx = strToTranslate.find(resourceProtocol);
	while(beginIdx != std::string::npos)
	{
		// Found 'resource://' protocol prefix
		idx = beginIdx + resourceProtocol.length();
		endIdx = strToTranslate.find_first_of("\"", idx);

		// Check for malformed statements
		if(endIdx != std::string::npos)
		{
			// Sanity-check for more malformed statements
			testIdx = strToTranslate.find(">", beginIdx);
			if(testIdx > endIdx || testIdx == std::string::npos)
			{
				idx = strToTranslate.find_first_of("/", idx);

				if(idx == std::string::npos || idx > endIdx)
				{
					// Didn't find any further '/', Not specifying resource group
					std::string fileName = strToTranslate.substr(beginIdx + resourceProtocol.length(), endIdx - (beginIdx + resourceProtocol.length()));
					resourceDataURI = resourceToDataURI(fileName);
				}
				else
				{
					std::string groupName = strToTranslate.substr(beginIdx + resourceProtocol.length(), idx - (beginIdx + resourceProtocol.length()));
					std::string fileName = strToTranslate.substr(idx + 1, endIdx - (idx + 1));
					resourceDataURI = resourceToDataURI(fileName, groupName);
				}

				if(resourceDataURI.length())
				{
					resourceDataURI = "\"" + resourceDataURI;
					strToTranslate.erase(beginIdx, (endIdx - beginIdx));
					strToTranslate.insert(beginIdx, resourceDataURI);
				}

				beginIdx = strToTranslate.find(resourceProtocol, beginIdx + 1);
				resourceDataURI = "";
			}
			else // Uh oh! Malformed statement!
			{
				if(testIdx != std::string::npos)
					beginIdx = strToTranslate.find(resourceProtocol, testIdx + 1);
				else
					beginIdx = std::string::npos;
			}
		}
		else // Uh oh! Malformed statement!
			beginIdx = std::string::npos;
	}
}

std::string NaviLibrary::htmlToDataURI(std::string htmlString)
{
	translateLocalProtocols(htmlString);
	translateResourceProtocols(htmlString);
	
	std::string encodedDataURI = "data:";
	encodedDataURI += "text/html";
	encodedDataURI += ";base64,";
	encodedDataURI += encodeBase64(htmlString);

	return encodedDataURI;
}

std::string NaviLibrary::resourceToDataURI(const std::string &resFileName, const std::string &resourceGroupName)
{
	std::string resourceDataURI = "";

	if(!Ogre::ResourceGroupManager::getSingleton().resourceExists(resourceGroupName, resFileName))
	{
		Ogre::LogManager::getSingleton().logMessage("WARNING! In NaviLibrary::resourceToDataURI, the resource '" + resFileName + "' in resource group '"
			+ resourceGroupName + "' does not exist!");
		return "";
	}

	Ogre::DataStreamPtr pStream = Ogre::ResourceGroupManager::getSingleton().openResource(resFileName, resourceGroupName);
	Ogre::String data = pStream->getAsString();

	size_t idx = resFileName.find_last_of(".");

	if(idx == std::string::npos)
		return "";
	else
	{
		std::string fExt = resFileName.substr(idx+1);
		std::string mimeType = "application/octet-stream";

		if(fExt == "html" || fExt == "htm" || fExt == "shtml")
			return htmlToDataURI(data);

		// This represents a long night of work
		if(fExt == "php")
		{
			translateLocalProtocols(data);
			translateResourceProtocols(data);
			mimeType = "text/php";
		}
		else if(fExt == "asp")
		{
			translateLocalProtocols(data);
			translateResourceProtocols(data);
			mimeType = "text/asp";
		}
		else if(fExt == "xhtml")
		{
			translateLocalProtocols(data);
			translateResourceProtocols(data);
			mimeType = "application/xhtml+xml";
		}
		else if(fExt == "xml")
		{
			translateLocalProtocols(data);
			translateResourceProtocols(data);
			mimeType = "text/xml";
		}
		else if(fExt == "js")
		{
			translateLocalProtocols(data);
			translateResourceProtocols(data);
			mimeType = "application/javascript";
		}
		else if(fExt == "cgi")
		{
			translateLocalProtocols(data);
			translateResourceProtocols(data);
			mimeType = "wwwserver/shellcgi";
		}
		else if(fExt == "css")
		{
			translateLocalProtocols(data);
			translateResourceProtocols(data);
			mimeType = "text/css";
		}
		else if(fExt == "swf") mimeType = "application/x-shockwave-flash";
		else if(fExt == "mml") mimeType = "application/mathml+xml";
		else if(fExt == "svg") mimeType = "image/svg+xml";
		else if(fExt == "xslt") mimeType = "application/xslt+xml";
		else if(fExt == "rss") mimeType = "application/rss+xml";
		else if(fExt == "txt") mimeType = "text/plain";
		else if(fExt == "rtf") mimeType = "text/rtf";
		else if(fExt == "doc") mimeType = "application/doc";
		else if(fExt == "gif") mimeType = "image/gif";
		else if(fExt == "jpg" || fExt == "jpeg") mimeType = "image/jpeg";
		else if(fExt == "png") mimeType = "image/png";
		else if(fExt == "tiff") mimeType = "image/tiff";
		else if(fExt == "tga" || fExt == "targa") mimeType = "image/tga";
		else if(fExt == "bmp") mimeType = "image/bmp";
		else if(fExt == "wbmp") mimeType = "image/vnd.wap.wbmp";
		else if(fExt == "pdf") mimeType = "application/pdf";
		else if(fExt == "flv") mimeType = "video/x-flv";
		else if(fExt == "mpg" || fExt == "mpeg") mimeType = "video/mpeg";
		else if(fExt == "mp4") mimeType = "video/mp4";
		else if(fExt == "mov") mimeType = "video/quicktime";
		else if(fExt == "wmv") mimeType = "video/x-ms-wmv";
		else if(fExt == "rm") mimeType = "application/vnd.rn-realmedia";
		else if(fExt == "ogg") mimeType = "application/ogg";
		else if(fExt == "mp3") mimeType = "audio/mpeg";
		else if(fExt == "wav") mimeType = "audio/wav";
		else if(fExt == "wma") mimeType = "audio/x-ms-wma";
		else if(fExt == "asf") mimeType = "application/asx";
		else if(fExt == "au") mimeType = "audio/au";
		else if(fExt == "mid" || fExt == "midi") mimeType = "audio/midi";

		resourceDataURI = "data:";
		resourceDataURI += mimeType;
		resourceDataURI += ";base64,";
		resourceDataURI += encodeBase64(data);

		return resourceDataURI;
	}

	return "";
}

std::string NaviLibrary::escapeString(std::string strToEscape)
{
	NaviLibrary::replaceAll(strToEscape, "%", "%25", true);
	NaviLibrary::replaceAll(strToEscape, " ", "%20");
	NaviLibrary::replaceAll(strToEscape, "\"", "%22");
	NaviLibrary::replaceAll(strToEscape, "`", "%60");
	NaviLibrary::replaceAll(strToEscape, "\\", "%5C");
	NaviLibrary::replaceAll(strToEscape, "#", "%23");
	NaviLibrary::replaceAll(strToEscape, "<", "%3C");
	NaviLibrary::replaceAll(strToEscape, ">", "%3E");
	NaviLibrary::replaceAll(strToEscape, "&", "%26");
	NaviLibrary::replaceAll(strToEscape, "=", "%3D");
	NaviLibrary::replaceAll(strToEscape, "?", "%3F");
		
	return strToEscape;
}

std::string NaviLibrary::unescapeString(std::string strToUnescape)
{
	NaviLibrary::replaceAll(strToUnescape, "%20", " ");
	NaviLibrary::replaceAll(strToUnescape, "%22", "\"");
	NaviLibrary::replaceAll(strToUnescape, "%60", "`");
	NaviLibrary::replaceAll(strToUnescape, "%5C", "\\");
	NaviLibrary::replaceAll(strToUnescape, "%23", "#");
	NaviLibrary::replaceAll(strToUnescape, "%3C", "<");
	NaviLibrary::replaceAll(strToUnescape, "%3E", ">");
	NaviLibrary::replaceAll(strToUnescape, "%26", "&");
	NaviLibrary::replaceAll(strToUnescape, "%3D", "=");
	NaviLibrary::replaceAll(strToUnescape, "%3F", "?");
	NaviLibrary::replaceAll(strToUnescape, "%25", "%");

	return strToUnescape;
}

std::string NaviLibrary::lowerString(std::string strToLower)
{	
	for(unsigned int i = 0; i < strToLower.length(); i++)
		strToLower[i] = tolower(strToLower[i]);

	return strToLower;
}

void NaviLibrary::replaceAll(std::string &sourceStr, const std::string &replaceWhat, const std::string &replaceWith, bool avoidCyclic)
{
	if(avoidCyclic)
	{
		std::string shadowSourceStr = sourceStr;
		std::string shadowReplaceWith = replaceWith;
		NaviLibrary::replaceAll(shadowReplaceWith, "%", "_", false);

		size_t idx = shadowSourceStr.find(replaceWhat);
		while(idx != std::string::npos)
		{
			sourceStr.erase(idx, replaceWhat.length());
			shadowSourceStr.erase(idx, replaceWhat.length());
			sourceStr.insert(idx, replaceWith);
			shadowSourceStr.insert(idx, shadowReplaceWith);
			idx = shadowSourceStr.find(replaceWhat);
		}
	}
	else
	{
		size_t idx = sourceStr.find(replaceWhat);
		while(idx != std::string::npos)
		{
			sourceStr.erase(idx, replaceWhat.length());
			sourceStr.insert(idx, replaceWith);
			idx = sourceStr.find(replaceWhat);
		}
	}
}

bool NaviLibrary::hexStringToRGB(const std::string& hexString, unsigned char &R, unsigned char &G, unsigned char &B)
{
	if(hexString.length() == 7)
	{
		if(hexString.find("#") == 0 && hexString.substr(1).find_first_not_of("0123456789ABCDEFabcdef") == std::string::npos)
		{
			R = static_cast<unsigned char>(strtoul(hexString.substr(1, 2).c_str(), 0, 16));
			G = static_cast<unsigned char>(strtoul(hexString.substr(3, 2).c_str(), 0, 16));
			B = static_cast<unsigned char>(strtoul(hexString.substr(5, 2).c_str(), 0, 16));

			return true;
		}
	}

	return false;
}

int NaviLibrary::colorDistanceRGB(const unsigned char &R1, const unsigned char &G1, const unsigned char &B1, const unsigned char &R2, const unsigned char &G2, const unsigned char &B2)
{
	return abs((int)R1 - (int)R2) + abs((int)G1 - (int)G2) + abs((int)B1 - (int)B2);
}

/**
* The following is freely-available code by René Nyffenegger
* Obtained from: http://www.adp-gmbh.ch/cpp/common/base64.html
*/

static inline bool is_base64(unsigned char c) 
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}

static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

std::string NaviLibrary::encodeBase64(const std::string &strToEncode)
{
	unsigned char const* bytes_to_encode = reinterpret_cast<const unsigned char*>(strToEncode.c_str());
	unsigned int in_len = (unsigned int)strToEncode.length();

	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--) {
	char_array_3[i++] = *(bytes_to_encode++);
	if (i == 3) {
	  char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
	  char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
	  char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
	  char_array_4[3] = char_array_3[2] & 0x3f;

	  for(i = 0; (i <4) ; i++)
		ret += base64_chars[char_array_4[i]];
	  i = 0;
	}
	}

	if (i)
	{
	for(j = i; j < 3; j++)
	  char_array_3[j] = '\0';

	char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
	char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
	char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
	char_array_4[3] = char_array_3[2] & 0x3f;

	for (j = 0; (j < i + 1); j++)
	  ret += base64_chars[char_array_4[j]];

	while((i++ < 3))
	  ret += '=';

	}

	return ret;
}