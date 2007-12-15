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

#include "Navi.h"
#include "NaviUtilities.h"
#include <OgreBitwise.h>

using namespace Ogre;
using namespace NaviLibrary;
using namespace NaviLibrary::NaviUtilities;

Navi::Navi(Ogre::RenderWindow* renderWin, std::string name, std::string homepage, const NaviPosition &naviPosition, 
		   unsigned short width, unsigned short height, unsigned short zOrder)
{
	naviName = name;
	naviWidth = width;
	naviHeight = height;
	winWidth = renderWin->getWidth();
	winHeight = renderWin->getHeight();
	renderWindow = renderWin;
	isWinFocused = true;
	position = naviPosition;
	movable = true;
	windowID = 0;
	overlay = 0;
	panel = 0;
	needsUpdate = false;
	maxUpdatePS = 48;
	forceMax = false;
	lastUpdateTime = 0;
	opacity = 1;
	usingMask = false;
	ignoringTrans = true;
	transparent = 0.05;
	ignoringBounds = false;
	usingColorKeying = false;
	keyFuzziness = 0.0;
	keyR = keyG = keyB = 255;
	keyFOpacity = 0;
	keyFillR = keyFillG = keyFillB = 255;
	isMaterial = false;
	okayToDelete = false;
	isVisible = true;
	fadingOut = false;
	fadingOutStart = fadingOutEnd = 0;
	fadingIn = false;
	fadingInStart = fadingInEnd = 0;
	compensateNPOT = false;
	texWidth = width;
	texHeight = height;

	createMaterial();
	createOverlay(zOrder);
	createBrowser(renderWin, homepage);

	Ogre::WindowEventUtilities::addWindowEventListener(renderWin, this);
}

Navi::Navi(Ogre::RenderWindow* renderWin, std::string name, std::string homepage, unsigned short width, unsigned short height,
		   Ogre::FilterOptions texFiltering)
{
	naviName = name;
	naviWidth = width;
	naviHeight = height;
	winWidth = renderWin->getWidth();
	winHeight = renderWin->getHeight();
	renderWindow = renderWin;
	isWinFocused = true;
	position = NaviPosition();
	movable = false;
	windowID = 0;
	overlay = 0;
	panel = 0;
	needsUpdate = false;
	maxUpdatePS = 48;
	forceMax = false;
	lastUpdateTime = 0;
	opacity = 1;
	usingMask = false;
	ignoringTrans = true;
	transparent = 0.05;
	ignoringBounds = false;
	usingColorKeying = false;
	keyR = keyG = keyB = 255;
	keyFOpacity = 0;
	keyFillR = keyFillG = keyFillB = 255;
	isMaterial = true;
	okayToDelete = false;
	isVisible = true;
	fadingOut = false;
	fadingOutStart = fadingOutEnd = 0;
	fadingIn = false;
	fadingInStart = fadingInEnd = 0;
	compensateNPOT = false;
	texWidth = width;
	texHeight = height;

	createMaterial(texFiltering);
	createBrowser(renderWin, homepage);	

	WindowEventUtilities::addWindowEventListener(renderWin, this);
}


Navi::~Navi()
{
	delete[] naviCache;

	WindowEventUtilities::removeWindowEventListener(renderWindow, this);

	if(windowID)
	{
		LLMozLib::getInstance()->remObserver(windowID, this);
		LLMozLib::getInstance()->destroyBrowserWindow(windowID);
	}

	if(overlay)
	{
		overlay->remove2D(panel);
		OverlayManager::getSingletonPtr()->destroyOverlayElement(panel);
		OverlayManager::getSingletonPtr()->destroy(overlay);
	}

	MaterialManager::getSingletonPtr()->remove(naviName + "Material");
	TextureManager::getSingletonPtr()->remove(naviName + "Texture");
	if(usingMask) TextureManager::getSingletonPtr()->remove(naviName + "MaskTexture");
}

