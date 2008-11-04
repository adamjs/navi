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

Navi::Navi(Ogre::RenderWindow* renderWin, std::string name, const NaviPosition &naviPosition, 
		   unsigned short width, unsigned short height, unsigned short zOrder)
{
	webView = 0;
	naviName = name;
	naviWidth = width;
	naviHeight = height;
	winWidth = renderWin->getWidth();
	winHeight = renderWin->getHeight();
	renderWindow = renderWin;
	isWinFocused = true;
	position = naviPosition;
	movable = true;
	overlay = 0;
	panel = 0;
	maxUpdatePS = 0;
	lastUpdateTime = 0;
	opacity = 1;
	usingMask = false;
	ignoringTrans = true;
	transparent = 0.05;
	ignoringBounds = false;
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
	maskCache = 0;
	maskPitch = 0;
	matPass = 0;
	baseTexUnit = 0;
	maskTexUnit = 0;

	createMaterial();
	createOverlay(zOrder);
	createWebView();

	Ogre::WindowEventUtilities::addWindowEventListener(renderWin, this);
}

Navi::Navi(Ogre::RenderWindow* renderWin, std::string name, unsigned short width, unsigned short height,
		   Ogre::FilterOptions texFiltering)
{
	webView = 0;
	naviName = name;
	naviWidth = width;
	naviHeight = height;
	winWidth = renderWin->getWidth();
	winHeight = renderWin->getHeight();
	renderWindow = renderWin;
	isWinFocused = true;
	position = NaviPosition();
	movable = false;
	overlay = 0;
	panel = 0;
	maxUpdatePS = 0;
	lastUpdateTime = 0;
	opacity = 1;
	usingMask = false;
	ignoringTrans = true;
	transparent = 0.05;
	ignoringBounds = false;
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
	maskCache = 0;
	maskPitch = 0;
	matPass = 0;
	baseTexUnit = 0;
	maskTexUnit = 0;

	createMaterial(texFiltering);
	createWebView();	

	WindowEventUtilities::addWindowEventListener(renderWin, this);
}


