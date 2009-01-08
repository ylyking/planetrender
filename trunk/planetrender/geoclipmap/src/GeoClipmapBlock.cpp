#include "GeoClipmapBlock.h"
#include "GeoClipmapPatch.h"
#include "GeoClipmapCube.h"

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
	return m_parentPatch.getMat(m_LodLvl);
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
	m_parentPatch.getWorldTransforms(m_LodLvl, &m_PatchTx);
	m_BlockTx.makeTrans(m_Pos.x, m_Pos.y, 0);
	m_Tx = m_parentMovObj._getParentNodeFullTransform() * m_PatchTx * m_BlockTx;
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

void GeoClipmapBlock::_updateCustomGpuParameter(const GpuProgramParameters::AutoConstantEntry &constantEntry, GpuProgramParameters *params) const
{
	Matrix4 mat;
	switch (constantEntry.data)
	{
	case 0:
		params->_writeRawConstant(constantEntry.physicalIndex, m_BlockTx);
		break;
	case 1:
		params->_writeRawConstant(constantEntry.physicalIndex, m_PatchTx);
		break;
	case 2:
		m_parentMovObj.getParentSceneNode()->getWorldTransforms(&mat);
		params->_writeRawConstant(constantEntry.physicalIndex, mat);
		break;
	case 3:
		params->_writeRawConstant(constantEntry.physicalIndex, reinterpret_cast<const GeoClipmapCube&>(m_parentMovObj).getRadius());
		break;
	default:
		Renderable::_updateCustomGpuParameter(constantEntry, params);
	}
}
