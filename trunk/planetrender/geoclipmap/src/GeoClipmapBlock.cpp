#include <Ogre.h>
#include "GeoClipmapBlock.h"
#include "GeoClipmapPatch.h"

using namespace Ogre;

GeoClipmapBlock::GeoClipmapBlock(GeoClipmapPatch* patch) : mPatch(patch), mMeshName("ogrehead.mesh"), mDepth(-1)//, mWireBoundingBox(0)
{
}

GeoClipmapBlock::~GeoClipmapBlock(void)
{
}


const MaterialPtr& GeoClipmapBlock::getMaterial(void) const
{
	return mPatch->getDataSource().getMaterial(mDepth);
}

void GeoClipmapBlock::getRenderOperation(RenderOperation &op)
{
	MeshPtr mshBlock = MeshManager::getSingleton().getByName(mMeshName);
	if (mshBlock.isNull())
		OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Could not find mesh " + mMeshName,
					"GeoClipmapBlock::getRenderOperation" );
	mshBlock->getSubMesh(0)->_getRenderOperation(op);
}

void GeoClipmapBlock::getWorldTransforms(Matrix4* xform) const
{
	return mPatch->getParentNode()->getWorldTransforms(xform);
}

Real GeoClipmapBlock::getSquaredViewDepth(const Camera *cam) const
{
	return mPatch->getParentNode()->getSquaredViewDepth(cam);
}

const LightList& GeoClipmapBlock::getLights(void) const
{
	return mPatch->queryLights();
}

void GeoClipmapBlock::update(const Vector3& pos, const Vector3& scale, const Vector3& localBlockPos, int depth, BlockType bt, const Vector2& courserPosOffset)
{
	Matrix4 matPatchScale = Matrix4::IDENTITY, matLocalScale = Matrix4::IDENTITY, matTrans;

	matTrans.makeTrans(pos);
	matPatchScale.setScale(mPatch->getScale());
	matLocalScale.setScale(scale);

	mCachedLocalTransform = matPatchScale * matTrans * matLocalScale;

	mDepth = depth;

	mMeshName = mPatch->getMeshName(bt);

	mLocalAABB = mPatch->getMeshAABB(bt);
	mLocalAABB.transform(mCachedLocalTransform);

	mWorldAABB = mLocalAABB;
	Matrix4 worldTx;
	getWorldTransforms(&worldTx);
	mWorldAABB.transform(worldTx);

	mLocalPos = localBlockPos;
	mLocalPos.w = (mPatch->getN()) / 2.0;

	if (depth == 0) return;
	mCourserPosOffset.x = courserPosOffset.x;
	mCourserPosOffset.y = courserPosOffset.y;
}

void GeoClipmapBlock::_updateCustomGpuParameter(const GpuProgramParameters::AutoConstantEntry &constantEntry, GpuProgramParameters* params) const
{
	switch(constantEntry.data) {
		case 0: // mCachedLocalTransform
			params->_writeRawConstant(constantEntry.physicalIndex, mCachedLocalTransform);
			break;
		case 1: // Local block Pos
			params->_writeRawConstant(constantEntry.physicalIndex, mLocalPos);
			break;
		case 2: // courserPosOffset
			params->_writeRawConstant(constantEntry.physicalIndex, mCourserPosOffset, 2);
			break;
		default:
			Renderable::_updateCustomGpuParameter(constantEntry, params);
	}		
}

void Ogre::GeoClipmapBlock::postRender( SceneManager* sm, RenderSystem* rsys )
{
	rsys->resetClipPlanes();
}

bool Ogre::GeoClipmapBlock::preRender( SceneManager* sm, RenderSystem* rsys )
{
	for (int i = 0; i < 4; i++)
	{
		float p[4];
		mPatch->getPlaneCoeff(i, p);
		rsys->addClipPlane(p[0], p[1], p[2], p[3]);
	}
	return true;
}