Navi::~Navi()
{
	if(maskCache)
		delete[] maskCache;

	WindowEventUtilities::removeWindowEventListener(renderWindow, this);

	if(webView)
		webView->destroy();

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

void Navi::createWebView()
{
	webView = Awesomium::WebCore::Get().createWebView(naviWidth, naviHeight);
	webView->setListener(this);
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
		TEX_TYPE_2D, texWidth, texHeight, 0, PF_BYTE_BGR,
		TU_DYNAMIC_WRITE_ONLY_DISCARDABLE, this);

	HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();
	texDepth = Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
	texPitch = (pixelBox.rowPitch*texDepth);

	uint8* pDest = static_cast<uint8*>(pixelBox.data);

	memset(pDest, 128, texHeight*texPitch);

	pixelBuffer->unlock();

	MaterialPtr material = MaterialManager::getSingleton().create(naviName + "Material", 
		ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	matPass = material->getTechnique(0)->getPass(0);
	matPass->setSceneBlending(SBT_TRANSPARENT_ALPHA);
	matPass->setDepthWriteEnabled(false);

	baseTexUnit = matPass->createTextureUnitState(naviName + "Texture");
	
	baseTexUnit->setTextureFiltering(texFiltering, texFiltering, FO_NONE);
	if(texFiltering == FO_ANISOTROPIC)
		baseTexUnit->setTextureAnisotropy(4);
}

// This is for when the rendering device has a hiccup and loses the dynamic texture
void Navi::loadResource(Resource* resource)
{
	Texture *tex = static_cast<Texture*>(resource); 

	tex->setTextureType(TEX_TYPE_2D);
	tex->setWidth(texWidth);
	tex->setHeight(texHeight);
	tex->setNumMipmaps(0);
	tex->setFormat(PF_BYTE_BGR);
	tex->setUsage(TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
	tex->createInternalResources();

	// force update
}

void Navi::update()
{
	if(maxUpdatePS)
		if(timer.getMilliseconds() - lastUpdateTime < 1000 / maxUpdatePS)
			return;

	Ogre::Real fadeMod = 1;
	
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

	baseTexUnit->setAlphaOperation(LBX_SOURCE1, LBS_MANUAL, LBS_CURRENT, fadeMod * opacity);

	if(!webView->isDirty())
		return;

	TexturePtr texture = TextureManager::getSingleton().getByName(naviName + "Texture");
	
	HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();

	uint8* destBuffer = static_cast<uint8*>(pixelBox.data);

	webView->render(destBuffer, (int)texPitch, (int)texDepth);

	pixelBuffer->unlock();

	lastUpdateTime = timer.getMilliseconds();
}

bool Navi::isPointOverMe(int x, int y)
{
	if(x < 0 || x > (int)winWidth) return false;
	if(y < 0 || y > (int)winHeight) return false;
	if(isMaterial || !isVisible) return false;

	if(panel->getLeft() < x && x < (panel->getLeft()+panel->getWidth()))
		if(panel->getTop() < y && y < (panel->getTop()+panel->getHeight()))
			return !ignoringTrans || !maskCache ? true : 
				maskCache[getRelativeY(y)*maskPitch+getRelativeX(x)*1+(1-1)] > 255*transparent;

	return false;
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

void Navi::loadURL(const std::string& url)
{
	webView->loadURL(url);
}

void Navi::loadFile(const std::string& file)
{
	webView->loadFile(file);
}

void Navi::loadHTML(const std::string& html)
{
	webView->loadHTML(html);
}

void Navi::evaluateJS(const std::string& javascript)
{
	webView->executeJavascript(javascript);
}

void Navi::setCallback(const std::string& name, const NaviDelegate& callback)
{
	delegateMap[name] = callback;

	webView->setCallback(name);
}

void Navi::setProperty(const std::string& name, const Awesomium::JSValue& value)
{
	webView->setProperty(name, value);
}

void Navi::setIgnoreBounds(bool ignoreBounds)
{
	ignoringBounds = ignoreBounds;
}

void Navi::setIgnoreTransparent(bool ignoreTrans, float threshold)
{
	ignoringTrans = ignoreTrans;

	limit<float>(threshold, 0, 1);

	transparent = threshold;
}

void Navi::setMask(std::string maskFileName, std::string groupName)
{
	if(usingMask)
	{
		if(maskTexUnit)
		{
			matPass->removeTextureUnitState(1);
			maskTexUnit = 0;
		}

		if(!TextureManager::getSingleton().getByName(naviName + "MaskTexture").isNull())
			TextureManager::getSingleton().remove(naviName + "MaskTexture");
	}

	if(maskCache)
	{
		delete[] maskCache;
		maskCache = 0;
	}

	if(maskFileName == "")
	{
		usingMask = false;
		return;
	}

	if(!maskTexUnit)
	{
		maskTexUnit = matPass->createTextureUnitState();
		maskTexUnit->setIsAlpha(true);
		maskTexUnit->setTextureFiltering(FO_NONE, FO_NONE, FO_NONE);
		maskTexUnit->setColourOperationEx(LBX_SOURCE1, LBS_CURRENT, LBS_CURRENT);
		maskTexUnit->setAlphaOperation(LBX_MODULATE);
	}
	
	Image srcImage;
	srcImage.load(maskFileName, groupName);

	Ogre::PixelBox srcPixels = srcImage.getPixelBox();
	unsigned char* conversionBuf = 0;
	
	if(srcImage.getFormat() != Ogre::PF_BYTE_A)
	{
		size_t dstBpp = Ogre::PixelUtil::getNumElemBytes(Ogre::PF_BYTE_A);
		conversionBuf = new unsigned char[srcImage.getWidth() * srcImage.getHeight() * dstBpp];
		Ogre::PixelBox convPixels(Ogre::Box(0, 0, srcImage.getWidth(), srcImage.getHeight()), Ogre::PF_BYTE_A, conversionBuf);
		Ogre::PixelUtil::bulkPixelConversion(srcImage.getPixelBox(), convPixels);
		srcPixels = convPixels;
	}

	TexturePtr maskTexture = TextureManager::getSingleton().createManual(
		naviName + "MaskTexture", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		TEX_TYPE_2D, texWidth, texHeight, 0, PF_BYTE_A, TU_STATIC_WRITE_ONLY);

	HardwarePixelBufferSharedPtr pixelBuffer = maskTexture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();
	size_t maskTexDepth = Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
	maskPitch = pixelBox.rowPitch;

	maskCache = new unsigned char[maskPitch*texHeight];

	uint8* buffer = static_cast<uint8*>(pixelBox.data);

	memset(buffer, 0, maskPitch * texHeight);

	size_t minRowSpan = std::min(maskPitch, srcPixels.rowPitch);
	size_t minHeight = std::min(texHeight, (unsigned short)srcPixels.getHeight());

	if(maskTexDepth == 1)
	{
		for(unsigned int row = 0; row < minHeight; row++)
			memcpy(buffer + row * maskPitch, (unsigned char*)srcPixels.data + row * srcPixels.rowPitch, minRowSpan);

		memcpy(maskCache, buffer, maskPitch*texHeight);
	}
	else if(maskTexDepth == 4)
	{
		size_t destRowOffset, srcRowOffset, cacheRowOffset;

		for(unsigned int row = 0; row < minHeight; row++)
		{
			destRowOffset = row * maskPitch * maskTexDepth;
			srcRowOffset = row * srcPixels.rowPitch;
			cacheRowOffset = row * maskPitch;

			for(unsigned int col = 0; col < minRowSpan; col++)
				maskCache[cacheRowOffset + col] = buffer[destRowOffset + col * maskTexDepth + 3] = ((unsigned char*)srcPixels.data)[srcRowOffset + col];
		}
	}
	else
	{
		OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED, 
			"Unexpected depth and format were encountered while creating a PF_BYTE_A HardwarePixelBuffer. Pixel format: " + 
			StringConverter::toString(pixelBox.format) + ", Depth:" + StringConverter::toString(maskTexDepth), "Navi::setMask");
	}

	pixelBuffer->unlock();

	if(conversionBuf)
		delete[] conversionBuf;

	maskTexUnit->setTextureName(naviName + "MaskTexture");
	usingMask = true;
}

void Navi::setMaxUPS(unsigned int maxUPS)
{
	maxUpdatePS = maxUPS;
}

void Navi::setMovable(bool isMovable)
{
	if(!isMaterial)
		movable = isMovable;
}

void Navi::setOpacity(float opacity)
{
	limit<float>(opacity, 0, 1);
	
	this->opacity = opacity;
}

void Navi::setPosition(const NaviPosition &naviPosition)
{
	if(isMaterial)
		return;

	position = naviPosition;
	resetPosition();
}

void Navi::resetPosition()
{
	if(isMaterial || !overlay || !panel) return;

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

	return;
}

void Navi::hide(bool fade, unsigned short fadeDurationMS)
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
}

void Navi::show(bool fade, unsigned short fadeDurationMS)
{
	if(fadingIn || fadingOut)
		fadingInStart = fadingInEnd = fadingOutStart = fadingOutEnd = fadingIn = fadingOut = 0;

	if(fade)
	{
		fadingInStart = timer.getMilliseconds();
		fadingInEnd = timer.getMilliseconds() + fadeDurationMS + 1;
		fadingIn = true;
	}

	isVisible = true;
	if(!isMaterial) overlay->show();
}

void Navi::focus()
{
	if(NaviManager::GetPointer() && !isMaterial)
		NaviManager::GetPointer()->focusNavi(0, 0, this);
	//else
	//	browserWin->focus();
}

void Navi::moveNavi(int deltaX, int deltaY)
{
	if(!isMaterial)
		panel->setPosition(panel->getLeft()+deltaX, panel->getTop()+deltaY);
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
	webView->injectMouseMove(xPos, yPos);
}

void Navi::injectMouseWheel(int relScroll)
{
	webView->injectMouseWheel(relScroll);
}

void Navi::injectMouseDown(int xPos, int yPos)
{
	webView->injectMouseDown(Awesomium::LEFT_MOUSE_BTN);
}

void Navi::injectMouseUp(int xPos, int yPos)
{
	webView->injectMouseUp(Awesomium::LEFT_MOUSE_BTN);
}

void Navi::onBeginNavigation(const std::string& url)
{
}

void Navi::onBeginLoading()
{
}

void Navi::onFinishLoading()
{
}

void Navi::onCallback(const std::string& name, const Awesomium::JSArguments& args)
{
	std::map<std::string, NaviDelegate>::iterator i = delegateMap.find(name);

	if(i != delegateMap.end())
		i->second(args);
}

void Navi::onReceiveTitle(const std::wstring& title)
{
}

void Navi::onChangeCursor(Awesomium::WebCursor cursor)
{
}