void Navi::createOverlay(unsigned short zOrder)
{
	OverlayManager& overlayManager = OverlayManager::getSingleton();

	panel = static_cast<PanelOverlayElement*>(overlayManager.createOverlayElement("Panel", naviName + "Panel"));
	panel->setMetricsMode(Ogre::GMM_PIXELS);
	panel->setMaterialName(naviName + "Material");
	panel->setDimensions(naviWidth, naviHeight);
	if(compensateNPOT)
		panel->setUV(0, 0, (Real)naviWidth/(Real)texWidth, (Real)naviHeight/(Real)texHeight);	
	
	overlay = overlayManager.create(naviName + "Overlay");
	overlay->add2D(panel);
	overlay->setZOrder(zOrder);
	resetPosition();
	if(isVisible) overlay->show();
}

void Navi::createBrowser(Ogre::RenderWindow* renderWin, std::string homepage)
{
	size_t tempAttr = 0;
	HWND windowHnd;
	renderWin->getCustomAttribute( "WINDOW", &tempAttr );
	windowHnd = (HWND)tempAttr;

	windowID = LLMozLib::getInstance()->createBrowserWindow(windowHnd, naviWidth, naviHeight);

	LLMozLib::getInstance()->addObserver(windowID,this);
	LLMozLib::getInstance()->setBrowserAgentId(naviName);
	LLMozLib::getInstance()->setEnabled(windowID, true);
	LLMozLib::getInstance()->focusBrowser(windowID, true);

	navigateTo(homepage);
}

void Navi::createMaterial(Ogre::FilterOptions texFiltering)
{
	limit<float>(opacity, 0, 1);

	if(!Bitwise::isPO2(naviWidth) || !Bitwise::isPO2(naviHeight))
	{
		if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_NON_POWER_OF_2_TEXTURES))
		{
			if(Root::getSingleton().getRenderSystem()->getCapabilities()->getNonPOW2TexturesLimited())
				compensateNPOT = true;
		}
		else compensateNPOT = true;
		
		if(compensateNPOT)
		{
			texWidth = Bitwise::firstPO2From(naviWidth);
			texHeight = Bitwise::firstPO2From(naviHeight);
		}
	}

	// Create the texture
	TexturePtr texture = TextureManager::getSingleton().createManual(
		naviName + "Texture", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		TEX_TYPE_2D, texWidth, texHeight, 0, PF_BYTE_BGRA,
		TU_DYNAMIC_WRITE_ONLY_DISCARDABLE, this);

	HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();
	texPixelSize = Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
	texPitch = (pixelBox.rowPitch*texPixelSize);

	naviCache = new unsigned char[texHeight*texPitch];

	uint8* pDest = static_cast<uint8*>(pixelBox.data);

	// Fill the texture with a transparent color
	for(size_t i = 0; i < (size_t)(texHeight*texPitch); i++)
	{
		if((i+1)%texPixelSize)	
			pDest[i] = naviCache[i] = 64; // B, G, R
		else 
			pDest[i] = naviCache[i] = 0; // A
	}

	pixelBuffer->unlock();

	MaterialPtr material = MaterialManager::getSingleton().create(naviName + "Material", 
		ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	material->getTechnique(0)->getPass(0)->setSceneBlending(SBT_TRANSPARENT_ALPHA);
	material->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);

	TextureUnitState* texUnit = material->getTechnique(0)->getPass(0)->createTextureUnitState(naviName + "Texture");

	texUnit->setTextureFiltering(texFiltering, texFiltering, FO_NONE);
	if(texFiltering == FO_ANISOTROPIC)
		texUnit->setTextureAnisotropy(4);
}

