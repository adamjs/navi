#include "NaviDemo.h"
#include "NaviUtilities.h"

using namespace Ogre;
using namespace NaviLibrary;

NaviDemo::NaviDemo()
{
	shouldQuit = false;
	renderWin = 0;
	sceneMgr = 0;
	dbgOverlay = 0;
	dbgText = 0;
	finalMaskSet = false;
	forward = left = right = back = up = down = turnL = turnR = false;
	lastTime = timer.getMilliseconds();
}

NaviDemo::~NaviDemo()
{

}

void NaviDemo::Startup()
{
	Root *root = new Root("plugins.cfg", "ogre.cfg", "ogre.log");
	if (!root->restoreConfig())
	{
		shouldQuit = !root->showConfigDialog();
		if(shouldQuit) return;
	}

	root->saveConfig();
	root->initialise(true, "NaviDemo");
	renderWin = root->getAutoCreatedWindow();

	sceneMgr = root->createSceneManager("TerrainSceneManager", "NaviDemoSceneMgr");
	sceneMgr->setAmbientLight(ColourValue(1, 1, 1));
	sceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_ADDITIVE);

	Camera* camera = sceneMgr->createCamera("MainCam");
	camera->setPosition(0,10,-15);
	camera->lookAt(0, 0, 30);
	Viewport* viewport = renderWin->addViewport(camera);
	viewport->setBackgroundColour(ColourValue(0.5, 0.5, 0.5));
	camera->setAspectRatio((float)viewport->getActualWidth() / (float) viewport->getActualHeight());
	camera->setFarClipDistance( 1000 );
	camera->setNearClipDistance( 1 );
	camNode = sceneMgr->getRootSceneNode()->createChildSceneNode("camNode");
	camNode->attachObject(camera);

	if (root->getRenderSystem()->getCapabilities()->hasCapability(RSC_INFINITE_FAR_PLANE))
		camera->setFarClipDistance(0);

	Light* light = sceneMgr->createLight( "Sun" );
	light->setType( Light::LT_DIRECTIONAL );
	light->setDiffuseColour( ColourValue( .82, .81, .7 ) );
	light->setSpecularColour( ColourValue( .82, .81, .7 ) );
	light->setDirection( Vector3( 0, -1, 1 ) ); 
    	
	parseResources();

	sceneMgr->setSkyDome( true, "CloudySky", 5, 6, 8 );

	// Set-up the Frame-Rate overlay
	dbgOverlay = OverlayManager::getSingleton().getByName("Core/DebugOverlay");
	if(dbgOverlay) { dbgOverlay->show(); dbgOverlay->getChild("Core/LogoPanel")->hide(); }
	else throw std::string("error getting DebugOverlay!");

	dbgText = OverlayManager::getSingleton().getOverlayElement("Core/DebugText");
	dbgText->setCaption("To move around, use keys: W A S D Q E PageUp PageDown");

	// Startup, create, and manage Navis
	NaviManager::Get().Startup(renderWin);

	NaviManager::Get().createNavi("welcomeNavi", "local://welcome.html", BottomCenter, 1024, 128, true, 35);
	NaviManager::Get().setNaviMask("welcomeNavi", "welcome.png");
	NaviManager::Get().addNaviEventListener("welcomeNavi", this);

	NaviManager::Get().createNavi("testNavi", "local://naviLogo.html", 25, 15, 512, 512, true, false, 35);
	NaviManager::Get().addNaviEventListener("testNavi", this);
	NaviManager::Get().setNaviMask("testNavi", "naviLogo.png");
	NaviManager::Get().showNavi("testNavi", true, 5500);

	NaviManager::Get().createNavi("controlsNavi", "", TopRight, 256, 256, true, 35);
	NaviManager::Get().addNaviEventListener("controlsNavi", this);
	NaviManager::Get().setNaviMask("controlsNavi", "controlsNaviMask.png");
	NaviManager::Get().setNaviColorKey("controlsNavi", "#200020", 0.6);
	NaviManager::Get().bindNaviData("controlsNavi", "urlChange", NaviDelegate(this, &NaviDemo::changeURL));

	NaviData testData("DemoNaviData");
	testData.add("msg", "Welcome to Navi!");
	NaviManager::Get().navigateNaviTo("controlsNavi", "local://controls.html", testData);

	// Startup NaviMouse and create the cursors
	NaviMouse* mouse = NaviManager::Get().StartupMouse();
	NaviCursor* defaultCursor = mouse->createCursor("fadingCursor", 3, 2);
	defaultCursor->addFrame(1200, "cursor1.png")->addFrame(100, "cursor2.png")->addFrame(100, "cursor3.png")->addFrame(100, "cursor4.png");
	defaultCursor->addFrame(100, "cursor5.png")->addFrame(100, "cursor6.png")->addFrame(100, "cursor5.png")->addFrame(100, "cursor4.png");
	defaultCursor->addFrame(100, "cursor3.png")->addFrame(100, "cursor2.png");
	mouse->setDefaultCursor("fadingCursor");

	// If you define a "move" cursor, NaviMouse will automatically use that when you move a Navi
	NaviCursor* moveCursor = mouse->createCursor("move", 19, 19);
	moveCursor->addFrame(0, "cursorMove.png");

	// Create the ground
	Plane plane( Vector3::UNIT_Y, 0);
	MeshManager::getSingleton().createPlane("ground",
		ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, plane,
		1500,1500,1,1,true,1,9,9,Vector3::UNIT_Z);
	Entity* ent = sceneMgr->createEntity( "GroundEntity", "ground" );
	sceneMgr->getRootSceneNode()->createChildSceneNode("GroundNode")->attachObject(ent);
	ent->setMaterialName("Terrain");
	ent->setCastShadows(false);

	Plane plane2( Vector3::NEGATIVE_UNIT_Z, -60 );
	MeshManager::getSingleton().createPlane("demoPlane",
		ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, plane2,
		35,35,1,1,true,1,1,1,Vector3::UNIT_Y);

	// Creates the Video Plane and subsequent NaviMaterial
	Entity* vidEnt = sceneMgr->createEntity( "VideoEntity", "demoPlane" );
	vidEnt->setMaterialName(NaviManager::Get().createNaviMaterial("videoNavi", "local://video.html", 512, 512, true, 18, true, 0.95));
	NaviManager::Get().setNaviColorKey("videoNavi", "#d500d5");
	SceneNode *videoNode = sceneMgr->getRootSceneNode()->createChildSceneNode("VideoNode", Vector3(10, 3, 0));
	videoNode->attachObject(vidEnt);
	videoNode->yaw(Ogre::Degree(10), Ogre::Node::TS_WORLD);

	// Creates the Text Plane and subsequent NaviMaterial
	Entity* txtEnt = sceneMgr->createEntity( "TextEntity", "demoPlane" );
	txtEnt->setMaterialName(NaviManager::Get().createNaviMaterial("textNavi", "local://text.html", 512, 512));
	NaviManager::Get().setNaviColorKey("textNavi", "#383338", 0, "#000000", 0.2);
	SceneNode *txtNode = sceneMgr->getRootSceneNode()->createChildSceneNode(Vector3(-15, 0, 15));
	txtNode->attachObject(txtEnt);
	txtNode->yaw(Ogre::Degree(-10), Ogre::Node::TS_WORLD);

	// Create the Blue Knot
	ent = sceneMgr->createEntity( "BlueMetalKnot", "knot.mesh" );
	ent->setMaterialName("Knot");
	ent->setCastShadows( true );

	// Position and scale the Blue Knot
	SceneNode* node = sceneMgr->getRootSceneNode()->createChildSceneNode("KnotNode", Vector3(0, 12, 45));
	node->attachObject( ent );
	node->scale(.07, .07, .07);

	loadInputSystem();

	Ogre::WindowEventUtilities::addWindowEventListener(renderWin, this);
}

