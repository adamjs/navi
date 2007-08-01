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

using namespace Ogre;
using namespace NaviLibrary;

Navi::Navi(Ogre::RenderWindow* renderWin, std::string name, std::string homepage, const NaviPosition &naviPosition,
	unsigned short width, unsigned short height, bool isMovable, bool visible, unsigned int maxUpdatesPerSec, bool forceMaxUpdate, unsigned short zOrder, float _opacity)
{
	naviName = name;
	naviWidth = width;
	naviHeight = height;
	winWidth = renderWin->getWidth();
	winHeight = renderWin->getHeight();
	renderWindow = renderWin;
	isWinFocused = true;
	position = naviPosition;
	movable = isMovable;
	windowID = 0;
	overlay = 0;
	panel = 0;
	needsUpdate = false;
	maxUpdatePS = maxUpdatesPerSec;
	forceMax = forceMaxUpdate;
	lastUpdateTime = 0;
	opacity = _opacity;
	usingMask = false;
	ignoringTrans = true;
	transparent = 0.05;
	usingColorKeying = false;
	keyFuzziness = 0.0;
	keyR = keyG = keyB = 255;
	keyFOpacity = 0;
	keyFillR = keyFillG = keyFillB = 255;
	alphaCache = new unsigned char[naviWidth*naviHeight];
	for(int i = 0; i < naviWidth*naviHeight; i++) alphaCache[i] = 255;
	isMaterialOnly = false;
	okayToDelete = false;
	isVisible = visible;
	fadingOut = false;
	fadingOutStart = 0;
	fadingOutEnd = 0;
	fadingIn = false;
	fadingInStart = 0;
	fadingInEnd = 0;

	createMaterial();
	createOverlay(zOrder);
	createBrowser(renderWin, homepage);

	Ogre::WindowEventUtilities::addWindowEventListener(renderWin, this);
}

Navi::Navi(Ogre::RenderWindow* renderWin, std::string name, std::string homepage, unsigned short width, unsigned short height, bool visible,
			unsigned int maxUpdatesPerSec, bool forceMaxUpdate, float _opacity, Ogre::FilterOptions texFiltering)
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
	maxUpdatePS = maxUpdatesPerSec;
	forceMax = forceMaxUpdate;
	lastUpdateTime = 0;
	opacity = _opacity;
	usingMask = false;
	ignoringTrans = true;
	transparent = 0.05;
	usingColorKeying = false;
	keyR = keyG = keyB = 255;
	keyFOpacity = 0;
	keyFillR = keyFillG = keyFillB = 255;
	alphaCache = new unsigned char[naviWidth*naviHeight];
	for(int i = 0; i < naviWidth*naviHeight; i++) alphaCache[i] = 255;
	isMaterialOnly = true;
	okayToDelete = false;
	isVisible = visible;
	fadingOut = false;
	fadingOutStart = 0;
	fadingOutEnd = 0;
	fadingIn = false;
	fadingInStart = 0;
	fadingInEnd = 0;

	createMaterial(texFiltering);
	createBrowser(renderWin, homepage);	

	WindowEventUtilities::addWindowEventListener(renderWin, this);
}