// This is for when the rendering device has a hiccup and loses the dynamic texture
void Navi::loadResource(Resource* resource)
{
	Texture *tex = static_cast<Texture*>(resource); 

	tex->setTextureType(TEX_TYPE_2D);
	tex->setWidth(texWidth);
	tex->setHeight(texHeight);
	tex->setNumMipmaps(0);
	tex->setFormat(PF_BYTE_BGRA);
	tex->setUsage(TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
	tex->createInternalResources();

	needsUpdate = true;
	update();
}

void Navi::update()
{
	if(!isWinFocused) return;
	if(!isVisible) return;

	if(forceMax || fadingIn || fadingOut)
	{
		if(maxUpdatePS)
		{
			unsigned long timeSinceLastUpdate = timer.getMilliseconds() - lastUpdateTime;
			unsigned long msBetweenEachUpdate = 1000 / maxUpdatePS;

			if(timeSinceLastUpdate < msBetweenEachUpdate) return;
		}
	}
	else
	{
		if(maxUpdatePS && needsUpdate)
		{
			unsigned long timeSinceLastUpdate = timer.getMilliseconds() - lastUpdateTime;
			unsigned long msBetweenEachUpdate = 1000 / maxUpdatePS;

			if(timeSinceLastUpdate < msBetweenEachUpdate) return;
		}
		else if(!needsUpdate)
			return;
	}
	
	unsigned char* pixels = 0;

	if(needsUpdate || forceMax)
	{
		LLMozLib::getInstance()->grabBrowserWindow(windowID);
		pixels = (unsigned char*)(LLMozLib::getInstance()->getBrowserWindowPixels(windowID));
		if(!pixels) return;
	}

	limit<float>(opacity, 0, 1);

	TexturePtr texture = TextureManager::getSingleton().getByName(naviName + "Texture");
	
	HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();

	uint8* pDest = static_cast<uint8*>(pixelBox.data);

	size_t browserPitch = LLMozLib::getInstance()->getBrowserRowSpan(windowID);
	size_t browserDepth = LLMozLib::getInstance()->getBrowserDepth(windowID);
	
	size_t destPixelSize = texPixelSize;
	size_t pitch = texPitch;
	
	unsigned char B, G, R, A;

	uint8* maskData = 0;
	size_t maskPitch, maskDepth;
	int colDist = 0;
	float tempOpa = 0;
	float fadeMod = 1;

	if(usingMask)
	{
		TexturePtr maskTexture = TextureManager::getSingleton().getByName(naviName + "MaskTexture");
		if(!maskTexture.isNull())
		{
			HardwarePixelBufferSharedPtr maskPBuffer = maskTexture->getBuffer();
			maskData = new uint8[maskPBuffer->getSizeInBytes()];
			PixelBox maskPixelBox(maskPBuffer->getWidth(), maskPBuffer->getHeight(), maskPBuffer->getDepth(), maskPBuffer->getFormat(), maskData);
			maskPBuffer->blitToMemory(maskPixelBox);

			maskDepth = PixelUtil::getNumElemBytes(maskPBuffer->getFormat());
			maskPitch = maskPixelBox.rowPitch*maskDepth;
		}
	}

	if(fadingIn)
	{
		if(fadingInEnd < timer.getMilliseconds())
			fadingInStart = fadingInEnd = fadingIn = 0;
		else
			fadeMod = (float)(timer.getMilliseconds() - fadingInStart) / (float)(fadingInEnd - fadingInStart);
	} 
	else if(fadingOut)
	{
		if(fadingOutEnd < timer.getMilliseconds())
		{
			fadingOutStart = fadingOutEnd = fadeMod = fadingOut = isVisible = 0;

			if(!isMaterial)
				overlay->hide();
		}
		else
			fadeMod = 1 - (float)(timer.getMilliseconds() - fadingOutStart) / (float)(fadingOutEnd - fadingOutStart);
	}

	for(size_t y = 0; y < (size_t)naviHeight; y++)
	{
		for(size_t x = 0; x < naviWidth; x++)
		{	
			if(needsUpdate || forceMax)
			{
				size_t srcx = x * browserDepth;
				B = pixels[(y*browserPitch)+srcx]; //blue
				G = pixels[(y*browserPitch)+srcx+1]; //green
				R = pixels[(y*browserPitch)+srcx+2]; // red
				A = 255 * opacity; //alpha

				if(maskData)
					A = maskData[(y*maskPitch)+(x*maskDepth)+3] * opacity;

				if(usingColorKeying)
				{
					if(!keyFuzziness)
					{
						if(R == keyR && G == keyG && B == keyB)
						{
							R = keyFillR;
							G = keyFillG;
							B = keyFillB;
							A = A * keyFOpacity;
						}
					}
					else
					{
						colDist = abs((int)keyR - (int)R) + abs((int)keyG - (int)G) + abs((int)keyB - (int)B);
						if(colDist < (keyFuzziness * 400))
						{
							R = keyFillR;
							G = keyFillG;
							B = keyFillB;
							tempOpa = (colDist / (keyFuzziness * 400)) + keyFOpacity;
							if(tempOpa > 1) tempOpa = 1;
							A = A * tempOpa;
						}
					}
				}

				size_t destx = x * destPixelSize;
				pDest[y*pitch+destx] = naviCache[y*pitch+destx] = B;
				pDest[y*pitch+destx+1] = naviCache[y*pitch+destx+1] = G;
				pDest[y*pitch+destx+2] = naviCache[y*pitch+destx+2] = R;
				pDest[y*pitch+destx+3] = A * fadeMod;
				naviCache[y*pitch+destx+3] = A;
			}
			else
			{
				size_t destx = x * destPixelSize;
				pDest[y*pitch+destx] = naviCache[y*pitch+destx];
				pDest[y*pitch+destx+1] = naviCache[y*pitch+destx+1];
				pDest[y*pitch+destx+2] = naviCache[y*pitch+destx+2];
				pDest[y*pitch+destx+3] = naviCache[y*pitch+destx+3] * fadeMod;
			}
		}
	}

	if(maskData) delete[] maskData;
	pixelBuffer->unlock();

	needsUpdate = false;
	lastUpdateTime = timer.getMilliseconds();
}

bool Navi::isPointOverMe(int x, int y)
{
	if(x < 0 || x > (int)winWidth) return false;
	if(y < 0 || y > (int)winHeight) return false;
	if(isMaterial || !isVisible) return false;

	if(panel->getLeft() < x && x < (panel->getLeft()+panel->getWidth()))
		if(panel->getTop() < y && y < (panel->getTop()+panel->getHeight()))
			return !ignoringTrans? true : 
				naviCache[getRelativeY(y)*texPitch+getRelativeX(x)*texPixelSize+(texPixelSize-1)] > 255*transparent;

	return false;
}

void Navi::onPageChanged(const EventType& eventIn) 
{
	needsUpdate = true;
}

void Navi::onNavigateBegin(const EventType& eventIn) {}

void Navi::onNavigateComplete(const EventType& eventIn) 
{
	for(std::vector<NaviEventListener*>::const_iterator nel = eventListeners.begin(); nel != eventListeners.end(); ++nel)
		(*nel)->onNavigateComplete(this, eventIn.getEventUri(), eventIn.getIntValue());
}

void Navi::onUpdateProgress(const EventType& eventIn) {}

void Navi::onStatusTextChange(const EventType& eventIn)
{
	std::string statusMsg = eventIn.getStringValue();

	if(isPrefixed(statusMsg, "NAVI_DATA:", false))
	{
		std::vector<std::string> stringVector = split(statusMsg, "?", false);
		if(stringVector.size() == 3)
		{
			NaviData naviDataEvent(stringVector[1], stringVector[2]);

			if(!eventListeners.empty())
				for(std::vector<NaviEventListener*>::const_iterator nel = eventListeners.begin(); nel != eventListeners.end(); nel++)
					(*nel)->onNaviDataEvent(this, naviDataEvent);

			if(!delegateMap.empty())
			{
				ensureKeysMapIter = ensureKeysMap.find(stringVector[1]);
				if(ensureKeysMapIter != ensureKeysMap.end())
					naviDataEvent.ensure(ensureKeysMapIter->second);

				dmBounds = delegateMap.equal_range(stringVector[1]);
				for(delegateIter = dmBounds.first; delegateIter != dmBounds.second; delegateIter++)
					delegateIter->second(naviDataEvent);
			}
		}
	}
}

void Navi::onLocationChange(const EventType& eventIn) 
{
	for(std::vector<NaviEventListener*>::const_iterator nel = eventListeners.begin(); nel != eventListeners.end(); ++nel)
		(*nel)->onLocationChange(this, eventIn.getEventUri());
}

void Navi::onClickLinkHref(const EventType& eventIn) 
{
	for(std::vector<NaviEventListener*>::const_iterator nel = eventListeners.begin(); nel != eventListeners.end(); ++nel)
		(*nel)->onLinkClicked(this, eventIn.getStringValue());
}

void Navi::windowMoved(RenderWindow* rw) {}

void Navi::windowResized(RenderWindow* rw) 
{
	winWidth = rw->getWidth();
	winHeight = rw->getHeight();
}

void Navi::windowClosed(RenderWindow* rw) {}

void Navi::windowFocusChange(RenderWindow* rw) 
{
	isWinFocused = rw->isVisible();
}

void Navi::navigateTo(std::string url)
{
	translateLocalProtocols(url);
	translateResourceProtocols(url);
	LLMozLib::getInstance()->navigateTo(windowID, url);
}

void Navi::navigateTo(std::string url, const NaviData &naviData)
{
	std::string suffix = "";

	if(naviData.getName().length())
		suffix = "?" + naviData.getName() + "?" + naviData.toQueryString();

	translateLocalProtocols(url);
	LLMozLib::getInstance()->navigateTo(windowID, url + suffix);
}

void Navi::navigateBack()
{
	LLMozLib::getInstance()->navigateBack(windowID);
}

void Navi::navigateForward()
{
	LLMozLib::getInstance()->navigateForward(windowID);
}

void Navi::navigateStop()
{
	LLMozLib::getInstance()->navigateStop(windowID);
}

bool Navi::canNavigateForward()
{
	return LLMozLib::getInstance()->canNavigateForward(windowID);
}

bool Navi::canNavigateBack()
{
	return LLMozLib::getInstance()->canNavigateBack(windowID);
}

std::string Navi::evaluateJS(std::string script, const NaviUtilities::Args &args)
{
	if(args.size() && script.size())
	{
		std::vector<std::string> temp = split(script, "?", false);
		script.clear();

		for(unsigned int i = 0; i < temp.size(); ++i)
		{
			script += temp[i];
			if(args.size() > i && i != temp.size()-1)
			{
				if(args[i].isWideString())
				{
					script += "decodeURIComponent(\"" + encodeURIComponent(args[i].wstr()) + "\")";
				}
				else if(args[i].isNumber())
				{
					script += args[i].str();
				}
				else
				{
					std::string escapedStr = args[i].str();
					replaceAll(escapedStr, "\"", "\\\"");
					script += "\"" + escapedStr + "\"";
				}
			}
		}
	}

	return LLMozLib::getInstance()->evaluateJavascript(windowID, script);
}

Navi* Navi::addEventListener(NaviEventListener* newListener)
{
	if(newListener)
	{
		for(std::vector<NaviEventListener*>::iterator i = eventListeners.begin(); i != eventListeners.end(); ++i)
			if(*i == newListener) return this;

		eventListeners.push_back(newListener);
	}

	return this;
}

Navi* Navi::removeEventListener(NaviEventListener* removeListener)
{
	for(std::vector<NaviEventListener*>::iterator i = eventListeners.begin(); i != eventListeners.end();)
	{
		if(*i == removeListener)
			i = eventListeners.erase(i);
		else
			++i;
	}

	return this;
}

Navi* Navi::bind(const std::string &naviDataName, const NaviDelegate &callback, const NaviUtilities::Strings &keys)
{
	if(callback.empty() || naviDataName.empty()) return this;
	
	delegateMap.insert(std::pair<std::string, NaviDelegate>(naviDataName, callback));

	if(keys.size())
		ensureKeysMap[naviDataName] = keys;

	return this;
}

Navi* Navi::unbind(const std::string &naviDataName, const NaviDelegate &callback)
{
	if(delegateMap.empty()) return this;
	dmBounds = delegateMap.equal_range(naviDataName);

	delegateIter = dmBounds.first;
	while(delegateIter != dmBounds.second)
	{
		if(callback.empty())
			delegateIter = delegateMap.erase(delegateIter);
		else
		{
			if(delegateIter->second == callback)
			{
				delegateMap.erase(delegateIter);
				dmBounds = delegateMap.equal_range(naviDataName);
				delegateIter = dmBounds.first;
			}
			else delegateIter++;
		}
	}

	if(!delegateMap.count(naviDataName))
		ensureKeysMap.erase(naviDataName);

	return this;
}

Navi* Navi::setBackgroundColor(float red, float green, float blue)
{
	limit<float>(red, 0, 1);
	limit<float>(green, 0, 1);
	limit<float>(blue, 0, 1);

	LLMozLib::getInstance()->setBackgroundColor(windowID, red*255, green*255, blue*255);

	return this;
}

Navi* Navi::setBackgroundColor(const std::string& hexColor)
{
	unsigned char red, green, blue = 0;

	if(hexStringToRGB(hexColor, red, green, blue))
		LLMozLib::getInstance()->setBackgroundColor(windowID, red, green, blue);

	return this;
}

Navi* Navi::setColorKey(const std::string &keyColor, float keyFillOpacity, const std::string &keyFillColor, float keyFuzzy)
{
	if(keyColor.length())
	{
		if(hexStringToRGB(keyColor, keyR, keyG, keyB) && hexStringToRGB(keyFillColor, keyFillR, keyFillG, keyFillB))
		{
			limit<float>(keyFillOpacity, 0, 1);
			limit<float>(keyFuzzy, 0, 1);

			keyFOpacity = keyFillOpacity;
			usingColorKeying = true;
			keyFuzziness = keyFuzzy;
		}
		else usingColorKeying = false;
	}
	else usingColorKeying = false;
	
	needsUpdate = true;

	return this;
}

Navi* Navi::setForceMaxUpdate(bool forceMaxUpdate)
{
	forceMax = forceMaxUpdate;
	return this;
}

Navi* Navi::setIgnoreBounds(bool ignoreBounds)
{
	ignoringBounds = ignoreBounds;
	return this;
}

Navi* Navi::setIgnoreTransparent(bool ignoreTrans, float threshold)
{
	ignoringTrans = ignoreTrans;

	limit<float>(threshold, 0, 1);

	transparent = threshold;
	return this;
}

Navi* Navi::setMask(std::string maskFileName, std::string groupName)
{
	if(usingMask)
		if(!TextureManager::getSingleton().getByName(naviName + "MaskTexture").isNull())
			TextureManager::getSingleton().remove(naviName + "MaskTexture");

	if(maskFileName == "")
	{
		usingMask = false;
		return this;
	}
	
	Image maskImage;
	maskImage.load(maskFileName, groupName);

	TexturePtr maskTexture = TextureManager::getSingleton().loadImage(
		naviName + "MaskTexture", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		maskImage, TEX_TYPE_2D, 0, 1, false, PF_BYTE_BGRA);

	if(maskTexture->getWidth() < texWidth || maskTexture->getHeight() < texHeight)
		OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, 
			"Mask dimensions must be greater than or equal to the dimensions of the Navi's internal texture. " +
				templateString("Mask Dimensions: ?x?, Texture Dimensions: ?x?", 
				Args(maskTexture->getWidth())(maskTexture->getHeight())(texWidth)(texHeight)),
			"Navi::setMask");

	needsUpdate = true;
	usingMask = true;

	return this;
}

