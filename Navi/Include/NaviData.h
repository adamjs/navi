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

#include <string>
#include <map>

namespace NaviLibrary
{
	class Navi;

	class NaviData
	{
		friend Navi;
		std::string dataString;
		std::string dataName;
		unsigned short paramCounter;
		
		// Private constructor overload used internally
		
	public:
		/**
		* Constructs an empty NaviData object with a Name
		*
		* @param	_dataName	The Name of the NaviData object
		*/
		NaviData(const std::string &_dataName);

		/**
		* Constructs a NaviData object from a query string with a name
		*
		* @param	_dataName	The Name of the NaviData object
		*
		* @param	_dataString	The data of the NaviData object. This must be a
		*						query string (param=val&param=val) that has been
		*						encoded with encodeURIComponent.
		*/
		NaviData(const std::string &_dataName, const std::string &_dataString);

		~NaviData();

		/**
		* Adds a String parameter to a NaviData object
		*
		* @param	paramName	The name of the parameter
		*
		* @param	paramValue	The value (actual data content) of the parameter, as a String
		*/
		void add(const std::string &paramName, const std::string &paramValue);

		/**
		* Adds a Wide String parameter to a NaviData object
		*
		* @param	paramName	The name of the parameter
		*
		* @param	paramValue	The value (actual data content) of the parameter, as a Wide String
		*/
		void add(const std::string &paramName, const std::wstring &paramValue);

		/**
		* Adds an Integer parameter to a NaviData object
		*
		* @param	paramName	The name of the parameter
		*
		* @param	paramValue	The value (actual data content) of the parameter, as an Integer
		*/
		void add(const std::string &paramName, int paramValue);

		/**
		* Adds a Float parameter to a NaviData object
		*
		* @param	paramName	The name of the parameter
		*
		* @param	paramValue	The value (actual data content) of the parameter, as a Float
		*/
		void add(const std::string &paramName, float paramValue);

		/**
		* Gets the name of a NaviData object
		*/
		std::string getName() const;

		/**
		* A convenience function that checks whether or not this NaviData is
		* named a certain name
		*
		* @param	testName	The name to test against
		*
		* @param	caseSensitive	If true, compares the names literally, sensitive to upper/lower-case.
		*							If false, compares the names while ignoring case
		*/
		bool isNamed(std::string testName, bool caseSensitive = false) const;

		/**
		* Gets a Wide String parameter from a NaviData object
		*
		* @param	paramName	The name of the parameter to retrieve
		*
		* @param	paramValOut		The wide string object to insert the value of the parameter in, if the parameter is found
		*
		* @param	caseSensitive	If true, looks up the parameter name literally, sensitive to upper/lower-case.
		*							If false, looks up the parameter name while ignoring case
		*
		* @return	True if the parameter is found, False otherwise
		*/
		bool get(const std::string &paramName, std::wstring &paramValOut, bool caseSensitive = false) const;

		/**
		* Gets a String parameter from a NaviData object. If the NaviData object contains Unicode characters,
		* this will be narrowed to a multibyte string based on the current locale.
		*
		* @param	paramName	The name of the parameter to retrieve
		*
		* @param	paramValOut		The string object to insert the value of the parameter in, if the parameter is found
		*
		* @param	caseSensitive	If true, looks up the parameter name literally, sensitive to upper/lower-case.
		*							If false, looks up the parameter name while ignoring case
		*
		* @return	True if the parameter is found, False otherwise
		*/
		bool get(const std::string &paramName, std::string &paramValOut, bool caseSensitive = false) const;

		/**
		* Gets an Integer parameter from a NaviData object
		*
		* @param	paramName	The name of the parameter to retrieve
		*
		* @param	paramValOut		The integer object to insert the value of the parameter in, if the parameter is found
		*
		* @param	caseSensitive	If true, looks up the parameter name literally, sensitive to upper/lower-case.
		*							If false, looks up the parameter name while ignoring case
		*
		* @return	True if the parameter is found, False otherwise
		*
		* @throws	Ogre::Exception::ERR_INVALIDPARAMS	Throws this if it could not cast the parameter value to an Integer
		*/
		bool get(const std::string &paramName, int &paramValOut, bool caseSensitive = false) const;

		/**
		* Gets a Float parameter from a NaviData object
		*
		* @param	paramName	The name of the parameter to retrieve
		*
		* @param	paramValOut		The float object to insert the value of the parameter in, if the parameter is found
		*
		* @param	caseSensitive	If true, looks up the parameter name literally, sensitive to upper/lower-case.
		*							If false, looks up the parameter name while ignoring case
		*
		* @return	True if the parameter is found, False otherwise
		*
		* @throws	Ogre::Exception::ERR_INVALIDPARAMS	Throws this if it could not cast the parameter value to a Float
		*/
		bool get(const std::string &paramName, float &paramValOut, bool caseSensitive = false) const;

		/**
		* Fills in a std::map<std::string,std::string> with all available parameter names and values.
		* If a parameter value has Unicode characters, they will be narrowed to a multibyte string 
		* based on the current locale.
		*
		* @author	This was written by Mark Manyen for the purpose of Lua integration.
		*
		* @param	dataMapOut	The map to fill with pairs of parameter names and values. 		
		*/
		void getDataMap(std::map<std::string,std::string> &dataMapOut) const;
	};

}

#endif