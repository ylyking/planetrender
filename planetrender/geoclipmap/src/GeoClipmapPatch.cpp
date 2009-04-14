#include <OgreVector2.h>
#include <OgreStringConverter.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgreTextureUnitState.h>
#include <OgreCamera.h>
#include <OgreNode.h>
#include "GeoClipmapCube.h"
#include "GeoClipmapPatch.h"
#include "Clipmap.h"

using namespace Ogre;

GeoClipmapPatch::GeoClipmapPatch(const GeoClipmapCube& parent, int faceID) :
	m_Parent(parent),
	m_FaceID(faceID),
	m_ViewPosListUpdated(true)
{
	createMat();
}

GeoClipmapPatch::~GeoClipmapPatch(void)
{
	deleteMat();
}

GeoClipmapPatch::BlockList::iterator GeoClipmapPatch::getBlock(const BlockList::iterator& freeBlockPtr)
{
	if (freeBlockPtr == m_BlockList.end()) {
		m_BlockList.push_back(new GeoClipmapBlock(*this, m_Parent));
		return --m_BlockList.end();
	}
	return freeBlockPtr;
}

GeoClipmapPatch::BlockList::iterator GeoClipmapPatch::placeRing(int lodLvl, const BlockList::iterator& freeBlockPtr)
{
	assert(lodLvl >= 0 && lodLvl < m_ViewPosList.size());
	BlockList::iterator itBlk = freeBlockPtr;
	for(int i = 0; i < 16; i++) {
		itBlk = getBlock(itBlk);
		if (i < 12)
			(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_MXM);
		else if (i < 14)
			(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_MX3);
		else if (i < 16)
			(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_3XM);
		(*itBlk)->m_Pos = getBlockPos(i);
		(*itBlk)->m_LodLvl = lodLvl;
		(*itBlk)->m_Mat = m_NormalMatList[lodLvl];
		(*itBlk)->m_BasePatch = false;
		itBlk = nextBlock(itBlk);
	}

	int m = (m_Parent.getN() + 1) / 4;

	float ml = m-1;

	// T Fill
	if (lodLvl > 0) {
		itBlk = getBlock(itBlk);
		(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_TFillV);
		(*itBlk)->m_Pos = Vector2(-(2 * ml + 1), 0);
		(*itBlk)->m_LodLvl = lodLvl;
		(*itBlk)->m_Mat = m_TFillMatList[lodLvl];
		(*itBlk)->m_BasePatch = false;
		itBlk = nextBlock(itBlk);

		itBlk = getBlock(itBlk);
		(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_TFillV);
		(*itBlk)->m_Pos = Vector2(2 * ml + 1, 0);
		(*itBlk)->m_LodLvl = lodLvl;
		(*itBlk)->m_Mat = m_TFillMatList[lodLvl];
		(*itBlk)->m_BasePatch = false;
		itBlk = nextBlock(itBlk);

		itBlk = getBlock(itBlk);
		(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_TFillH);
		(*itBlk)->m_Pos = Vector2(0, -(2 * ml + 1));
		(*itBlk)->m_LodLvl = lodLvl;
		(*itBlk)->m_Mat = m_TFillMatList[lodLvl];
		(*itBlk)->m_BasePatch = false;
		itBlk = nextBlock(itBlk);

		itBlk = getBlock(itBlk);
		(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_TFillH);
		(*itBlk)->m_Pos = Vector2(0, 2 * ml + 1);
		(*itBlk)->m_LodLvl = lodLvl;
		(*itBlk)->m_Mat = m_TFillMatList[lodLvl];
		(*itBlk)->m_BasePatch = false;
		itBlk = nextBlock(itBlk);
	}

	return itBlk; // return the ptr to next free block
}

GeoClipmapPatch::BlockList::iterator GeoClipmapPatch::placeL(int lodLvl, const BlockList::iterator& freeBlockPtr)
{
	assert(lodLvl >= 0 && lodLvl < m_ViewPosList.size() - 1);

	static int m = (m_Parent.getN() + 1) / 4;
	static float pos = (m - 0.5);

	BlockList::iterator itBlk = freeBlockPtr;

	itBlk = getBlock(itBlk);
	(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_2XL);
	(*itBlk)->m_Pos = Vector2(Math::Sign(m_ViewPosList[lodLvl].x - m_ViewPosList[lodLvl + 1].x) * pos, 0);
	(*itBlk)->m_LodLvl = lodLvl;
	(*itBlk)->m_Mat = m_NormalMatList[lodLvl];
	(*itBlk)->m_BasePatch = false;
	itBlk = nextBlock(itBlk);

	itBlk = getBlock(itBlk);
	(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_LX2);
	(*itBlk)->m_Pos = Vector2(0, Math::Sign(m_ViewPosList[lodLvl].y - m_ViewPosList[lodLvl + 1].y) * pos);
	(*itBlk)->m_LodLvl = lodLvl;
	(*itBlk)->m_Mat = m_NormalMatList[lodLvl];
	(*itBlk)->m_BasePatch = false;
	itBlk = nextBlock(itBlk);

	return itBlk; // return the ptr to next free block
}

