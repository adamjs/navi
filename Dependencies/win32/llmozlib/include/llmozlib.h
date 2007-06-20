/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Linden Lab Inc. (http://lindenlab.com) code.
 *
 * The Initial Developer of the Original Code is:
 *   Callum Prentice (callum@ubrowser.com)
 *
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Callum Prentice (callum@ubrowser.com)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef LLMOZLIB_H
#define LLMOZLIB_H

#include <string>
#include <map>

class LLEmbeddedBrowser;
class LLEmbeddedBrowserWindow;

////////////////////////////////////////////////////////////////////////////////
// data class that is passed with an event
class LLEmbeddedBrowserWindowEvent
{
	public:
		LLEmbeddedBrowserWindowEvent( int eventWindowIdIn, std::string uriIn ) :
			mEventWindowId( eventWindowIdIn ),
			mEventUri( uriIn )
		{
		};

		// single int passed with the event - e.g. progress
		LLEmbeddedBrowserWindowEvent( int eventWindowIdIn, std::string uriIn, int intValIn ) :
			mEventWindowId( eventWindowIdIn ),
			mEventUri( uriIn ),
			mIntVal( intValIn )
		{
		};

		// string passed with the event
		LLEmbeddedBrowserWindowEvent( int eventWindowIdIn, std::string uriIn, std::string stringValIn ) :
			mEventWindowId( eventWindowIdIn ),
			mEventUri( uriIn ),
			mStringVal( stringValIn )
		{
		};

		// string and an int passed with the event
		LLEmbeddedBrowserWindowEvent( int eventWindowIdIn, std::string uriIn, std::string stringValIn, int intValIn ) :
			mEventWindowId( eventWindowIdIn ),
			mEventUri( uriIn ),
			mStringVal( stringValIn ),
			mIntVal( intValIn )
		{
		};

		// 4 ints passed (semantically as a rectangle but could be anything - didn't want to make a RECT type structure)
		LLEmbeddedBrowserWindowEvent( int eventWindowIdIn, std::string uriIn, int xIn, int yIn, int widthIn, int heightIn ) :
			mEventWindowId( eventWindowIdIn ),
			mEventUri( uriIn ),
			mXVal( xIn ),
			mYVal( yIn ),
			mWidthVal( widthIn ),
			mHeightVal( heightIn )
		{
		};

		virtual ~LLEmbeddedBrowserWindowEvent()
		{
		};

		int getEventWindowId() const
		{
			return mEventWindowId;
		};

		std::string getEventUri() const
		{
			return mEventUri;
		};

		int getIntValue() const
		{
			return mIntVal;
		};

		std::string getStringValue() const
		{
			return mStringVal;
		};

		void getRectValue( int& xOut, int& yOut, int& widthOut, int& heightOut ) const
		{
			xOut = mXVal;
			yOut = mYVal;
			widthOut = mWidthVal;
			heightOut = mHeightVal;
		};

	private:
		int mEventWindowId;
		std::string mEventUri;
		int mIntVal;
		std::string mStringVal;
		int mXVal;
		int mYVal;
		int mWidthVal;
		int mHeightVal;
};

////////////////////////////////////////////////////////////////////////////////
// derrive from this class and override these methods to observe these events
class LLEmbeddedBrowserWindowObserver
{
	public:
		virtual ~LLEmbeddedBrowserWindowObserver() { };
		typedef LLEmbeddedBrowserWindowEvent EventType;
		
		virtual void onPageChanged( const EventType& eventIn ) { };
		virtual void onNavigateBegin( const EventType& eventIn ) { };
		virtual void onNavigateComplete( const EventType& eventIn ) { };
		virtual void onUpdateProgress( const EventType& eventIn ) { };
		virtual void onStatusTextChange( const EventType& eventIn ) { };
		virtual void onLocationChange( const EventType& eventIn ) { };
		virtual void onClickLinkHref( const EventType& eventIn ) { };
};

////////////////////////////////////////////////////////////////////////////////
// main library class
class LLMozLib
{
	public:
		virtual ~LLMozLib();

		// singleton access
		static LLMozLib* getInstance();											

		// housekeeping
		bool init( std::string appBaseDirIn, std::string profileDirNameIn );	
		bool reset();															
		bool clearCache();
		int getLastError();														
		const std::string getVersion();											
		void setBrowserAgentId( std::string idIn );								

		// browser window - creation/deletion, mutation etc.
		int createBrowserWindow( void* nativeWindowHandleIn, int browserWindowWidthIn, int browserWindowHeightIn );
		bool destroyBrowserWindow( int browserWindowIdIn );
		bool setSize( int browserWindowIdIn, int widthIn, int heightIn );
		bool scrollByLines( int browserWindowIdIn, int linesIn );
		bool setBackgroundColor( int browserWindowIdIn, const int redIn, const int greenIn, const int blueIn );
		bool setEnabled( int browserWindowIdIn, bool enabledIn );

		// add/remove yourself as an observer on browser events - see LLEmbeddedBrowserWindowObserver declaration
		bool addObserver( int browserWindowIdIn, LLEmbeddedBrowserWindowObserver* subjectIn );
		bool remObserver( int browserWindowIdIn, LLEmbeddedBrowserWindowObserver* subjectIn );

		// navigation - self explanatory
		bool navigateTo( int browserWindowIdIn, const std::string uriIn );
		bool navigateStop( int browserWindowIdIn );
		bool canNavigateBack( int browserWindowIdIn );
		bool navigateBack( int browserWindowIdIn );
		bool canNavigateForward( int browserWindowIdIn );
		bool navigateForward( int browserWindowIdIn );

		// javascript evaluation/execution
		std::string evaluateJavascript( int browserWindowIdIn, const std::string scriptIn );

		// access to rendered bitmap data
		const unsigned char* grabBrowserWindow( int browserWindowIdIn );		// renders page to memory and returns pixels
		const unsigned char* getBrowserWindowPixels( int browserWindowIdIn );	// just returns pixels - no render
		const int getBrowserWidth( int browserWindowIdIn );						// current browser width (can vary slightly after page is rendered)
		const int getBrowserHeight( int browserWindowIdIn );					// current height
		const int getBrowserDepth( int browserWindowIdIn );						// depth in bytes
		const int getBrowserRowSpan( int browserWindowIdIn );					// width in pixels * depth in bytes

		// mouse/keyboard interaction
		bool mouseDown( int browserWindowIdIn, int xPosIn, int yPosIn );		// send a mouse down event to a browser window at given XY in browser space
		bool mouseUp( int browserWindowIdIn, int xPosIn, int yPosIn );			// send a mouse up event to a browser window at given XY in browser space
		bool mouseMove( int browserWindowIdIn, int xPosIn, int yPosIn );		// send a mouse move event to a browser window at given XY in browser space
		bool keyPress( int browserWindowIdIn, int keyCodeIn );					// send a key press event to a browser window 
		bool focusBrowser( int browserWindowIdIn, bool focusBrowserIn );		// set/remove focus to given browser window

		// accessor/mutator for scheme that browser doesn't follow - e.g. secondlife.com://
		void setNoFollowScheme( int browserWindowIdIn, std::string schemeIn );
		std::string getNoFollowScheme( int browserWindowIdIn );

	private:
		LLMozLib();
		LLEmbeddedBrowserWindow* getBrowserWindowFromWindowId( int browserWindowIdIn );
		static LLMozLib* sInstance;
		const int mMaxBrowserWindows;
		typedef std::map< int, LLEmbeddedBrowserWindow* > BrowserWindowMap;
		typedef std::map< int, LLEmbeddedBrowserWindow* >::iterator BrowserWindowMapIter;
		BrowserWindowMap mBrowserWindowMap;
};

#endif // LLMOZLIB_H