void NaviDemo::Update()
{
	NaviManager::Get().Update();
	Root::getSingleton().renderOneFrame();
	updateStats();
	Ogre::WindowEventUtilities::messagePump();
	InputManager::getSingletonPtr()->capture();

	static Vector3 transVector;
	transVector = Vector3::ZERO;
	if(forward) transVector.z += 5;
	if(back) transVector.z -= 5;
	if(left) transVector.x += 5;
	if(right) transVector.x -= 5;
	if(up) transVector.y += 5;
	if(down) transVector.y -=5;

	camNode->translate(camNode->getOrientation() * transVector * ((float)(timer.getMilliseconds()-lastTime)/1000));
	if(turnL) camNode->yaw(Degree(6 * ((float)(timer.getMilliseconds()-lastTime)/1000)));
	if(turnR) camNode->yaw(Degree(-6 * ((float)(timer.getMilliseconds()-lastTime)/1000)));
	lastTime = timer.getMilliseconds();
}

void NaviDemo::Shutdown()
{
	NaviManager::Get().Shutdown();
	Root::getSingleton().shutdown();
}

void NaviDemo::changeURL(const NaviData& naviData)
{
	std::string urlStr;
	if(naviData.get("url", urlStr))
		dbgText->setCaption(urlStr);

	NaviManager::Get().navigateNaviTo("testNavi", urlStr);
	
	if(!finalMaskSet)
	{
		NaviManager::Get().setNaviMask("testNavi", "testNaviMask.png");
		finalMaskSet = true;
	}
}