GeoClipmapPatch::BlockList::iterator GeoClipmapPatch::placeFinest(int lodLvl, const BlockList::iterator& freeBlockPtr)
{
	assert(lodLvl == m_ViewPosList.size() - 1);

	static int m = (m_Parent.getN() + 1) / 4;
	static float pos = (m - 0.5);

	BlockList::iterator itBlk = freeBlockPtr;

	for(int i = 16; i < 24; i++) {
		itBlk = getBlock(itBlk);
		if (i < 20)
			(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_MXM);
		else if (i < 22)
			(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_MX3);
		else if (i < 24)
			(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_2XL);
		(*itBlk)->m_Pos = getBlockPos(i);
		(*itBlk)->m_LodLvl = lodLvl;
		(*itBlk)->m_Mat = m_NormalMatList[lodLvl];
		(*itBlk)->m_BasePatch = false;
		itBlk = nextBlock(itBlk);
	}

	return itBlk; // return the ptr to next free block
}

GeoClipmapPatch::BlockList::iterator GeoClipmapPatch::nextBlock(BlockList::iterator usedBlockPtr)
{
	(*usedBlockPtr)->computeTransform();
	// check is the block inside the face, if it is not, recycle it
	Vector3 vCamInCubeSpace = m_Parent.getCamera()->getPosition() - m_Parent.getParentNode()->getPosition();

	// normal direction culling
	if ((*usedBlockPtr)->m_BlockPosInCubeSpace.dotProduct(vCamInCubeSpace) < -1e-3)
		return usedBlockPtr;

	int mLod = 1 << ((*usedBlockPtr)->m_LodLvl);
	// inside plane culling
	Vector2 blockPosInPatchSpace = (*usedBlockPtr)->m_Pos / mLod + m_ViewPosList[(*usedBlockPtr)->m_LodLvl];
	Vector2 blockMinCornerPosInPatchSpace;
	blockMinCornerPosInPatchSpace.x = Math::Abs(blockPosInPatchSpace.x) - m_Parent.getMeshAABB((*usedBlockPtr)->m_MeshName).getMaximum().x / mLod;
	blockMinCornerPosInPatchSpace.y = Math::Abs(blockPosInPatchSpace.y) - m_Parent.getMeshAABB((*usedBlockPtr)->m_MeshName).getMaximum().y / mLod;

	if (blockMinCornerPosInPatchSpace.x > m_Parent.getClipmapSize() / 2.0 ||
		blockMinCornerPosInPatchSpace.y > m_Parent.getClipmapSize() / 2.0)
		return usedBlockPtr;

	return ++usedBlockPtr;
}

void GeoClipmapPatch::_updateRenderQueue(RenderQueue* queue)
{
	if (m_ViewPosListUpdated) {
		m_PatchTxMatList.resize(m_ViewPosList.size());

		for(int lodLvl = 1; lodLvl < (int)m_PatchTxMatList.size(); lodLvl++) {
			// room for optimization, store the matRing
			// translate the ring and scale them
			Matrix4 matRing;
			matRing.makeTrans(m_ViewPosList[lodLvl].x, m_ViewPosList[lodLvl].y, 0);
			matRing.setScale(Vector3(Math::Pow(2, -lodLvl) , Math::Pow(2, -lodLvl) , 1));

			Matrix4 faceTx;
			// mul it to the parent transform
			m_Parent.getFaceTransformMatrix(m_FaceID, &faceTx);
			m_PatchTxMatList[lodLvl] = faceTx * matRing;
			
			// update texture
			if (lodLvl > 0) {
				m_ViewPosList[lodLvl].y = -m_ViewPosList[lodLvl].y;
				m_Parent.getClipmap(m_FaceID)->updateVisibleArea(lodLvl, m_ViewPosList[lodLvl]);
				m_ViewPosList[lodLvl].y = -m_ViewPosList[lodLvl].y;
			}
		}

		BlockList::iterator itBlk =  m_BlockList.begin();
		for(int lodLvl = 1; lodLvl < (int)m_ViewPosList.size(); lodLvl++) {
			itBlk = placeRing(lodLvl, itBlk);
			if (lodLvl < (int)m_ViewPosList.size() - 1)
				itBlk = placeL(lodLvl, itBlk);
			else
				itBlk = placeFinest(lodLvl, itBlk);
		}
		// clear unused
		m_BlockList.erase(itBlk, m_BlockList.end());

		if (m_BaseBlock.get() == NULL) {
			m_BaseBlock.reset(new GeoClipmapBlock(*this, m_Parent));
			m_BaseBlock->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_BASE);
			m_BaseBlock->m_Pos = Vector2(0, 0);
			m_BaseBlock->m_LodLvl = 0;
			m_BaseBlock->m_Mat = m_NormalMatList[0];
			m_BaseBlock->m_BasePatch = true;
			m_BaseBlock->computeTransform();
		}

		m_ViewPosListUpdated = false;
	}
	
	queue->addRenderable(m_BaseBlock.get());

	for(BlockList::iterator itBlk = m_BlockList.begin(); itBlk != m_BlockList.end(); itBlk++) {
		// frustum culling
		// this implementation is wrong...
		/*Matrix4 mat;
		(*itBlk)->getWorldTransforms(&mat);

		AxisAlignedBox aabb;
		aabb = m_Parent.getMeshAABB((*itBlk)->m_MeshName);
		aabb.transform(mat);

		if (!m_Parent.getCamera()->isVisible(aabb))
		continue;*/

		queue->addRenderable(*itBlk);
	}
}

