#pragma once

#include <list>
#include <OgreNode.h>
#include <OgreMovableObject.h>
#include <OgreMath.h>
#include <OgreVector3.h>
#include <OgreQuaternion.h>
#include <OgreString.h>
#include "GeoClipmapStack.h"
#include "GeoClipmapBlock.h"

namespace Ogre
{
	typedef GeoClipmapStack::TextureNameList TextureNameList;
	class GeoClipmapPatch : public MovableObject
	{
	private:
		// Clipmap parameter
		int mM, mN, mCoarsestClipmapSize;
		// Patch size
		Real mWidth, mHeight;
		// Bounding info.
		AxisAlignedBox mAABB;
		Real mRadius;
		// Patch list
		typedef std::list<GeoClipmapBlock*> BlockList;
		BlockList mBlocks;
		// Patch geometry name
		String mBlkMeshNamePrefix;
		// this is the player camera, it should be replaced by a vector3
		Vector3* mViewPos;
		// local transformation
		Vector3 mScale;
		Camera* mCam;
		int mLastUpdatePos[3];
		// pre-computed block offsets
		Vector3 mBlockOffsets[28];
		// AABB of meshes
		AxisAlignedBox mMeshAABBs[7];
		GeoClipmapStack* mDataSrc;
		// Clipplane list
		Real mClipPlanes[4][4];
		// relocate the patches
		void update();
		// init functions
		void createGrid(SceneManager* sceneMgr, BlockType bt, int SegX, int SegY);
		void createBlockMeshes(SceneManager* sceneMgr);
		void createTFillingGrid(SceneManager* sceneMgr, BlockType bt, int coarserVertexCount, bool transpose = false);
		void removeBlockMeshes();
		void calcBlockOffsets();
		Vector3 getLocalCamPos();
	public:
		// block types
		const String& getMeshName(BlockType bt) const;
		inline const AxisAlignedBox& getMeshAABB(BlockType bt) const {
			return mMeshAABBs[bt];
		}
		GeoClipmapPatch(Real width, Real height, Camera* cam, const GeoClipmapStack::TextureNameList& texNames, int n = 15);
		virtual ~GeoClipmapPatch(void);
		// override
		virtual const String& getMovableType(void) const;
		virtual const AxisAlignedBox& getBoundingBox(void) const;
		virtual Real getBoundingRadius(void) const;
		virtual void _updateRenderQueue(RenderQueue* queue);
		virtual void _notifyAttached(Node* parent, bool isTagPoint = false);
		virtual void _notifyMoved(void);
		const Vector3& getScale() const {
			return mScale;
		};
		int getN() const {
			return mN;
		}
		const GeoClipmapStack& getDataSource() {
			return *mDataSrc;
		}
		virtual void visitRenderables(Renderable::Visitor* visitor, 
			bool debugRenderables = false) {
		}
		void getPlaneCoeff(int i, float* pBuf) const;
	};
}