void NaviDemo::parseResources()
{
    ConfigFile cf;
	try 
	{
		cf.load("resources.cfg");
	} 
	catch( Exception& e ) 
	{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 
		MessageBox( NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
		fprintf(stderr, "An exception has occured: %s\n",
            e.getFullDescription().c_str());
#endif
		return;
	}

    // Go through all sections & settings in the file
    ConfigFile::SectionIterator seci = cf.getSectionIterator();

    String secName, typeName, archName;
    while (seci.hasMoreElements())
    {
        secName = seci.peekNextKey();
        ConfigFile::SettingsMultiMap *settings = seci.getNext();
        ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i)
        {
            typeName = i->first;
            archName = i->second;
            ResourceGroupManager::getSingleton().addResourceLocation(
                archName, typeName, secName);
        }
    }

	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}

void NaviDemo::loadInputSystem()
{
	inputMgr = InputManager::getSingletonPtr();
    inputMgr->initialise(renderWin);
    inputMgr->addMouseListener(this, "NaviDemoMouseListener");
	inputMgr->addKeyListener(this, "NaviDemoKeyListener");
}

void NaviDemo::updateStats()
{
	static String currFps = "Current FPS: ";
	static String avgFps = "Average FPS: ";
	static String bestFps = "Best FPS: ";
	static String worstFps = "Worst FPS: ";
	static String tris = "Triangle Count: ";

	// update stats when necessary
	try {
		OverlayElement* guiAvg = OverlayManager::getSingleton().getOverlayElement("Core/AverageFps");
		OverlayElement* guiCurr = OverlayManager::getSingleton().getOverlayElement("Core/CurrFps");
		OverlayElement* guiBest = OverlayManager::getSingleton().getOverlayElement("Core/BestFps");
		OverlayElement* guiWorst = OverlayManager::getSingleton().getOverlayElement("Core/WorstFps");

		const RenderTarget::FrameStats& stats = renderWin->getStatistics();
		guiAvg->setCaption(avgFps + StringConverter::toString(stats.avgFPS));
		guiCurr->setCaption(currFps + StringConverter::toString(stats.lastFPS));
		guiBest->setCaption(bestFps + StringConverter::toString(stats.bestFPS)
			+" "+StringConverter::toString(stats.bestFrameTime)+" ms");
		guiWorst->setCaption(worstFps + StringConverter::toString(stats.worstFPS)
			+" "+StringConverter::toString(stats.worstFrameTime)+" ms");

		OverlayElement* guiTris = OverlayManager::getSingleton().getOverlayElement("Core/NumTris");
		guiTris->setCaption(tris + StringConverter::toString(stats.triangleCount));
	}
	catch(...) { /* ignore */ }
}

bool NaviDemo::mouseMoved(const OIS::MouseEvent &arg)
{
	if(arg.state.Z.rel != 0) NaviManager::Get().injectMouseWheel(arg.state.Z.rel);

	return NaviManager::Get().injectMouseMove(arg.state.X.abs, arg.state.Y.abs);
}

bool NaviDemo::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	return NaviManager::Get().injectMouseDown(id);
}

bool NaviDemo::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	return NaviManager::Get().injectMouseUp(id);
}

