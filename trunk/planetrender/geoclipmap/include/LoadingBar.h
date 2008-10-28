/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2007 The OGRE Team
Also see acknowledgements in Readme.html

You may use this sample code for anything you like, it is not covered by the
LGPL like the rest of the engine.
-----------------------------------------------------------------------------
*/
#ifndef __LoadingBar_h__
#define __LoadingBar_h__

#include <ogre.h>
#include "OgreResourceGroupManager.h"
#include "OgreException.h"
#include "OgreOverlay.h"
#include "OgreOverlayManager.h"
#include "OgreRenderWindow.h"

using namespace Ogre;

class LoadingBar : public ResourceGroupListener
{
protected:
	RenderWindow* mWindow;
	Overlay* mLoadOverlay;
	Real mInitProportion;
	unsigned short mNumGroupsInit;
	unsigned short mNumGroupsLoad;
	Real mProgressBarMaxSize;
	Real mProgressBarScriptSize;
	Real mProgressBarInc;
	OverlayElement* mLoadingBarElement;
	OverlayElement* mLoadingDescriptionElement;
	OverlayElement* mLoadingCommentElement;

public:
	LoadingBar() {}
	virtual ~LoadingBar(){}

	/** Show the loading bar and start listening.
	@param window The window to update
	@param numGroupsInit The number of groups you're going to be initialising
	@param numGroupsLoad The number of groups you're going to be loading
	@param initProportion The proportion of the progress which will be taken
	up by initialisation (ie script parsing etc). Defaults to 0.7 since
	script parsing can often take the majority of the time.
	*/
	virtual void start(RenderWindow* window,
		unsigned short numGroupsInit = 1,
		unsigned short numGroupsLoad = 1,
		Real initProportion = 0.70f)
	{
		mWindow = window;
		mNumGroupsInit = numGroupsInit;
		mNumGroupsLoad = numGroupsLoad;
		mInitProportion = initProportion;
		// We need to pre-initialise the 'Bootstrap' group so we can use
		// the basic contents in the loading screen
		ResourceGroupManager::getSingleton().initialiseResourceGroup("Bootstrap");

		OverlayManager& omgr = OverlayManager::getSingleton();
		mLoadOverlay = (Overlay*)omgr.getByName("Core/LoadOverlay");
		if (!mLoadOverlay)
		{
			OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
				"Cannot find loading overlay", "LoadingBar::start");
		}
		mLoadOverlay->show();

		// Save links to the bar and to the loading text, for updates as we go
		mLoadingBarElement = omgr.getOverlayElement("Core/LoadPanel/Bar/Progress");
		mLoadingCommentElement = omgr.getOverlayElement("Core/LoadPanel/Comment");
		mLoadingDescriptionElement = omgr.getOverlayElement("Core/LoadPanel/Description");

		OverlayElement* barContainer = omgr.getOverlayElement("Core/LoadPanel/Bar");
		mProgressBarMaxSize = barContainer->getWidth();
		mLoadingBarElement->setWidth(0);

		// self is listener
		ResourceGroupManager::getSingleton().addResourceGroupListener(this);



	}

	/** Hide the loading bar and stop listening.
	*/
	virtual void finish(void)
	{
		// hide loading screen
		mLoadOverlay->hide();

		// Unregister listener
		ResourceGroupManager::getSingleton().removeResourceGroupListener(this);

	}


	// ResourceGroupListener callbacks
	void resourceGroupScriptingStarted(const String& groupName, size_t scriptCount)
	{
		assert(mNumGroupsInit > 0 && "You stated you were not going to init "
			"any groups, but you did! Divide by zero would follow...");
		// Lets assume script loading is 70%
		mProgressBarInc = mProgressBarMaxSize * mInitProportion / (Real)scriptCount;
		mProgressBarInc /= mNumGroupsInit;
		mLoadingDescriptionElement->setCaption("Parsing scripts...");
		mWindow->update();
	}
	void scriptParseStarted(const String& scriptName)
	{
		mLoadingCommentElement->setCaption(scriptName);
		mWindow->update();
	}
	void scriptParseEnded(const String& scriptName)
	{
		mLoadingBarElement->setWidth(
			mLoadingBarElement->getWidth() + mProgressBarInc);
		mWindow->update();
	}
	void resourceGroupScriptingEnded(const String& groupName)
	{
	}
	void resourceGroupLoadStarted(const String& groupName, size_t resourceCount)
	{
		assert(mNumGroupsLoad > 0 && "You stated you were not going to load "
			"any groups, but you did! Divide by zero would follow...");
		mProgressBarInc = mProgressBarMaxSize * (1-mInitProportion) /
			(Real)resourceCount;
		mProgressBarInc /= mNumGroupsLoad;
		mLoadingDescriptionElement->setCaption("Loading resources...");
		mWindow->update();
	}
	void resourceLoadStarted(const ResourcePtr& resource)
	{
		mLoadingCommentElement->setCaption(resource->getName());
		mWindow->update();
	}
	void resourceLoadEnded(void)
	{
	}
	void worldGeometryStageStarted(const String& description)
	{
		mLoadingCommentElement->setCaption(description);
		mWindow->update();
	}
	void worldGeometryStageEnded(void)
	{
		mLoadingBarElement->setWidth(
			mLoadingBarElement->getWidth() + mProgressBarInc);
		mWindow->update();
	}
	void resourceGroupLoadEnded(const String& groupName)
	{
	}

};

#endif // #ifndef __LoadingBar_h__
