#pragma once

#include "GeoClipmapBlock.h"
#include <OgreRenderQueue.h>
#include <OgrePlane.h>
#include <list>
#include <vector>

namespace Ogre
{
	class GeoClipmapCube;

	class GeoClipmapPatch
	{
	private:
		//// Variables
		const GeoClipmapCube& m_Parent;
		int m_FaceID;
		std::vector<Vector2> m_ViewPosList; // view posititon for different lod lvls
		std::vector<Matrix4> m_PatchTxMatList; // tx mat for different lod lvls
		bool m_ViewPosListUpdated; // update flag
		std::vector<MaterialPtr> m_NormalMatList, m_TFillMatList;

		// Block pools
		typedef std::list<GeoClipmapBlock*> BlockList;
		BlockList m_BlockList;

		// Clip plane coeff.
		Plane m_ClipPlanes[4];

		//// Methods
		BlockList::iterator getBlock(const BlockList::iterator& freeBlockPtr);
		BlockList::iterator placeRing(int lodLvl, const BlockList::iterator& freeBlockPtr);
		BlockList::iterator placeL(int lodLvl, const BlockList::iterator& freeBlockPtr);
		BlockList::iterator placeFinest(int lodLvl, const BlockList::iterator& freeBlockPtr);
		Vector2 getBlockPos(int blockID) const;
		BlockList::iterator nextBlock(BlockList::iterator usedBlockPtr);
		void createMat();
		void deleteMat();
	public:
		GeoClipmapPatch(const GeoClipmapCube& parent, int faceID);
		~GeoClipmapPatch(void);
		//// getters/ setters
		const std::vector<Vector2>& getViewPosList() const { return m_ViewPosList; }
		void setViewPosList(const std::vector<Vector2>& val) { m_ViewPosListUpdated = true; m_ViewPosList = val; }
		const Ogre::Plane& getClipPlanes(int clipPlaneID) const {
			assert(clipPlaneID >= 0 && clipPlaneID < 4);
			return m_ClipPlanes[clipPlaneID];
		};
		void setClipPlanes(int clipPlaneID, const Ogre::Plane& val) {
			assert(clipPlaneID >= 0 && clipPlaneID < 4);
			m_ClipPlanes[clipPlaneID] = val;
		}

		//// Methods
		void _updateRenderQueue(RenderQueue* queue);
		void getWorldTransforms(int lodLvl, Matrix4* mat) const;
		const MaterialPtr& getMat(unsigned int lod) const;
	};
}