bool NaviDemo::keyPressed( const OIS::KeyEvent &arg )
{
	if(NaviManager::Get().isAnyNaviFocused())
	{
		dbgText->setCaption("Click in a blank area to de-focus all Navis before moving around.");
		return true;
	}
	else
		dbgText->setCaption("To move around, use keys: W A S D Q E PageUp PageDown");

	switch(arg.key)
	{
	case OIS::KC_W:
		forward = true;
		break;
	case OIS::KC_A:
		left = true;
		break;
	case OIS::KC_D:
		right = true;
		break;
	case OIS::KC_S:
		back = true;
		break;
	case OIS::KC_PGUP:
		up = true;
		break;
	case OIS::KC_PGDOWN:
		down = true;
		break;
	case OIS::KC_Q:
		turnL = true;
		break;
	case OIS::KC_E:
		turnR = true;
		break;
	}

	return true;
}

bool NaviDemo::keyReleased( const OIS::KeyEvent &arg )
{
	if(arg.key == OIS::KC_ESCAPE)
		shouldQuit = true;

	switch(arg.key)
	{
	case OIS::KC_W:
		forward = false;
		break;
	case OIS::KC_A:
		left = false;
		break;
	case OIS::KC_D:
		right = false;
		break;
	case OIS::KC_S:
		back = false;
		break;
	case OIS::KC_PGUP:
		up = false;
		break;
	case OIS::KC_PGDOWN:
		down = false;
		break;
	case OIS::KC_Q:
		turnL = false;
		break;
	case OIS::KC_E:
		turnR = false;
		break;
	}

	return true;
}

void NaviDemo::windowMoved(RenderWindow* rw) {}

void NaviDemo::windowResized(RenderWindow* rw) 
{
	// Gotta tell the InputManager about the proper size of the window
	inputMgr->setWindowExtents(rw->getWidth(), rw->getHeight());
}

void NaviDemo::windowClosed(RenderWindow* rw) 
{
	shouldQuit = true;
}

void NaviDemo::windowFocusChange(RenderWindow* rw) {}


void NaviDemo::onNaviDataEvent(const std::string &naviName, const NaviData &naviData)
{
	if(naviData.isNamed("opacityChange"))
	{
		float opacity;
		try {
			if(naviData.get("opacity", opacity))
				NaviManager::Get().setNaviOpacity("testNavi", opacity);
		} catch(...) {}

	}
	else if(naviData.isNamed("updateTypeChange"))
	{
		std::string updateTypeStr;
		if(naviData.get("updateType", updateTypeStr))
		{
			if(updateTypeStr == "auto")
				NaviManager::Get().setForceMaxUpdate("testNavi", false);
			else
				NaviManager::Get().setForceMaxUpdate("testNavi", true);
		}
	}
	else if(naviData.isNamed("maxUPSChange"))
	{
		int maxUPS;
		try {
			if(naviData.get("maxUPS", maxUPS))
				NaviManager::Get().setMaxUpdatesPerSec("testNavi", maxUPS);
		} catch(...) {}
	}
	else if(naviData.isNamed("ambientChange"))
	{
		int ambientPercent;
		try 
		{
			if(naviData.get("ambient", ambientPercent))
			{	
				if(ambientPercent >= 0 && ambientPercent <= 100)
				{
					float ambientDecimal = (float)ambientPercent / 100;

					sceneMgr->setAmbientLight(ColourValue(ambientDecimal, ambientDecimal, ambientDecimal));
				}
			}				
		} catch(...) {}
	}
	else if(naviData.isNamed("toggleWorldGeometry"))
	{
		sceneMgr->getRootSceneNode()->flipVisibility(true);
	}
	else if(naviData.isNamed("gotoNaviTest"))
	{
		NaviManager::Get().navigateNaviTo("testNavi", "www.google.com");

		if(!finalMaskSet)
		{
			NaviManager::Get().setNaviMask("testNavi", "testNaviMask.png");
			finalMaskSet = true;
		}
	}
	else if(naviData.isNamed("exit"))
	{
		shouldQuit = true;
	}
}

void NaviDemo::onNaviLinkClicked(const std::string &naviName, const std::string &linkHref)
{
	
}