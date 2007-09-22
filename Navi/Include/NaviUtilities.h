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
#include <iomanip>
#include <OgreResourceGroupManager.h>
#include <OgreLogManager.h>

namespace NaviLibrary
{
	/**
	* Various public utilities that are internally used by
	* by NaviLibrary but may be of some use to you.
	*/
	namespace NaviUtilities
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
		*
		* @note
		*	For example:
		*	\verbatim local://filename.html --> file:///C:\My Application\NaviLocal\filename.html \endverbatim
		*
		* @param	strToTranslate		The string to translate.
		*/
		void translateLocalProtocols(std::string &strToTranslate);

		/**
		* Replaces all '"resource://Group/Filename.ext"' with an equivalent Data URI of the Ogre Resource.
		* Syntax of '"resource://Filename.ext"' assumes the Resource Group is DEFAULT_RESOURCE_GROUP_NAME.
		* If the Ogre Resource is not found, the 'resource://' specifier instance is skipped.
		* Please note that this statement is required to be encased within double-quotes to be valid.
		* Certain filetypes will be additionally parsed for local/resource specifiers and translated accordingly.
		*
		* @param	strToTranslate		The string to translate.
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
		* Converts a string into its lower-case equivalent.
		*
		* @param	strToLower	The standard string to convert to lower-case.
		*
		* @return	A standard string containing the equivalent lower-case representation.
		*/
		std::string lowerString(std::string strToLower);

		/**
		* Checks whether or not a string is prefixed with a certain prefix.
		*
		* @param	sourceString	The string to check.
		*
		* @param	prefix		The prefix to search for.
		*
		* @param	ignoreCase		Whether or not to ignore differences in case, default is true.
		*
		* @return	Whether or not a match was found.
		*/
		inline bool isPrefixed(const std::string &sourceString, const std::string &prefix, bool ignoreCase = true)
		{
			return ignoreCase ? lowerString(sourceString.substr(0, prefix.length())) == lowerString(prefix) 
				: sourceString.substr(0, prefix.length()) == prefix;
		}

		/**
		* Checks whether or not a string is 'numeric' in nature (begins with actual, parseable digits).
		*
		* @param	numberString	The string to check.
		* @note		Strings beginning with 'true'/'false' (regardless of case) are numeric.
		*
		* @return	Whether or not the string is numeric (can be successfully parsed into a number).
		*/
		bool isNumeric(const std::string &numberString);

		/**
		* Converts a Number (int, float, double, bool, etc.) to a String.
		*
		* @param	number	The number (usually of type int, float, double, bool, etc.) to convert to a String.
		*
		* @return	If the conversion succeeds, returns the string equivalent of the number, otherwise an empty string.
		*/
		template<class NumberType>
		inline std::string numberToString(const NumberType &number)
		{
			std::ostringstream converter;

			return (converter << std::setprecision(17) << number).fail() ? "" : converter.str();
		}

		/**
		* Converts a String to a Number.
		*
		* @param	<NumberType>	The NumberType (int, float, bool, double, etc.) to convert to.
		*
		* @param	numberString	A string containing a valid numeric sequence (can check using isNumeric()).
		* @note		Strings beginning with 'true'/'false' (regardless of case) are numeric and will be converted accordingly.
		*
		* @return	If conversion succeeds, returns a number of type 'NumberType', otherwise returns a '0' equivalent.
		*/
		template<class NumberType>
		inline NumberType toNumber(const std::string &numberString)
		{
			if(isPrefixed(numberString, "true")) return 1;
			else if(isPrefixed(numberString, "false")) return 0;

			std::istringstream converter(numberString);
			
			if(typeid(NumberType)==typeid(bool))
			{
				int result;
				return (converter >> result).fail() ? false : !!result;
			}

			NumberType result;
			return (converter >> result).fail() ? 0 : result;
		}

		/**
		* Converts a multibyte string (standard string) to a wide string.
		*
		* @param	stringToConvert		The multibyte (standard) string to convert.
		*
		* @return	The wide-equivalent of the passed string.
		*/
		std::wstring toWide(const std::string &stringToConvert);

		/**
		* Converts a wide string to a multibyte string (standard string), based on the current locale.
		*
		* @param	wstringToConvert	The wide string to convert.
		*
		* @return	The multibyte-equivalent of the passed string, based on the current locale.
		*/
		std::string toMultibyte(const std::wstring &wstringToConvert);

		/**
		* Sets the current locale, used for 'toMultibyte()'. If you never call this, the default is usually "English". 
		*
		* @param	localeLanguage	The name of the locale language to set. An empty string sets this to the current locale of the OS.
		*/
		void setLocale(const std::string &localeLanguage = "");

		/**
		* Replaces all instances of 'replaceWhat' with 'replaceWith' inside a source string.
		*
		* @param	sourceStr	The string to do this to.
		*
		* @param	replaceWhat		What to be replaced.
		*
		* @param	replaceWith		All occurrences of 'replaceWhat' will be replaced with this.
		*
		* @return	The number of instances replaced within 'sourceStr'.
		*/
		int replaceAll(std::string &sourceStr, const std::string &replaceWhat, const std::string &replaceWith);

