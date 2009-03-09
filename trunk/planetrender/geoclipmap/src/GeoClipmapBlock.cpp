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
	return m_Mat;
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
	Matrix4 blockTx;
	blockTx.makeTrans(m_Pos.x, m_Pos.y, 0);

	if (m_BasePatch)
		dynamic_cast<const GeoClipmapCube&>(m_parentMovObj).getFaceTransformMatrix(m_parentPatch.getFaceID(), &m_PatchTx);
	else
		m_parentPatch.getWorldTransforms(m_LodLvl, &m_PatchTx);

	Matrix4 patchBlockTx;
	patchBlockTx = m_PatchTx * blockTx;

	m_Tx = m_parentMovObj._getParentNodeFullTransform() * patchBlockTx;

	Vector4 blockPosInCubeSpace4;
	blockPosInCubeSpace4 = patchBlockTx * Vector4(0, 0, 0, 1);
	m_BlockPosInCubeSpace = Vector3(blockPosInCubeSpace4.x, blockPosInCubeSpace4.y, blockPosInCubeSpace4.z);
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
	for (int i = 0; i < 4; i++)
	{
		rsys->addClipPlane(m_parentPatch.getClipPlanes(i));
	}
	return true;
}

void GeoClipmapBlock::_updateCustomGpuParameter(const GpuProgramParameters::AutoConstantEntry &constantEntry, GpuProgramParameters *params) const
{
	Matrix4 mat;
	float vx = 0, vy = 0;
	switch (constantEntry.data)
	{
	case 1://patchTx
		params->_writeRawConstant(constantEntry.physicalIndex, m_PatchTx);
		break;
	case 2://cubeTx
		m_parentMovObj.getParentSceneNode()->getWorldTransforms(&mat);
		params->_writeRawConstant(constantEntry.physicalIndex, mat);
		break;
	case 3://radius
		params->_writeRawConstant(constantEntry.physicalIndex, reinterpret_cast<const GeoClipmapCube&>(m_parentMovObj).getRadius());
		break;
	case 4://nl
		if (m_LodLvl == 0)
			params->_writeRawConstant(constantEntry.physicalIndex, (float)reinterpret_cast<const GeoClipmapCube&>(m_parentMovObj).getClipmapSize() + 1);
		else
			params->_writeRawConstant(constantEntry.physicalIndex, (float)reinterpret_cast<const GeoClipmapCube&>(m_parentMovObj).getN());
		break;
	case 5://blockPos float4(blockPosLod1++, blockPos0);
		if (m_LodLvl == 0) {
			vx = m_parentPatch.getViewPosList()[0].x;
			vy = m_parentPatch.getViewPosList()[0].y;
		}
		if (m_BasePatch)
			params->_writeRawConstant(constantEntry.physicalIndex, Vector4::ZERO);
		else
			params->_writeRawConstant(constantEntry.physicalIndex, Vector4(m_Pos.x, m_Pos.y, vx, vy));
		break;
	case 6:// clipCenter, Vector4(enable, clip center pos, clip size);
		if (m_BasePatch)
			params->_writeRawConstant(constantEntry.physicalIndex, Vector4(m_parentPatch.getViewPosList().size() - 1, m_parentPatch.getViewPosList()[0].x, m_parentPatch.getViewPosList()[0].y, reinterpret_cast<const GeoClipmapCube&>(m_parentMovObj).getN() / 2 / 2));
		else
			params->_writeRawConstant(constantEntry.physicalIndex, Vector4(0, 0, 0, 0));
		break;
	default:
		Renderable::_updateCustomGpuParameter(constantEntry, params);
	}
}
