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

#ifndef __NaviData_H__
#define __NaviData_H__
#if _MSC_VER > 1000
#pragma once
#endif

#include "NaviPlatform.h"
#include <string>
#include <map>
#include <OgreAny.h>

namespace NaviLibrary
{
	/**
	* A generic value container that can contain a string, wide string, integer, float,
	* double, or boolean value and can convert between them on-the-fly.
	*/
	class _NaviExport NaviDataValue
	{
		friend class NaviData;
		std::wstring value;
		
	public:
		/**
		* Creates an empty NaviDataValue.
		*/
		NaviDataValue();

		/**
		* Creates a NaviDataValue from a string.
		*/
		NaviDataValue(const std::string &value);

		/**
		* Creates a NaviDataValue from a wide string.
		*/
		NaviDataValue(const std::wstring &value);

		/**
		* Creates a NaviDataValue from an integer.
		*/
		NaviDataValue(int value);

		/**
		* Creates a NaviDataValue from a float.
		*/
		NaviDataValue(float value);

		/**
		* Creates a NaviDataValue from a double.
		*/
		NaviDataValue(double value);

		/**
		* Creates a NaviDataValue from a boolean.
		*/
		NaviDataValue(bool value);

		/**
		* Assigns this NaviDataValue a string value
		*/
		NaviDataValue& operator=(const std::string &value);

		/**
		* Assigns this NaviDataValue a wide string value
		*/
		NaviDataValue& operator=(const std::wstring &value);

		/**
		* Assigns this NaviDataValue an integer value
		*/
		NaviDataValue& operator=(int value);

		/**
		* Assigns this NaviDataValue a float value
		*/
		NaviDataValue& operator=(float value);

		/**
		* Assigns this NaviDataValue a double value
		*/
		NaviDataValue& operator=(double value);

		/**
		* Assigns this NaviDataValue a boolean value
		*/
		NaviDataValue& operator=(bool value);

		/**
		* Returns the value of this NaviDataValue as a wide string
		*/
		std::wstring wstr() const;

		/**
		* Returns the value of this NaviDataValue as a string
		*
		* @note	If the value is actually a wide string, it will be downgraded via NaviUtilities::toMultibyte
		*/
		std::string str() const;

		/**
		* Returns whether or not the value of this NaviDataValue is empty
		*/
		inline bool isEmpty() const;
		
		/**
		* Returns whether or not the value of this NaviDataValue is numeric (see NaviUtilities::isNumeric)
		*
		* @note	Boolean ("true"/"false") values are numeric.
		*/
		inline bool isNumber() const;

		/**
		* Returns the value of this NaviDataValue as an integer
		*
		* @note	If the value is unable to be cast into an integer, 0 will be returned
		*/
		int toInt() const;

		/**
		* Returns the value of this NaviDataValue as a float
		*
		* @note	If the value is unable to be cast into a float, 0 will be returned
		*/
		float toFloat() const;

		/**
		* Returns the value of this NaviDataValue as a double
		*
		* @note	If the value is unable to be cast into a double, 0 will be returned
		*/
		double toDouble() const;

		/**
		* Returns the value of this NaviDataValue as a boolean
		*
		* @note	If the value is unable to be cast into a boolean, false will be returned
		*/
		bool toBool() const;
	};

	/**
	* A map container that holds pairs of named NaviDataValue's. Used for communication
	* between the page of a Navi and the application.
	*/
	class _NaviExport NaviData
	{
		std::string name;
		std::map<std::string,NaviDataValue> data;

	public:
		/**
		* Creates a NaviData object.
		*
		* @param	name	The name of the NaviData to create.
		*
		* @param	queryString		A valid Query String with values encoded using encodeURIComponent.
		*/
		NaviData(const std::string &name, const std::string &queryString = "");

		/**
		* Validates whether or not this NaviData contains a certain key.
		*
		* @param	key		The name of the key to look for.
		* @note		To additionally validate that the value of the key is numeric, prefix the key with "#".
		*
		* @param	throwOnFailure	Whether or not to throw a Ogre::Exception::ERR_RT_ASSERTION_FAILED on failed validation.
		*
		* @return	Whether or not the key passed validation.
		*/
		bool ensure(const std::string &key, bool throwOnFailure = true) const;

		/**
		* Validates whether or not this NaviData contains a series of keys.
		*
		* @param	keys	A string vector containing the names of the keys to look for.
		* @note		To additionally validate that the value of the key is numeric, prefix the key with "#".
		* @note		It is extremely useful to use NaviUtilities::Strings with this function.
		* @note		This check can be invoked alternately via the last parameter of NaviManager::bind
		*
		* @param	throwOnFailure	Whether or not to throw a Ogre::Exception::ERR_RT_ASSERTION_FAILED on failed validation.
		*
		* @return	Whether or not all keys passed validation.
		*/
		bool ensure(const std::vector<std::string> &keys, bool throwOnFailure = true) const;

		/**
		* Returns the name of this NaviData.
		*/
		std::string getName() const;

		/**
		* Returns whether or not 'keyName' exists within the NaviData.
		*/
		bool exists(const std::string &keyName) const;

		/**
		* This subscript operator works just like the subscript operator of a map. Returns a reference to
		* a NaviDataValue object.
		*
		* @note
		*	For example:
		*	\code
		*	// Assignment:
		*	myNaviData["newKey"] = "Hello, new value.";
		*	
		*	// Value retrieval:
		*	std::string myMessage = myNaviData["newKey"].str(); // myMessage holds "Hello, new value."
		*	\endcode
		*/
		const NaviDataValue& operator[](const std::string &keyName) const;

		/**
		* This subscript operator works just like the subscript operator of a map. Returns a reference to
		* a NaviDataValue object.
		*
		* @note
		*	For example:
		*	\code
		*	// Assignment:
		*	myNaviData["newKey"] = "Hello, new value.";
		*	
		*	// Value retrieval:
		*	std::string myMessage = myNaviData["newKey"].str(); // myMessage holds "Hello, new value."
		*	\endcode
		*/
		NaviDataValue& operator[](const std::string &keyName);

		/**
		* Returns the number of data pairs in this NaviData.
		*/
		int size() const;

		/**
		* Retrieves the contents of this NaviData as a string map.
		*
		* @param	encodeVals	Whether or not to encode the values using 'encodeURIComponent' to preserve unicode characters.
		*
		* @return	A string map representing the contents of this NaviData.
		*/
		const std::map<std::string,std::string>& toStringMap(bool encodeVals) const;

		/**
		* Retrieves the contents of this NaviData as a Query String.
		*
		* @note		All values will be encoded using 'encodeURIComponent'.
		*
		* @return	A query string representing the contents of this NaviData.
		*/
		const std::string& toQueryString() const;
	};

}

#endif