#include "NaviDemo.h"
#include "NaviUtilities.h"

using namespace Ogre;
using namespace NaviLibrary;
using namespace NaviLibrary::NaviUtilities;


NaviDemo::NaviDemo()
{
	menubar, status, chat, equip = 0;
	shouldQuit = false;
	renderWin = 0;
	sceneMgr = 0;

	Root *root = new Root();

	shouldQuit = !root->showConfigDialog();
	if(shouldQuit) return;

	renderWin = root->initialise(true, "NaviDemo");
	sceneMgr = root->createSceneManager("TerrainSceneManager");
	WindowEventUtilities::addWindowEventListener(renderWin, this);

	createScene();

	setupNavis();

	loadInputSystem();
}

NaviDemo::~NaviDemo()
{
	delete NaviManager::GetPointer();
	Root::getSingleton().shutdown();
}

void NaviDemo::createScene()
{
	sceneMgr->setAmbientLight(ColourValue::White);
	sceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_ADDITIVE);

	Camera* camera = sceneMgr->createCamera("MainCam");
	Viewport* viewport = renderWin->addViewport(camera);
	viewport->setBackgroundColour(ColourValue(0.17, 0.14, 0.1));

	camera->setAspectRatio(viewport->getActualWidth() / (Real)viewport->getActualHeight());

	parseResources();
}

void NaviDemo::setupNavis()
{
	// Create the NaviManager make our Navis
	naviMgr = new NaviManager(renderWin, "..\\Media");

	menubar = naviMgr->createNavi("menubar", NaviPosition(BottomCenter), 1024, 128);
	menubar->loadFile("menubar.html");
	menubar->setCallback("hpChange", NaviDelegate(this, &NaviDemo::hpChange));
	menubar->setCallback("turnOff", NaviDelegate(this, &NaviDemo::turnOff));
	menubar->setCallback("turnOn", NaviDelegate(this, &NaviDemo::turnOn));
	menubar->setMask("navimenu_bg.png");
	menubar->setMovable(false);

	status = naviMgr->createNavi("status", NaviPosition(0, 0), 512, 256);
	status->loadFile("status.html");
	status->setMask("status_bg.png");
	status->hide();

	chat = naviMgr->createNavi("chat", NaviPosition(BottomLeft, 40, -150), 512, 256);
	chat->loadURL("http://navi.agelessanime.com/chat/index.html");
	chat->setMask("navichat_bg.png");
	chat->hide();

	equip = naviMgr->createNavi("equip", NaviPosition(Right), 256, 512);
	equip->loadFile("equip.html");
	equip->setMask("naviequip_bg.png");
	equip->setCallback("itemEquipped", NaviDelegate(this, &NaviDemo::itemEquipped));
	equip->hide();
}

void NaviDemo::turnOn(const JSArguments& args)
{
	std::string naviName = args.at(0).toString();
	Navi* navi = naviMgr->getNavi(naviName);

	if(naviName == "quit")
	{
		shouldQuit = true;
	}
	else if(navi)
	{
		navi->resetPosition();
		navi->show(true);
		navi->focus();
	}
}

void NaviDemo::turnOff(const JSArguments& args)
{
	Navi* navi = naviMgr->getNavi(args.at(0).toString());

	if(navi)
		navi->hide(true);
}

void NaviDemo::hpChange(const JSArguments& args)
{
	static short curHP = 100;

	if(args.at(0).toString() == "up")
		curHP += (rand() % 30) + 1;
	else
		curHP -= (rand() % 30) + 1;

	limit<short>(curHP, 0, 100);

	status->setProperty("HP", curHP);
	status->evaluateJS("setHP(Client.HP)");

	logTemplate("Navi Menubar: Current HP is now ?", Args(curHP));
}

void NaviDemo::messageSent(const JSArguments& args)
{
	// Set the HTML of the element 'playerName' to our chat nickname
	status->setProperty("nickname", args.at(0).toString());
	status->evaluateJS("$('playerName').setHTML(Client.nickname)");

	logTemplate("Navi Chat: ?", Args(args.at(0).toString()));
}

void NaviDemo::itemEquipped(const JSArguments& args)
{
	logTemplate("Navi Equip: Item '?' equipped!", Args(args.at(0).toString()));
}

void NaviDemo::levelChanged(const JSArguments& args)
{
	int level = args.at(0).toInteger();

	logTemplate("Navi Status: Level has been changed to: ?, double that is: ?", Args(level)(level*2));
}

void NaviDemo::Update()
{
	naviMgr->Update();
	Root::getSingleton().renderOneFrame();
	Ogre::WindowEventUtilities::messagePump();
	InputManager::getSingletonPtr()->capture();
}

void NaviDemo::parseResources()
{
    ConfigFile cf;
	cf.load("resources.cfg");
    ConfigFile::SectionIterator seci = cf.getSectionIterator();

    String secName, typeName, archName;
    while(seci.hasMoreElements())
    {
        secName = seci.peekNextKey();
        ConfigFile::SettingsMultiMap *settings = seci.getNext();
        ConfigFile::SettingsMultiMap::iterator i;
        for(i = settings->begin(); i != settings->end(); ++i)
        {
            typeName = i->first;
            archName = i->second;
            ResourceGroupManager::getSingleton().addResourceLocation(archName, typeName, secName);
        }
    }

	ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}

void NaviDemo::loadInputSystem()
{
	inputMgr = InputManager::getSingletonPtr();
    inputMgr->initialise(renderWin);
    inputMgr->addMouseListener(this, "NaviDemoMouseListener");
	inputMgr->addKeyListener(this, "NaviDemoKeyListener");
}

bool NaviDemo::mouseMoved(const OIS::MouseEvent &arg)
{
	if(arg.state.Z.rel != 0) naviMgr->injectMouseWheel(arg.state.Z.rel);

	return naviMgr->injectMouseMove(arg.state.X.abs, arg.state.Y.abs);
}

bool NaviDemo::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	return naviMgr->injectMouseDown(id);
}

bool NaviDemo::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	return naviMgr->injectMouseUp(id);
}

bool NaviDemo::keyPressed( const OIS::KeyEvent &arg )
{
	if(naviMgr->isAnyNaviFocused())
		return true;

	return true;
}

bool NaviDemo::keyReleased( const OIS::KeyEvent &arg )
{
	switch(arg.key)
	{
	case OIS::KC_ESCAPE:
		shouldQuit = true;
		break;
	case OIS::KC_F1:
	{
		const RenderTarget::FrameStats& stats = renderWin->getStatistics();
		logTemplate("Current FPS: ?", Args((int)stats.lastFPS));
		logTemplate("Avg FPS: ?", Args((int)stats.avgFPS));
		logTemplate("Best FPS: ?", Args((int)stats.bestFPS));
		logTemplate("Worst FPS: ?", Args((int)stats.worstFPS));
		logTemplate("Batch Count: ?", Args((int)stats.batchCount));
		break;
	}
	case OIS::KC_F2:
		sceneMgr->getRootSceneNode()->flipVisibility(true);
		break;
	case OIS::KC_F3:
		naviMgr->resetAllPositions();
		break;
	}

	return true;
}

void NaviDemo::windowMoved(RenderWindow* rw) {}

void NaviDemo::windowResized(RenderWindow* rw) 
{
	inputMgr->setWindowExtents(rw->getWidth(), rw->getHeight());
}

void NaviDemo::windowClosed(RenderWindow* rw) 
{
	shouldQuit = true;
}

void NaviDemo::windowFocusChange(RenderWindow* rw) {}