Navi* Navi::setMaxUPS(unsigned int maxUPS)
{
	maxUpdatePS = maxUPS;
	return this;
}

Navi* Navi::setMovable(bool isMovable)
{
	if(!isMaterial)
		movable = isMovable;

	return this;
}

Navi* Navi::setOpacity(float opacity)
{
	limit<float>(opacity, 0, 1);
	
	this->opacity = opacity;

	needsUpdate = true;
	return this;
}

Navi* Navi::setPosition(const NaviPosition &naviPosition)
{
	if(isMaterial)
		return this;

	position = naviPosition;
	resetPosition();
	
	return this;
}

Navi* Navi::resetPosition()
{
	if(isMaterial || !overlay || !panel) return this;

	if(position.usingRelative)
	{
		int left = 0 + position.data.rel.x;
		int center = (winWidth/2)-(naviWidth/2) + position.data.rel.x;
		int right = winWidth - naviWidth + position.data.rel.x;

		int top = 0 + position.data.rel.y;
		int middle = (winHeight/2)-(naviHeight/2) + position.data.rel.y;
		int bottom = winHeight-naviHeight + position.data.rel.y;

		switch(position.data.rel.position)
		{
		case Left:
			panel->setPosition(left, middle);
			break;
		case TopLeft:
			panel->setPosition(left, top);
			break;
		case TopCenter:
			panel->setPosition(center, top);
			break;
		case TopRight:
			panel->setPosition(right, top);
			break;
		case Right:
			panel->setPosition(right, middle);
			break;
		case BottomRight:
			panel->setPosition(right, bottom);
			break;
		case BottomCenter:
			panel->setPosition(center, bottom);
			break;
		case BottomLeft:
			panel->setPosition(left, bottom);
			break;
		case Center:
			panel->setPosition(center, middle);
			break;
		default:
			panel->setPosition(position.data.rel.x, position.data.rel.y);
			break;
		}
	}
	else
		panel->setPosition(position.data.abs.left, position.data.abs.top);

	return this;
}

