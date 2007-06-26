#include "NaviDemo.h"
#include "NaviUtilities.h"

using namespace Ogre;
using namespace NaviLibrary;

NaviManager& naviMgr = NaviManager::Get();

NaviDemo::NaviDemo()
{
	shouldQuit = false;
	renderWin = 0;
	sceneMgr = 0;
#ifdef _DEBUG
	dbgOverlay = 0;
	dbgText = 0;
#endif
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
	sceneMgr->setFog( FOG_LINEAR, ColourValue(0.9, 0.9, 1), 0.0, 0, 1550 );

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

#ifdef _DEBUG
	// Set-up the Frame-Rate overlay
	dbgOverlay = OverlayManager::getSingleton().getByName("Core/DebugOverlay");
	if(dbgOverlay) { dbgOverlay->show(); dbgOverlay->getChild("Core/LogoPanel")->hide(); }
	else throw std::string("error getting DebugOverlay!");

	dbgText = OverlayManager::getSingleton().getOverlayElement("Core/DebugText");
	dbgText->setCaption("To move around, use keys: W A S D Q E PageUp PageDown");
#endif

	// Startup, create, and manage Navis
	naviMgr.Startup(renderWin);

	naviMgr.createNavi("menubar", "local://menubar.html", NaviPosition(BottomCenter), 1024, 128, false);
	naviMgr.setNaviMask("menubar", "navimenu_bg.png");
	naviMgr.bindNaviData("menubar", "turnOn", NaviDelegate(this, &NaviDemo::turnOn));
	naviMgr.bindNaviData("menubar", "turnOff", NaviDelegate(this, &NaviDemo::turnOff));
	naviMgr.bindNaviData("menubar", "hpChange", NaviDelegate(this, &NaviDemo::hpChange));

	naviMgr.createNavi("status", "local://status.html", NaviPosition(0, 0), 512, 256, true, false);
	naviMgr.setNaviMask("status", "status_bg.png");

	naviMgr.createNavi("chat", "http://navi.agelessanime.com/chat", NaviPosition(BottomLeft, 40, -150), 512, 256, true, false);
	naviMgr.setNaviMask("chat", "navichat_bg.png");
	naviMgr.setNaviColorKey("chat", "#19001a", 0.7);
	naviMgr.bindNaviData("chat", "messageSent", NaviDelegate(this, &NaviDemo::messageSent));

	naviMgr.createNavi("equip", "local://equip.html", NaviPosition(Right), 256, 512, true, false);
	naviMgr.setNaviMask("equip", "naviequip_bg.png");
	naviMgr.bindNaviData("equip", "itemEquipped", NaviDelegate(this, &NaviDemo::itemEquipped));

	// Startup NaviMouse and create the cursors
	NaviMouse* mouse = naviMgr.StartupMouse();
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

void NaviDemo::turnOn(const NaviData &naviData)
{
	std::string naviName;
	if(naviData.get("name", naviName))
	{
		if(naviName == "quit")
			shouldQuit = true;
		else
		{
			naviMgr.resetNaviPosition(naviName);
			naviMgr.showNavi(naviName, true);
		}
	}
}

void NaviDemo::turnOff(const NaviData &naviData)
{
	std::string naviName;
	if(naviData.get("name", naviName))
		naviMgr.hideNavi(naviName, true);
}

void NaviDemo::hpChange(const NaviData &naviData)
{
	static short curHP = 100;

	std::string direction;
	if(naviData.get("direction", direction))
	{
		if(direction == "up")
		{
			curHP += (rand() % 30) + 1;
			if(curHP > 100) curHP = 100;
			std::stringstream setHP;
			setHP << "setHP(" << curHP << ")";
			naviMgr.naviEvaluateJS("status", setHP.str());
		}
		else
		{
			curHP -= (rand() % 30) + 1;
			if(curHP < 0) curHP = 0;
			std::stringstream setHP;
			setHP << "setHP(" << curHP << ")";
			naviMgr.naviEvaluateJS("status", setHP.str());
		}
	}
}

void NaviDemo::messageSent(const NaviData &naviData)
{
	std::string nick, message;
	if(naviData.get("nick", nick))
		naviMgr.naviEvaluateJS("status", "$('playerName').innerHTML = '" + nick + "'");
	if(naviData.get("message", message))
		LogManager::getSingleton().logMessage("Navi Chat: " + message);
}

void NaviDemo::itemEquipped(const NaviData &naviData)
{
	std::string itemName;
	if(naviData.get("name", itemName))
		LogManager::getSingleton().logMessage("Navi Equip: Item '" + itemName + "' equipped!");
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
#ifdef _DEBUG
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
#endif
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
#ifdef _DEBUG
		dbgText->setCaption("Click in a blank area to de-focus all Navis before moving around.");
#endif
		return true;
	}
	else
	{
#ifdef _DEBUG
		dbgText->setCaption("To move around, use keys: W A S D Q E PageUp PageDown");
#endif
	}

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
	else if(arg.key == OIS::KC_F11)
		NaviManager::Get().resetAllPositions();

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