Vector2 GeoClipmapPatch::getBlockPos(int blockID)  const
{
	static int m = (m_Parent.getN() + 1) / 4;
	
	static float posInner = 1 + (m - 1) / 2.0;
	static float posOuter = 1 + 3 * (m - 1) / 2.0;

	static const int blockCount = 24;

	static Vector2 blockPosList[blockCount] = {
		// MxM blocks
		Vector2(-posOuter, posOuter), // 0
		Vector2(-posInner, posOuter),
		Vector2( posInner, posOuter),
		Vector2( posOuter, posOuter),
		Vector2(-posOuter, posInner),
		Vector2( posOuter, posInner),
		Vector2(-posOuter,-posInner),
		Vector2( posOuter,-posInner),
		Vector2(-posOuter,-posOuter),
		Vector2(-posInner,-posOuter),
		Vector2( posInner,-posOuter),
		Vector2( posOuter,-posOuter), // 11
		// Mx3
		Vector2(-posOuter, 0),
		Vector2( posOuter, 0), // 13
		// 3xM
		Vector2(0,-posOuter),
		Vector2(0, posOuter), // 15
		// Finest
		// MxM
		Vector2(-posInner, posInner),
		Vector2( posInner, posInner),
		Vector2(-posInner, -posInner),
		Vector2( posInner, -posInner), // 19
		// Mx3
		Vector2(-posInner, 0),
		Vector2( posInner, 0), // 21
		// 2XL
		Vector2(-0.5, 0),
		Vector2( 0.5, 0), // 23
	};

	assert(blockID >= 0 && blockID < blockCount);
	return blockPosList[blockID];
}

void GeoClipmapPatch::getWorldTransforms(int lodLvl, Matrix4* mat) const
{
	assert(lodLvl >= 0 && lodLvl < m_ViewPosList.size());
	*mat =  m_PatchTxMatList[lodLvl];
}

void GeoClipmapPatch::createMat()
{
	String patchNamePrefix = StringConverter::toString((long)this) + "_";
	for (int lodLvl = 0; lodLvl < m_Parent.getClipmapDepth(); lodLvl++)
	{
		m_NormalMatList.push_back(MaterialManager::getSingleton().create(
			patchNamePrefix + StringConverter::toString(lodLvl) + "_Normal",
			ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME));			
	
		Pass* pass = m_NormalMatList[lodLvl]->getTechnique(0)->getPass(0);
#if ATMOSPHERE == 1
		pass->setVertexProgram("GeoClipmapWithAtmosphereVP");
		pass->setFragmentProgram("GeoClipmapWithAtmosphereFP");
#else
		pass->setVertexProgram("GeoClipmapVP");
		pass->setFragmentProgram("GeoClipmapFP");
#endif

		TextureUnitState* tus;

		tus = pass->createTextureUnitState("earth_h.bmp");
		//tus = pass->createTextureUnitState("mars_h.bmp");

		tus->setBindingType(TextureUnitState::BT_FRAGMENT);	

		tus = pass->createTextureUnitState(m_Parent.getClipmap(m_FaceID)->getLayerTexture(lodLvl)->getName());
		tus->setBindingType(TextureUnitState::BT_VERTEX);	

#if ATMOSPHERE == 1
		tus = pass->createTextureUnitState(m_Parent.getOpticalDepthTexName());
		tus->setBindingType(TextureUnitState::BT_VERTEX);	
#endif
		

		m_TFillMatList.push_back(m_NormalMatList[lodLvl]->clone
				(patchNamePrefix + StringConverter::toString(lodLvl) + "_TFill")
			);

		m_TFillMatList[lodLvl]->getTechnique(0)->getPass(0)->setCullingMode(CULL_NONE);

		m_TFillMatList[lodLvl]->compile();
		m_NormalMatList[lodLvl]->compile();
	}
}

void GeoClipmapPatch::deleteMat()
{
	for (int lodLvl = 0; lodLvl < m_Parent.getClipmapDepth(); lodLvl++)
	{
		MaterialManager::getSingleton().remove((ResourcePtr)m_NormalMatList[lodLvl]);
	}
	m_NormalMatList.clear();
}
