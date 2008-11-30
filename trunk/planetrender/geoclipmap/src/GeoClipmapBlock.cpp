#include "GeoClipmapBlock.h"
#include "GeoClipmapPatch.h"

#include <Ogre.h>
using namespace Ogre;

GeoClipmapBlock::GeoClipmapBlock(const GeoClipmapPatch& patch, const MovableObject& movObj) :
	m_parentPatch(patch), m_parentMovObj(movObj)
{

}

GeoClipmapBlock::~GeoClipmapBlock(void)
{
}

const MaterialPtr& GeoClipmapBlock::getMaterial(void) const
{
	//return mPatch->getDataSource().getMaterial(mDepth);
	//return MaterialManager::getSingleton().getByName("NoMaterial");
	return MaterialManager::getSingleton().getByName("NoMaterial");
}

void GeoClipmapBlock::getRenderOperation(RenderOperation &op)
{
	MeshPtr mshBlock = MeshManager::getSingleton().getByName(m_MeshName);
	if (mshBlock.isNull())
		OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Could not find mesh " + m_MeshName,
		"GeoClipmapBlock::getRenderOperation");
	mshBlock->getSubMesh(0)->_getRenderOperation(op);
}

void GeoClipmapBlock::computeTransform()
{
	Matrix4 matPatchTx, matBlockTx;
	m_parentPatch.getWorldTransforms(m_LodLvl, &matPatchTx);
	//matBlockTx.makeTrans(m_parentPatch.getBlockPos(m_PosIdx));
	matBlockTx.makeTrans(m_Pos.x, m_Pos.y, 0);
	m_Tx = matPatchTx * matBlockTx;
}

void GeoClipmapBlock::getWorldTransforms(Matrix4* xform) const
{
	*xform = m_Tx;
}

Real GeoClipmapBlock::getSquaredViewDepth(const Camera *cam) const
{
	return m_parentMovObj.getParentNode()->getSquaredViewDepth(cam);
}

const LightList& GeoClipmapBlock::getLights(void) const
{
	return m_parentMovObj.queryLights();
}

void Ogre::GeoClipmapBlock::postRender(SceneManager* sm, RenderSystem* rsys)
{
	rsys->resetClipPlanes();
}

bool Ogre::GeoClipmapBlock::preRender(SceneManager* sm, RenderSystem* rsys)
{
	//return true;
	for (int i = 0; i < 4; i++)
	{
		rsys->addClipPlane(m_parentPatch.getClipPlanes(i));
	}
	return true;
}
