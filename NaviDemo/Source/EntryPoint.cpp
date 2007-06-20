#include "NaviDemo.h"
#ifdef WIN32
#include <windows.h>
#endif

int main()
{
	NaviDemo demo;

	try
	{
		demo.Startup();

		while(!demo.shouldQuit)
		{
			demo.Update();
		}
	}
	catch( Ogre::Exception& e )
	{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		ShowCursor(true);
		MessageBox( NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
		std::cerr << "An exception has occured: " << e.getFullDescription();
#endif
	}


	demo.Shutdown();

	return 0;
}