Navi::~Navi()
{
	delete[] alphaCache;

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

	panel = static_cast<OverlayContainer*>(overlayManager.createOverlayElement("Panel", naviName + "Panel"));
	panel->setMetricsMode(Ogre::GMM_PIXELS);
	panel->setDimensions(naviWidth, naviHeight);
	panel->setMaterialName(naviName + "Material");
	
	overlay = overlayManager.create(naviName + "Overlay");
	overlay->add2D(panel);
	overlay->setZOrder(zOrder);
	setDefaultPosition();
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
	if(opacity > 1) opacity = 1;
	if(opacity < 0) opacity = 0;

	// Create the texture
	TexturePtr texture = TextureManager::getSingleton().createManual(
		naviName + "Texture", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		TEX_TYPE_2D, naviWidth, naviHeight, 0, PF_BYTE_BGRA,
		TU_DYNAMIC_WRITE_ONLY_DISCARDABLE, this);

	HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();

	uint8* pDest = static_cast<uint8*>(pixelBox.data);

	// Fill the texture with a transparent color
	for(size_t i = 0; i < (size_t)(naviHeight*naviWidth*4); i++)
	{
		if((i+1)%4)	
			pDest[i] = 64; // B, G, R
		else 
			pDest[i] = 0; // A
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

void Navi::setMask(std::string maskFileName, std::string groupName)
{
	if(usingMask)
		if(!TextureManager::getSingleton().getByName(naviName + "MaskTexture").isNull())
			TextureManager::getSingleton().remove(naviName + "MaskTexture");

	if(maskFileName == "")
	{
		usingMask = false;
		return;
	}
	
	Image maskImage;
	maskImage.load(maskFileName, groupName);

	TexturePtr maskTexture = TextureManager::getSingleton().loadImage(
		naviName + "MaskTexture", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		maskImage, TEX_TYPE_2D, 0, 1, false, PF_BYTE_BGRA);

	if(maskTexture->getWidth() < naviWidth || maskTexture->getHeight() < naviHeight)
		OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, 
			"Mask width and height must each be greater than or equal to the width and height of the Navi.", 
			"Navi::setMask");

	needsUpdate = true;
	usingMask = true;
}

void Navi::update()
{
	// No sense in updating if the Render Window isn't even visible
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

	if(opacity > 1) opacity = 1;
	if(opacity < 0) opacity = 0;

	TexturePtr texture = TextureManager::getSingleton().getByName(naviName + "Texture");
	
	uint8* copyDataBuffer = 0;

	if(!(needsUpdate || forceMax))
	{
		// This is for fading, we don't want to make Gecko render more than it already has to
		// thus, we make a copy of the existing buffer
		HardwarePixelBufferSharedPtr copyBuffer = texture->getBuffer();
		copyDataBuffer = new uint8[copyBuffer->getSizeInBytes()];
		PixelBox copyPBox(copyBuffer->getWidth(), copyBuffer->getHeight(), copyBuffer->getDepth(), copyBuffer->getFormat(), copyDataBuffer);
		copyBuffer->blitToMemory(copyPBox);
		pixels = static_cast<uint8*>(copyPBox.data);
	}

	HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();

	uint8* pDest = static_cast<uint8*>(pixelBox.data);

	size_t browserPitch = LLMozLib::getInstance()->getBrowserRowSpan(windowID);
	size_t browserDepth = LLMozLib::getInstance()->getBrowserDepth(windowID);
	
	size_t destPixelSize = Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
	size_t pitch = (pixelBox.rowPitch*destPixelSize);
	
	unsigned char B, G, R, A;

	HardwarePixelBufferSharedPtr maskPBuffer;
	uint8* maskData;
	size_t maskWidth, mwOffset;
	bool validMask = false;
	int colDist = 0;
	float tempOpa = 0;
	float fadeMod = 1;

	if(usingMask)
	{
		TexturePtr maskTexture = TextureManager::getSingleton().getByName(naviName + "MaskTexture");
		if(!maskTexture.isNull())
		{
			maskPBuffer = maskTexture->getBuffer();

			// Lock the Mask Texture pixel buffer and get a pixel box
			maskPBuffer->lock(HardwareBuffer::HBL_READ_ONLY);
			const PixelBox& maskPBox = maskPBuffer->getCurrentLock();

			maskData = static_cast<uint8*>(maskPBox.data);

			maskWidth = maskTexture->getWidth();
			mwOffset = 0;
			if(maskWidth-naviWidth > 0) mwOffset = (maskWidth-naviWidth)*4;
			validMask = true;
		}
	}

	if(fadingIn)
	{
		if(fadingInEnd < timer.getMilliseconds())
		{
			fadingInStart = 0;
			fadingInEnd = 0;
			fadingIn = false;
		}
		else
			fadeMod = (float)(timer.getMilliseconds() - fadingInStart) / (float)(fadingInEnd - fadingInStart);
	} 
	else if(fadingOut)
	{
		if(fadingOutEnd < timer.getMilliseconds())
		{
			fadingOutStart = 0;
			fadingOutEnd = 0;
			fadingOut = false;
			isVisible = false;
			if(!isMaterialOnly) overlay->hide();
			fadeMod = 0;
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

				if(validMask)
					A = maskData[(y*browserPitch)+(y*mwOffset)+srcx+3] * opacity;

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
						colDist = colorDistanceRGB(keyR, keyG, keyB, R, G, B);
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
			}
			else
			{
				size_t srcx = x * browserDepth;
				// Get values from copied data
				B = pixels[(y*pitch)+srcx]; //blue
				G = pixels[(y*pitch)+srcx+1]; //green
				R = pixels[(y*pitch)+srcx+2]; // red
				A = alphaCache[y*naviWidth+(x)];  //alpha
			}

			size_t destx = x * destPixelSize;
			pDest[y*pitch+destx] = B;
			pDest[y*pitch+destx+1] = G;
			pDest[y*pitch+destx+2] = R;
			pDest[y*pitch+destx+3] = A * fadeMod;

			alphaCache[y*naviWidth+(x)] = A;
		}
	}

	if(validMask)
		maskPBuffer->unlock();

			
	pixelBuffer->unlock();

	if(!(needsUpdate || forceMax))
		delete[] copyDataBuffer;

	needsUpdate = false;
	lastUpdateTime = timer.getMilliseconds();
}

