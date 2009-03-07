/*
-----------------------------------------------------------------------------
Filename:    GeoClipmap.cpp
-----------------------------------------------------------------------------

This source file is generated by the Ogre AppWizard.

Check out: http://conglomerate.berlios.de/wiki/doku.php?id=ogrewizards

Based on the Example Framework for OGRE
(Object-oriented Graphics Rendering Engine)

Copyright (c) 2000-2007 The OGRE Team
For the latest info, see http://www.ogre3d.org/
`
You may use this sample code for anything you like, it is not covered by the
LGPL like the rest of the OGRE engine.
-----------------------------------------------------------------------------
*/
#pragma warning(disable: 4819)

#include "GeoClipmapCube.h"
#include "GeoClipmap.h"
#include "Clipmap.h"

//-------------------------------------------------------------------------------------
GeoClipmapApp::GeoClipmapApp(void)
{
}
//-------------------------------------------------------------------------------------
GeoClipmapApp::~GeoClipmapApp(void)
{
}

//-------------------------------------------------------------------------------------
void GeoClipmapApp::createScene(void)
{
	/*Clipmap* cm = new Clipmap(4, 129, 127);
	cm->addTexture("clipmap_129x129.bmp");
	cm->addTexture("clipmap_257x257.bmp");
	cm->addTexture("clipmap_513x513.bmp");
	cm->addTexture("clipmap_1025x1025.bmp");*/

	GeoClipmapCube* gcmcube = new GeoClipmapCube(100, 0, mSceneMgr, mCamera, 127);
	SceneNode* cubeNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("CubeNode", Vector3(-10, 0, 0));
	cubeNode->attachObject(gcmcube);

	//Entity* ogreHead = mSceneMgr->createEntity("Head", gcmcube->getMeshName(GeoClipmapCube::GCM_MESH_2XL));

	//SceneNode* headNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("HeadNode", Vector3(50, 0, 0));
	//headNode->attachObject(ogreHead);

	// Set ambient light
	mSceneMgr->setAmbientLight(ColourValue(0.5, 0.5, 0.5));

	// Create a light
	Light* l = mSceneMgr->createLight("MainLight");
	l->setPosition(20,80,50);

	//mSceneMgr->showBoundingBoxes(true);
}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
		int main(int argc, char *argv[])
#endif
		{
			// Create application object
			GeoClipmapApp app;

			try {
				app.go();
			} catch( Ogre::Exception& e ) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
				MessageBox( NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
				std::cerr << "An exception has occured: " <<
					e.getFullDescription().c_str() << std::endl;
#endif
			}

			return 0;
		}

#ifdef __cplusplus
	}
#endif