Navi* Navi::hide(bool fade, unsigned short fadeDurationMS)
{
	if(fadingIn || fadingOut)
		fadingInStart = fadingInEnd = fadingOutStart = fadingOutEnd = fadingIn = fadingOut = 0;

	if(fade)
	{
		fadingOutStart = timer.getMilliseconds();
		fadingOutEnd = timer.getMilliseconds() + fadeDurationMS + 1;
		fadingOut = true;
	}
	else
	{
		if(!isMaterial) overlay->hide();
		isVisible = false;
	}

	return this;
}

Navi* Navi::show(bool fade, unsigned short fadeDurationMS)
{
	if(fadingIn || fadingOut)
		fadingInStart = fadingInEnd = fadingOutStart = fadingOutEnd = fadingIn = fadingOut = 0;

	if(fade)
	{
		fadingInStart = timer.getMilliseconds();
		fadingInEnd = timer.getMilliseconds() + fadeDurationMS + 1;
		fadingIn = true;
	}
	else needsUpdate = true;

	isVisible = true;
	if(!isMaterial) overlay->show();

	return this;
}

Navi* Navi::focus()
{
	if(NaviManager::GetPointer() && !isMaterial)
		NaviManager::GetPointer()->focusNavi(0, 0, this);

	return this;
}