// This is for when the rendering device has a hiccup and loses the dynamic texture
void Navi::loadResource(Resource* resource)
{
	Texture *tex = static_cast<Texture*>(resource); 

	tex->setTextureType(TEX_TYPE_2D);
	tex->setWidth(naviWidth);
	tex->setHeight(naviHeight);
	tex->setNumMipmaps(0);
	tex->setFormat(PF_BYTE_BGRA);
	tex->setUsage(TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
	tex->createInternalResources();

	needsUpdate = true;
	update();
}

void Navi::moveNavi(int deltaX, int deltaY)
{
	if(movable && !isMaterialOnly)
		panel->setPosition(panel->getLeft()+deltaX, panel->getTop()+deltaY);
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
		suffix = "?" + naviData.getName() + "?" + naviData.dataString;

	translateLocalProtocols(url);
	LLMozLib::getInstance()->navigateTo(windowID, url + suffix);
}

std::string Navi::evaluateJS(const std::string &script)
{
	return LLMozLib::getInstance()->evaluateJavascript(windowID, script);
}

void Navi::addEventListener(NaviEventListener* newListener)
{
	if(newListener)
	{
		bool okayToAdd = true;
		for each(NaviEventListener* test in eventListeners)
			if(test == newListener) okayToAdd = false;

		if(okayToAdd)
			eventListeners.push_back(newListener);
	}
}

void Navi::removeEventListener(NaviEventListener* removeListener)
{
	std::vector<NaviEventListener*>::iterator elIter;
	elIter = eventListeners.begin();
	
	// Just to be paranoid, we loop through them all.
	while(elIter != eventListeners.end())
	{
		if((*elIter) == removeListener)
			elIter = eventListeners.erase(elIter);
		else
			elIter++;
	}
}

void Navi::bindNaviData(const std::string &naviDataName, const NaviDelegate &callback)
{
	if(callback.empty() || naviDataName.empty()) return;
		delegateMap.insert(std::pair<std::string, NaviDelegate>(naviDataName, callback));
}

void Navi::unbindNaviData(const std::string &naviDataName, const NaviDelegate &callback)
{
	if(delegateMap.empty()) return;
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
}

void Navi::setBackgroundColor(float red, float green, float blue)
{
	if(red > 1) red = 1;
	if(red < 0) red = 0;
	if(green > 1) green = 1;
	if(green < 0) green = 0;
	if(blue > 1) blue = 1;
	if(blue < 0) blue = 0;

	LLMozLib::getInstance()->setBackgroundColor(windowID, red*255, green*255, blue*255);
}

void Navi::setOpacity(float _opacity)
{
	if(_opacity > 1) _opacity = 1;
	if(_opacity < 0) _opacity = 0;
	
	opacity = _opacity;

	needsUpdate = true;
}

void Navi::setIgnoreTransparentAreas(bool ignoreTrans, float defineThreshold)
{
	ignoringTrans = ignoreTrans;

	if(defineThreshold > 1) defineThreshold = 1;
	if(defineThreshold < 0) defineThreshold = 0;

	transparent = defineThreshold;
}

void Navi::setColorKey(const std::string &keyColor, float keyFillOpacity, const std::string &keyFillColor, float keyFuzzy)
{
	if(keyColor.length())
	{
		if(hexStringToRGB(keyColor, keyR, keyG, keyB) && hexStringToRGB(keyFillColor, keyFillR, keyFillG, keyFillB))
		{
			if(keyFillOpacity > 1) keyFillOpacity = 1;
			if(keyFillOpacity < 0) keyFillOpacity = 0;
			if(keyFuzzy > 1) keyFuzzy = 1;
			if(keyFuzzy < 0) keyFuzzy = 0;

			keyFOpacity = keyFillOpacity;
			usingColorKeying = true;
			keyFuzziness = keyFuzzy;
		}
		else usingColorKeying = false;
	}
	else usingColorKeying = false;
	
	needsUpdate = true;
}

void Navi::setDefaultPosition()
{
	if(isMaterialOnly || !overlay || !panel) return;

	if(position.usingRelative && movable)
	{
		switch(position.data.rel.position)
		{
		case Left:
			panel->setPosition(0 + position.data.rel.x, (winHeight/2)-(naviHeight/2) + position.data.rel.y);
			break;
		case TopLeft:
			panel->setPosition(0 + position.data.rel.x, 0 + position.data.rel.y);
			break;
		case TopCenter:
			panel->setPosition((winWidth/2)-(naviWidth/2) + position.data.rel.x, 0 + position.data.rel.y);
			break;
		case TopRight:
			panel->setPosition(winWidth - naviWidth + position.data.rel.x, 0 + position.data.rel.y);
			break;
		case Right:
			panel->setPosition(winWidth-naviWidth + position.data.rel.x, (winHeight/2)-(naviHeight/2) + position.data.rel.y);
			break;
		case BottomRight:
			panel->setPosition(winWidth-naviWidth + position.data.rel.x, winHeight-naviHeight + position.data.rel.y);
			break;
		case BottomCenter:
			panel->setPosition((winWidth/2)-(naviWidth/2) + position.data.rel.x, winHeight-naviHeight + position.data.rel.y);
			break;
		case BottomLeft:
			panel->setPosition(0 + position.data.rel.x, winHeight-naviHeight + position.data.rel.y);
			break;
		case Center:
			panel->setPosition((winWidth/2)-(naviWidth/2) + position.data.rel.x, (winHeight/2)-(naviHeight/2) + position.data.rel.y);
			break;
		default:
			panel->setPosition(position.data.rel.x, position.data.rel.y);
			break;
		}
	}
	else if(position.usingRelative && !movable)
	{
		switch(position.data.rel.position)
		{
		case Left:
			panel->setVerticalAlignment(GVA_CENTER);
			panel->setPosition(0 + position.data.rel.x, -(naviHeight/2) + position.data.rel.y);
			break;
		case TopLeft:
			panel->setPosition(0 + position.data.rel.x, 0 + position.data.rel.y);
			break;
		case TopCenter:
			panel->setHorizontalAlignment(GHA_CENTER);
			panel->setPosition(-(naviWidth/2) + position.data.rel.x, 0 + position.data.rel.y);
			break;
		case TopRight:
			panel->setHorizontalAlignment(GHA_RIGHT);
			panel->setPosition(-naviWidth + position.data.rel.x, 0 + position.data.rel.y);
			break;
		case Right:
			panel->setVerticalAlignment(GVA_CENTER);
			panel->setHorizontalAlignment(GHA_RIGHT);
			panel->setPosition(-naviWidth + position.data.rel.x, -(naviHeight/2) + position.data.rel.y);
			break;
		case BottomRight:
			panel->setVerticalAlignment(GVA_BOTTOM);
			panel->setHorizontalAlignment(GHA_RIGHT);
			panel->setPosition(-naviWidth + position.data.rel.x, -naviHeight + position.data.rel.y);
			break;
		case BottomCenter:
			panel->setVerticalAlignment(GVA_BOTTOM);
			panel->setHorizontalAlignment(GHA_CENTER);
			panel->setPosition(-(naviWidth/2) + position.data.rel.x, -naviHeight + position.data.rel.y);
			break;
		case BottomLeft:
			panel->setVerticalAlignment(GVA_BOTTOM);
			panel->setPosition(0 + position.data.rel.x, -naviHeight + position.data.rel.y);
			break;
		case Center:
			panel->setVerticalAlignment(GVA_CENTER);
			panel->setHorizontalAlignment(GHA_CENTER);
			panel->setPosition(-(naviWidth/2) + position.data.rel.x, -(naviHeight/2) + position.data.rel.y);
			break;
		default:
			panel->setPosition(position.data.rel.x, position.data.rel.y);
			break;
		}
	}
	else
		panel->setPosition(position.data.abs.left, position.data.abs.top);
}

void Navi::hide(bool fade, unsigned short fadeDurationMS)
{
	if(!isVisible) return;

	if(fadingIn || fadingOut)
	{
		fadingInStart = 0;
		fadingInEnd = 0;
		fadingIn = false;
		fadingOutStart = 0;
		fadingOutEnd = 0;
		fadingOut = false;
	}

	if(fade)
	{
		fadingOutStart = timer.getMilliseconds();
		fadingOutEnd = timer.getMilliseconds() + fadeDurationMS + 1; // The +1 is to avoid division by 0 later
		fadingOut = true;
	}
	else
	{
		if(!isMaterialOnly) overlay->hide();
	}
}

void Navi::show(bool fade, unsigned short fadeDurationMS)
{
	if(isVisible) return;

	if(fadingIn || fadingOut)
	{
		fadingInStart = 0;
		fadingInEnd = 0;
		fadingIn = false;
		fadingOutStart = 0;
		fadingOutEnd = 0;
		fadingOut = false;
	}

	if(fade)
	{
		fadingInStart = timer.getMilliseconds();
		fadingInEnd = timer.getMilliseconds() + fadeDurationMS + 1; // The +1 is to avoid division by 0 later
		fadingIn = true;
	}
	else
		needsUpdate = true;
	
	isVisible = true;
	if(!isMaterialOnly) overlay->show();
}

bool Navi::isPointOverMe(int x, int y)
{
	if(x < 0 || x > (int)winWidth) return false;
	if(y < 0 || y > (int)winHeight) return false;
	if(isMaterialOnly || !isVisible) return false;

	// For absolute-positioned Navis
	if(panel->getVerticalAlignment()==GVA_TOP && panel->getHorizontalAlignment()==GHA_LEFT)
	{
		if(isPointWithin(x, y, panel->getLeft(), (panel->getLeft()+panel->getWidth()), panel->getTop(), (panel->getTop()+panel->getHeight())))
			return isPointOpaqueEnough(getRelativeX(x), getRelativeY(y));
		else
			return false;
	}
	else
	{
		// Hooray for relative-coordinate Kung Foo!

		int left, right, top, bottom = 0;
		
		if(panel->getHorizontalAlignment()==GHA_LEFT)
		{
			left = 0 + position.data.rel.x;
			right = left + naviWidth;
		}
		else if(panel->getHorizontalAlignment()==GHA_CENTER)
		{
			left = (winWidth/2)-(naviWidth/2) + position.data.rel.x;
			right = left + naviWidth;
		}
		else if(panel->getHorizontalAlignment()==GHA_RIGHT)
		{
			left = winWidth - naviWidth + position.data.rel.x;
			right = left + naviWidth;
		}

		if(panel->getVerticalAlignment()==GVA_TOP)
		{
			top = 0 + position.data.rel.y;
			bottom = top + naviHeight;
		}
		else if(panel->getVerticalAlignment()==GVA_CENTER)
		{
			top = (winHeight/2)-(naviHeight/2) + position.data.rel.y;
			bottom = top + naviHeight;
		}
		else if(panel->getVerticalAlignment()==GVA_BOTTOM)
		{
			top = winHeight - naviHeight + position.data.rel.y;
			bottom = top + naviHeight;
		}
		
		if(isPointWithin(x, y, left, right, top, bottom))
			return isPointOpaqueEnough(getRelativeX(x), getRelativeY(y));
		else
			return false;
	}

	return false;
}

bool Navi::isPointWithin(int x, int y, int left, int right, int top, int bottom)
{
	if(left < x && x < right) if(top < y && y < bottom)	return true;

	return false;
}

bool Navi::isPointOpaqueEnough(int x, int y)
{
	if(!ignoringTrans)
		return true;

	return alphaCache[y*naviWidth+x] > (255*transparent);
}

int Navi::getRelativeX(int absX)
{
	if(isMaterialOnly) return 0;
	int left = 0;

	if(panel->getHorizontalAlignment()==GHA_LEFT)
		left = panel->getLeft();
	else if(panel->getHorizontalAlignment()==GHA_CENTER)
		left = (winWidth/2)-(naviWidth/2) + position.data.rel.x;
	else if(panel->getHorizontalAlignment()==GHA_RIGHT)
		left = winWidth - naviWidth + position.data.rel.x;

	if(absX - left < 0)
		return 0;
	else if(naviWidth - 1 < absX - left)
		return naviWidth - 1;

	return absX - left;
}

int Navi::getRelativeY(int absY)
{
	if(isMaterialOnly) return 0;
	int top = 0;

	if(panel->getVerticalAlignment()==GVA_TOP)
		top = panel->getTop();
	else if(panel->getVerticalAlignment()==GVA_CENTER)
		top = (winHeight/2)-(naviHeight/2) + position.data.rel.y;
	else if(panel->getVerticalAlignment()==GVA_BOTTOM)
		top = winHeight - naviHeight + position.data.rel.y;

	if(absY - top < 0)
		return 0;
	else if(naviHeight - 1 < absY - top)
		return naviHeight - 1;
	
	return absY - top;
}

void Navi::onStatusTextChange(const EventType& eventIn)
{
	std::string statusMsg = eventIn.getStringValue();
	if(statusMsg.substr(0, 10) == "NAVI_DATA:")
	{
		std::string naviDataStr = statusMsg.substr(10);
		std::string naviDataName = "";
		size_t idx = naviDataStr.find_first_of("?");
		size_t endIdx;
		if(idx != std::string::npos)
		{
			idx++;
			endIdx = naviDataStr.find_first_of("?", idx);
			if(endIdx != std::string::npos)
			{
				naviDataName = naviDataStr.substr(idx, endIdx-1);
				naviDataStr = naviDataStr.substr(endIdx+1);
				NaviData naviDataEvent(naviDataName, naviDataStr);

				if(!eventListeners.empty())
					for(std::vector<NaviEventListener*>::const_iterator nel = eventListeners.begin(); nel != eventListeners.end(); nel++)
						(*nel)->onNaviDataEvent(naviName, naviDataEvent);

				if(!delegateMap.empty())
				{
					dmBounds = delegateMap.equal_range(naviDataName);
					for(delegateIter = dmBounds.first; delegateIter != dmBounds.second; delegateIter++)
						delegateIter->second(naviDataEvent);
				}
			}
		}
		
	}
}

void Navi::onPageChanged(const EventType& eventIn) 
{
	needsUpdate = true;
}

void Navi::onNavigateBegin(const EventType& eventIn) {}

void Navi::onNavigateComplete(const EventType& eventIn) {}

void Navi::onUpdateProgress(const EventType& eventIn) {}
void Navi::onLocationChange(const EventType& eventIn) {}
void Navi::onClickLinkHref(const EventType& eventIn) 
{
	for each(NaviEventListener* nel in eventListeners)
		nel->onNaviLinkClicked(naviName, eventIn.getStringValue());
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
