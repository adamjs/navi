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
#include "OgreStringConverter.h"
#include "NaviUtilities.h"

using namespace NaviLibrary;
using namespace NaviLibrary::NaviUtilities;

NaviDataValue::NaviDataValue() { }

NaviDataValue::NaviDataValue(const std::string &value) { *this = value; }

NaviDataValue::NaviDataValue(const std::wstring &value) { *this = value; }

NaviDataValue::NaviDataValue(int value) { *this = value; }

NaviDataValue::NaviDataValue(float value) { *this = value; }

NaviDataValue::NaviDataValue(double value) { *this = value; }

NaviDataValue& NaviDataValue::operator=(const std::string &value)
{
	this->value = toWide(value);
	return *this;
}

NaviDataValue& NaviDataValue::operator=(const std::wstring &value)
{
	this->value = value;
	return *this;
}

NaviDataValue& NaviDataValue::operator=(int value)
{
	this->value = toWide(numberToString(value));
	return *this;
}

NaviDataValue& NaviDataValue::operator=(float value)
{
	this->value = toWide(numberToString(value));
	return *this;
}

NaviDataValue& NaviDataValue::operator=(double value)
{
	this->value = toWide(numberToString(value));
	return *this;
}

NaviDataValue& NaviDataValue::operator=(bool value)
{
	this->value = toWide(numberToString(value));
	return *this;
}

std::wstring NaviDataValue::wstr() const { return value; }

std::string NaviDataValue::str() const {return toMultibyte(value); }

bool NaviDataValue::isEmpty() const {	return value.empty(); }

bool NaviDataValue::isNumber() const { return isNumeric(toMultibyte(value)); }

int NaviDataValue::toInt() const { return toNumber<int>(toMultibyte(value)); }

float NaviDataValue::toFloat() const { return toNumber<float>(toMultibyte(value)); }

double NaviDataValue::toDouble() const  { return toNumber<double>(toMultibyte(value)); }

bool NaviDataValue::toBool() const { return toNumber<bool>(toMultibyte(value)); }


NaviData::NaviData(const std::string &name, const std::string &queryString)
{
	this->name = name;

	if(queryString.length())
	{
		std::map<std::string,std::string> tempMap = splitToMap(queryString, "&", "=", false);
		
		for(std::map<std::string,std::string>::iterator i = tempMap.begin(); i != tempMap.end(); ++i)
			data[i->first] = decodeURIComponent(i->second);
	}
}

bool NaviData::ensure(const std::string &key, bool throwOnFailure) const
{
	std::string keyName = key;
	bool checkNumeric = false;

	if(isPrefixed(keyName, "#"))
	{
		keyName = keyName.substr(1);
		checkNumeric = true;
	}

	std::map<std::string,NaviDataValue>::const_iterator iter = data.find(keyName);

	if(iter == data.end())
	{
		
		if(throwOnFailure)
		{
			std::stringstream errorMsg;
			errorMsg << "A piece of NaviData failed validation. The requested key does not exist." << std::endl
				<< "NaviData Name: " << name << std::endl
				<< "Key: " << keyName << std::endl << std::endl;

			OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED, errorMsg.str(), "NaviData::ensure");
		}

		return false;
	}

	if(checkNumeric)
	{

		if(!isNumeric(iter->second.str()))
		{
			if(throwOnFailure)
			{
				std::stringstream errorMsg;
				errorMsg << "A piece of NaviData failed validation. The value of the requested key is not numeric." << std::endl
				<< "NaviData Name: " << name << std::endl
				<< "Key: " << keyName << std::endl
				<< "Value: " << iter->second.str() << std::endl << std::endl;

				OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED, errorMsg.str(), "NaviData::ensure");
			}

			return false;
		}
	}

	return true;
}

bool NaviData::ensure(const std::vector<std::string> &keys, bool throwOnFailure) const
{
	for(std::vector<std::string>::const_iterator i = keys.begin(); i != keys.end(); ++i)
		if(!ensure(*i, throwOnFailure))
			return false;

	return true;
}

std::string NaviData::getName() const
{
	return name;
}

bool NaviData::exists(const std::string &keyName) const
{
	return data.find(keyName) != data.end();
}

const NaviDataValue& NaviData::operator[](const std::string &keyName) const
{
	std::map<std::string,NaviDataValue>::const_iterator iter = data.find(keyName);
	
	if(iter != data.end())
		return iter->second;

	static NaviDataValue empty;

	return empty;
}

NaviDataValue& NaviData::operator[](const std::string &keyName)
{
	return data[keyName];
}

int NaviData::size() const
{
	return (int)data.size();
}

const std::map<std::string,std::string>& NaviData::toStringMap(bool encodeVals) const
{
	static std::map<std::string,std::string> stringMap;
	if(stringMap.size()) stringMap.clear();

	for(std::map<std::string,NaviDataValue>::const_iterator i = data.begin(); i != data.end(); ++i)
		stringMap[i->first] = encodeVals ? encodeURIComponent(i->second.wstr()) : i->second.str();

	return stringMap;
}

const std::string& NaviData::toQueryString() const
{
	static std::string queryString;
	if(queryString.length()) queryString.clear();

	queryString = joinFromMap(toStringMap(true), "&", "=", false);

	return queryString;
}
