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

#include "NaviData.h"
#include <sstream>
#include "OgreException.h"
#include "NaviUtilities.h"

using namespace NaviLibrary;

NaviData::NaviData(const std::string &_dataName)
{
	dataName = _dataName;
	dataString = "";
	paramCounter = 0;
}

NaviData::NaviData(const std::string &_dataName, const std::string &_dataString)
{
	dataName = _dataName;
	dataString = _dataString;

	if(_dataString.length())
		paramCounter = 1;
	else
		paramCounter = 0;
}

NaviData::~NaviData()
{

}

void NaviData::add(const std::string &paramName, const std::string &paramValue)
{
	add(paramName, toWide(paramValue));
}

void NaviData::add(const std::string &paramName, const std::wstring &paramValue)
{
	std::string paramString = paramName + "=" + encodeURIComponent(paramValue);

	if(paramCounter)
		dataString += "&";

	dataString += paramString;
	paramCounter++;
}

void NaviData::add(const std::string &paramName, int paramValue)
{
	std::stringstream paramConverter;
	paramConverter << paramValue;

	std::string paramString = paramName + "=" + paramConverter.str();

	if(paramCounter)
		dataString += "&";

	dataString += paramString;
	paramCounter++;
}

void NaviData::add(const std::string &paramName, float paramValue)
{
	std::stringstream paramConverter;
	paramConverter << paramValue;

	std::string paramString = paramName + "=" + paramConverter.str();

	if(paramCounter)
		dataString += "&";

	dataString += paramString;
	paramCounter++;
}

std::string NaviData::getName() const
{
	return dataName;
}

bool NaviData::isNamed(std::string testName, bool caseSensitive) const
{
	if(!caseSensitive)
		return lowerString(getName()) == lowerString(testName);
	
	return getName() == testName;
}

bool NaviData::get(const std::string &paramName, std::wstring &paramValOut, bool caseSensitive) const
{
	size_t idx;

	if(caseSensitive)
		idx = dataString.find(paramName);
	else
		idx = lowerString(dataString).find(lowerString(paramName));

	if(idx != std::string::npos)
	{
		//Found paramName
		idx = dataString.find_first_of("=", idx);
		if(idx != std::string::npos)
		{
			idx++;
			size_t endIdx = dataString.find_first_of("&", idx);

			if(endIdx != std::string::npos)
			{
				paramValOut = decodeURIComponent(dataString.substr(idx, endIdx-idx));
				return true;
			}
			else
			{
				// No further params, safe to return all to the end
				paramValOut = decodeURIComponent(dataString.substr(idx));
				return true;
			}
		}
	}

	return false;
}

bool NaviData::get(const std::string &paramName, std::string &paramValOut, bool caseSensitive) const
{
	std::wstring temp;
	if(get(paramName, temp, caseSensitive))
	{
		paramValOut = toMultibyte(temp);
		return true;
	}

	return false;
}

bool NaviData::get(const std::string &paramName, int &paramValOut, bool caseSensitive) const
{
	std::string paramValString;
	if(get(paramName, paramValString, caseSensitive))
	{
		std::istringstream i(paramValString);
		int x;
		if (!(i >> x))
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Could not convert Navi Parameter: \"" + paramName 
				+ "\" with a value of: \"" + paramValString + "\" to an integer.",  "NaviData::get");
		}
		else
		{
			paramValOut = x;
			return true;
		}
	}

	return false;
}

bool NaviData::get(const std::string &paramName, float &paramValOut, bool caseSensitive) const
{
	std::string paramValString;
	if(get(paramName, paramValString, caseSensitive))
	{
		std::istringstream i(paramValString);
		float x;
		if (!(i >> x))
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Could not convert Navi Parameter: \"" + paramName 
				+ "\" with a value of: \"" + paramValString + "\" to a float.",  "NaviData::get");
		}
		else
		{
			paramValOut = x;
			return true;
		}
	}

	return false;
}

void NaviData::getDataMap(std::map<std::string,std::string> &dataMapOut) const
{
	std::string key, value;
	std::string mydataString = dataString;
	std::string::iterator it = mydataString.begin();
	while(it != mydataString.end())
	{
		key = "";
		while((*it) != '=')
		{
			key += (*it);
			++it;
		}
		++it;	//skip the '='
		value = "";
		while((it != mydataString.end()) && ((*it) != '&'))
		{
			value += (*it);
			++it;
		}

		dataMapOut[key] = toMultibyte(decodeURIComponent(value));

		if(it != mydataString.end())
			++it; //skip the '&'
	}
}
