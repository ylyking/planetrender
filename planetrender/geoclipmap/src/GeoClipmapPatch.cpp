#include <OgreVector2.h>
#include <OgreStringConverter.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgreTextureUnitState.h>
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
		itBlk = nextBlock(itBlk);
	}

	if (lodLvl > 0) return itBlk;

	int m = (m_Parent.getN() + 1) / 4;
	float ml = m-1;
	int cl = m_Parent.getClipmapSize();
	
	float maxCoord = std::max(Math::Abs(m_ViewPosList[0].x), Math::Abs(m_ViewPosList[0].y));
	float half_nl = (m_Parent.getN() - 1) / 2.0;

	// if lodlvl = 0. fill the whole clipmap space
	// there are k rings, this should be optimize later
	//int k = Math::Ceil(m_Parent.getClipmapSize() / (m-1));
	//k = 2;
	int k = Math::Ceil((cl / 2.0 + maxCoord - half_nl) / ml);
	//return itBlk;
	for(int i = 0; i < k; i++) {
		// fill outwards
		float initPos = -half_nl - 0.5 * ml - i * ml;
		// fill by mirroring
		float x = initPos;
		float y = initPos;

		// 3xM
		itBlk = getBlock(itBlk);
		(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_3XM);
		(*itBlk)->m_Pos = Vector2(0, initPos);
		(*itBlk)->m_LodLvl = lodLvl;
		itBlk = nextBlock(itBlk);

		itBlk = getBlock(itBlk);
		(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_3XM);
		(*itBlk)->m_Pos = Vector2(0, -initPos);
		(*itBlk)->m_LodLvl = lodLvl;
		itBlk = nextBlock(itBlk);

		// Mx3
		itBlk = getBlock(itBlk);
		(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_MX3);
		(*itBlk)->m_Pos = Vector2(initPos, 0);
		(*itBlk)->m_LodLvl = lodLvl;
		itBlk = nextBlock(itBlk);

		itBlk = getBlock(itBlk);
		(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_MX3);
		(*itBlk)->m_Pos = Vector2(-initPos, 0);
		(*itBlk)->m_LodLvl = lodLvl;
		itBlk = nextBlock(itBlk);

		for(int j = 0; j < 2 + k; j++) {
			// MxM
			itBlk = getBlock(itBlk);
			(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_MXM);
			(*itBlk)->m_Pos = Vector2(x, y);
			(*itBlk)->m_LodLvl = lodLvl;
			itBlk = nextBlock(itBlk);

			itBlk = getBlock(itBlk);
			(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_MXM);
			(*itBlk)->m_Pos = Vector2(-x, y);
			(*itBlk)->m_LodLvl = lodLvl;
			itBlk = nextBlock(itBlk);

			itBlk = getBlock(itBlk);
			(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_MXM);
			(*itBlk)->m_Pos = Vector2(-x, -y);
			(*itBlk)->m_LodLvl = lodLvl;
			itBlk = nextBlock(itBlk);

			itBlk = getBlock(itBlk);
			(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_MXM);
			(*itBlk)->m_Pos = Vector2(x, -y);
			(*itBlk)->m_LodLvl = lodLvl;
			itBlk = nextBlock(itBlk);

			if (j > 0) {
				itBlk = getBlock(itBlk);
				(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_MXM);
				(*itBlk)->m_Pos = Vector2(y, x);
				(*itBlk)->m_LodLvl = lodLvl;
				itBlk = nextBlock(itBlk);

				itBlk = getBlock(itBlk);
				(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_MXM);
				(*itBlk)->m_Pos = Vector2(-y, x);
				(*itBlk)->m_LodLvl = lodLvl;
				itBlk = nextBlock(itBlk);

				itBlk = getBlock(itBlk);
				(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_MXM);
				(*itBlk)->m_Pos = Vector2(-y, -x);
				(*itBlk)->m_LodLvl = lodLvl;
				itBlk = nextBlock(itBlk);

				itBlk = getBlock(itBlk);
				(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_MXM);
				(*itBlk)->m_Pos = Vector2(y, -x);
				(*itBlk)->m_LodLvl = lodLvl;
				itBlk = nextBlock(itBlk);
			}
			x += ml;
		}
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
	itBlk = nextBlock(itBlk);

	itBlk = getBlock(itBlk);
	(*itBlk)->m_MeshName = m_Parent.getMeshName(GeoClipmapCube::GCM_MESH_LX2);
	(*itBlk)->m_Pos = Vector2(0, Math::Sign(m_ViewPosList[lodLvl].y - m_ViewPosList[lodLvl + 1].y) * pos);
	(*itBlk)->m_LodLvl = lodLvl;
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
		itBlk = nextBlock(itBlk);
	}

	return itBlk; // return the ptr to next free block
}