Navi* Navi::moveNavi(int deltaX, int deltaY)
{
	if(!isMaterial)
		panel->setPosition(panel->getLeft()+deltaX, panel->getTop()+deltaY);
	return this;
}

void Navi::getExtents(unsigned short &width, unsigned short &height)
{
	width = naviWidth;
	height = naviHeight;
}

int Navi::getRelativeX(int absX)
{
	if(isMaterial) return 0;

	int relX = absX - panel->getLeft();
	limit<int>(relX, 0, naviWidth-1);

	return relX;
}

int Navi::getRelativeY(int absY)
{
	if(isMaterial) return 0;

	int relY = absY - panel->getTop();
	limit<int>(relY, 0, naviHeight - 1);
	
	return relY;
}

bool Navi::isMaterialOnly()
{
	return isMaterial;
}

Ogre::PanelOverlayElement* Navi::getInternalPanel()
{
	if(isMaterial)
		return 0;
		
	return panel;
}

std::string Navi::getName()
{
	return naviName;
}

std::string Navi::getMaterialName()
{
	return naviName + "Material";
}

bool Navi::getVisibility()
{
	return isVisible;
}

void Navi::getDerivedUV(Ogre::Real& u1, Ogre::Real& v1, Ogre::Real& u2, Ogre::Real& v2)
{
	u1 = v1 = 0;
	u2 = v2 = 1;

	if(compensateNPOT)
	{
		u2 = (Ogre::Real)naviWidth/texWidth;
		v2 = (Ogre::Real)naviHeight/(Ogre::Real)texHeight;
	}
}

void Navi::injectMouseMove(int xPos, int yPos)
{
	LLMozLib::getInstance()->mouseMove(windowID, xPos, yPos);
}

void Navi::injectMouseWheel(int relScroll)
{
	LLMozLib::getInstance()->scrollByLines(windowID, -(relScroll/30));
}

void Navi::injectMouseDown(int xPos, int yPos)
{
	LLMozLib::getInstance()->mouseDown(windowID, xPos, yPos);
}

void Navi::injectMouseUp(int xPos, int yPos)
{
	LLMozLib::getInstance()->mouseUp(windowID, xPos, yPos);
}