		/**
		* Splits a string up into a series of tokens (contained within a string vector), delimited by a certain string.
		*
		* @param	sourceStr	The string to split up.
		*
		* @param	delimiter	What to delimit the source string by.
		*
		* @param	ignoreEmpty		Whether or not to ignore empty tokens. (usually created by 2 or more immediately adjacent delimiters)
		*
		* @return	A string vector containing a series of ordered tokens.
		*/
		const std::vector<std::string>& split(const std::string &sourceStr, const std::string &delimiter, bool ignoreEmpty = true);

		/**
		* A more advanced form of splitting, parses a string into a string map. Exceptionally useful for use with Query Strings.
		*
		* @param	sourceStr	The string to parse.
		*
		* @param	pairDelimiter	What to delimit pairs by.
		*
		* @param	keyValueDelimiter	What to delimit key-values by.
		*
		* @param	ignoreEmpty		Whether or not to ignore empty values. Empty pairs will always be ignored.
		*
		* @return	A string map containing the parsed equivalent of the passed string.
		*
		* @note
		*	For example:
		*	\code
		*	std::string myQueryString = "name=Bob&sex=none&color=purple";
		*	std::map<std::string,std::string> myMap = splitToMap(myQueryString, "&", "=");
		*	std::string myColor = myMap["color"]; // myColor is now 'purple' 
		*	\endcode
		*/
		const std::map<std::string,std::string>& splitToMap(const std::string &sourceStr, const std::string &pairDelimiter, const std::string &keyValueDelimiter, bool ignoreEmpty = true);

		/**
		* Joins a string vector into a single string. (Effectively does the inverse of NaviUtilities::split)
		*
		* @param	sourceVector	The string vector to join.
		*
		* @param	delimiter	What to delimit each token by.
		*
		* @param	ignoreEmpty		Whether or not to ignore empty strings within the string vector.
		*/
		std::string join(const std::vector<std::string> &sourceVector, const std::string &delimiter, bool ignoreEmpty = true);

		/**
		* Joins a string map into a single string. (Effectively does the inverse of NaviUtilities::splitToMap)
		*
		* @param	sourceMap	The string map to join.
		*
		* @param	pairDelimiter	What to delimit each pair by.
		*
		* @param	keyValueDelimiter	What to delimit each key-value by.
		*
		* @param	ignoreEmpty		Whether or not to ignore empty string values within the string map.
		*/
		std::string joinFromMap(const std::map<std::string,std::string> &sourceMap, const std::string &pairDelimiter, const std::string &keyValueDelimiter, bool ignoreEmpty = true);

		/**
		* This is an incredibly useful utility class for creating small inline vectors quickly.
		*
		* @note
		*	For example:
		*	\code
		*	// Let's say you have some function that accepts a vector of integers, "int addNumbers(std::vector<int> integers);"
		*	// Why waste code space doing a bunch of myVector.push_back(3), myVector.push_back(5), etc. when you can do this:
		*
		*	addNumbers(InlineVector<int>(3)(5)(10)); // returns 18
		*	\endcode
		*
		* @note	Adapted from http://erdani.org/publications/inline_containers.html
		*/
		template <class T>
		class InlineVector : public std::vector<T>
		{
		public:
			InlineVector() { }

			InlineVector(InlineVector &v) { this->swap(v); }

			explicit InlineVector(const T &firstArg) : std::vector<T>(1, firstArg) { }

			InlineVector& operator()(const T &newArg) 
			{
				this->push_back(newArg);
				return *this;
			}
		};

		/**
		* This is just a simple way to quickly make inline string vectors.
		*
		* @note
		*	For example:
		*	\code
		*	// Before:
		*	vector<string> myVector;
		*	myVector.push_back("hello");
		*	myVector.push_back("awesome");
		*	myVector.push_back("world");
		*	someFunction(myVector);
		*
		*	// After:
		*	someFunction(Strings("hello")("awesome")("world"));
		*	\endcode
		*/
		typedef InlineVector<std::string> Strings;

		/**
		* Converts a Hex Color String to R, G, B values.
		*
		* @param	hexString	A hex color string in format: "#XXXXXX"
		*
		* @param	R	The Red value to change.
		* @param	G	The Green value to change.
		* @param	B	The Blue value to change.
		*
		* @return	Returns whether or not the conversion was successful.
		*/
		bool hexStringToRGB(const std::string& hexString, unsigned char &R, unsigned char &G, unsigned char &B);

		/** 
		* Encodes a string into Base64.
		*
		* @param	strToEncode		The string to encode.
		*
		* @return	The Base64-encoded representation of the passed string.
		*/
		std::string encodeBase64(const std::string &strToEncode);
	}
}

#endif