GeoClipmapPatch::BlockList::iterator GeoClipmapPatch::nextBlock(BlockList::iterator usedBlockPtr)
{
	// check is the block inside the face, if it is not, recycle it
	Vector2 pos = (*usedBlockPtr)->m_Pos;
	pos *= Math::Pow(2, -(*usedBlockPtr)->m_LodLvl);
	pos += m_ViewPosList[(*usedBlockPtr)->m_LodLvl];
	float maxCoord = std::max(Math::Abs(pos.x), Math::Abs(pos.y));
	
	int m = (m_Parent.getN() + 1) / 4;
	float ml = m-1;
	int cl = m_Parent.getClipmapSize();

	if (maxCoord > (cl / 2.0 + ml))
		return usedBlockPtr;
	else {
		(*usedBlockPtr)->computeTransform();
		return ++usedBlockPtr;
	}
}

void GeoClipmapPatch::_updateRenderQueue(RenderQueue* queue)
{
	if (m_ViewPosListUpdated) {
		m_PatchTxMatList.resize(m_ViewPosList.size());

		for(int lodLvl = 0; lodLvl < (int)m_PatchTxMatList.size(); lodLvl++) {
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
			if (lodLvl > 0)
				m_Parent.getClipmap()->updateVisibleArea(lodLvl, m_ViewPosList[lodLvl]);
		}

		BlockList::iterator itBlk =  m_BlockList.begin();
		for(int i = 0; i < (int)m_ViewPosList.size(); i++) {
			itBlk = placeRing(i, itBlk);
			if (i < (int)m_ViewPosList.size() - 1)
				itBlk = placeL(i, itBlk);
			else
				itBlk = placeFinest(i, itBlk);
		}
		// clear unused
		m_BlockList.erase(itBlk, m_BlockList.end());

		m_ViewPosListUpdated = false;
	}
	
	for(BlockList::iterator itBlk = m_BlockList.begin(); itBlk != m_BlockList.end(); itBlk++) {
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
		m_LodMatList.push_back(MaterialManager::getSingleton().create(
			patchNamePrefix + StringConverter::toString(lodLvl),
			ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME));
		Pass* pass = m_LodMatList[lodLvl]->getTechnique(0)->getPass(0);
		pass->setVertexProgram("GeoClipmapVP");
		TextureUnitState* tus = pass->createTextureUnitState(m_Parent.getClipmap()->getLayerTexture(lodLvl)->getName());
		tus->setBindingType(TextureUnitState::BT_VERTEX);
		tus->setTextureFiltering(FO_POINT, FO_POINT,FO_NONE);
		//pass->setFragmentProgram("GeoClipmapFP");
		m_LodMatList[lodLvl]->compile();
	}
}

void GeoClipmapPatch::deleteMat()
{
	for (int lodLvl = 0; lodLvl < m_Parent.getClipmapDepth(); lodLvl++)
	{
		MaterialManager::getSingleton().remove((ResourcePtr)m_LodMatList[lodLvl]);
	}
	m_LodMatList.clear();
}

const MaterialPtr& GeoClipmapPatch::getMat(unsigned int lod) const
{
	return m_LodMatList[lod];
}
