/*
-----------------------------------------------------------------------------
Filename:    GeoClipmap.h
-----------------------------------------------------------------------------

This source file is generated by the Ogre AppWizard.

Check out: http://conglomerate.berlios.de/wiki/doku.php?id=ogrewizards

Based on the Example Framework for OGRE
(Object-oriented Graphics Rendering Engine)

Copyright (c) 2000-2007 The OGRE Team
For the latest info, see http://www.ogre3d.org/

You may use this sample code for anything you like, it is not covered by the
LGPL like the rest of the OGRE engine.
-----------------------------------------------------------------------------
*/
#ifndef __GeoClipmap_h_
#define __GeoClipmap_h_

#define CLOCKWISE 0
#define ANTICLOCKWISE 1
#define GEOCLIPMAP 1
#define ATMOSPHERE 1
#define HDR 1

#include "BaseApplication.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include "../res/resource.h"
#endif


class GeoClipmapApp : public BaseApplication
{
public:
	GeoClipmapApp(void);
	virtual ~GeoClipmapApp(void);

protected:
	//virtual void createCamera(void);
	virtual void createScene(void);
	void createOpticalDepthTexture(Real fOuterRadius, Real fInnerRadius, Real fRayleighScaleHeight, Real fMieScaleHeight, int samples, int texWidth, int texHeight, String opticalDepthTexName);
	void createSphere(const std::string& strName, const float r, const int nRings = 16, const int nSegments = 16, int order = ANTICLOCKWISE);
};


#endif // #ifndef __GeoClipmap_h_