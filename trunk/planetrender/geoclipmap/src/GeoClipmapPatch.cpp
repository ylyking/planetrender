#include <OgreMath.h>
#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreManualObject.h>
#include <OgreMeshManager.h>
#include <OgreStringConverter.h>
#include "GeoClipmapBlock.h"
#include "GeoClipmapPatch.h"
#include <OgreWireBoundingBox.h> 

using namespace Ogre;

GeoClipmapPatch::GeoClipmapPatch(Real width, Real height, Camera* cam, const GeoClipmapStack::TextureNameList& texNames, int n) : mWidth(width), mHeight(height),
	mAABB(-width / 2, 0, -height / 2, width / 2, 100, height / 2), mCam(cam), mScale(width / ((float) n - 1) , 1, height / ((float) n - 1) ), mN(n)
{
	mCoarsestClipmapSize = 129;
	mM = (n + 1) / 4;
	mScale = width / ((float) mCoarsestClipmapSize - 1);
	mLastUpdatePos[0] = 0xbeef;
	mLastUpdatePos[1] = 0xbeef;
	mLastUpdatePos[2] = 0xbeef;
	mRadius = Math::Sqrt(Math::Sqr(mWidth / 2) + Math::Sqr(mHeight / 2));
	mBlkMeshNamePrefix = "GeoClipmapBlockMesh" + StringConverter::toString((long)this);
	calcBlockOffsets();
	mDataSrc = new GeoClipmapStack(texNames, n);
}

GeoClipmapPatch::~GeoClipmapPatch(void)
{
	for(BlockList::iterator it = mBlocks.begin(); it != mBlocks.end(); it++) {
		delete *it;
	}
	delete mDataSrc;
}

const String& GeoClipmapPatch::getMovableType(void) const
{
	static String movType = "GeoClipmapPatch";
	return movType;
}

const AxisAlignedBox& GeoClipmapPatch::getBoundingBox(void) const
{
	return mAABB;
}

Real GeoClipmapPatch::getBoundingRadius(void) const
{
	return mRadius;
}

void GeoClipmapPatch::calcBlockOffsets()
{
	// first 12 offsets are mxm block, last 4 offsets are mx2 block
	const int m_1 = mM - 1;
	const Real MxMWidth = m_1;
	const Real Mx3MWidth = 2.0;
	const Real outer_x = (1 + 1.5 * m_1);
	const Real inner_x = (1 + 0.5 * m_1);
	const Real half_x = 0.5;
	// MxM offsets
	mBlockOffsets[0] = Vector3(-outer_x, 0, -outer_x);
	mBlockOffsets[1] = Vector3(-outer_x, 0, outer_x);
	mBlockOffsets[2] = Vector3(outer_x,  0, -outer_x);
	mBlockOffsets[3] = Vector3(outer_x,  0, outer_x);
	mBlockOffsets[4] = Vector3(-outer_x, 0, -inner_x);
	mBlockOffsets[5] = Vector3(-outer_x, 0, inner_x);
	mBlockOffsets[6] = Vector3(outer_x,  0, -inner_x);
	mBlockOffsets[7] = Vector3(outer_x,  0, inner_x);
	mBlockOffsets[8] = Vector3(-inner_x, 0, -outer_x);
	mBlockOffsets[9] = Vector3(-inner_x, 0,  outer_x);
	mBlockOffsets[10] = Vector3(inner_x,  0, -outer_x);
	mBlockOffsets[11] = Vector3(inner_x,  0,  outer_x);
	// Mx3 offsets
	mBlockOffsets[12] = Vector3(-outer_x, 0, 0);
	mBlockOffsets[13] = Vector3(outer_x,  0, 0);
	mBlockOffsets[14] = Vector3(0,  0, -outer_x);
	mBlockOffsets[15] = Vector3(0,  0,  outer_x);
	// MxM central offset
	mBlockOffsets[16] = Vector3(-inner_x, 0, -inner_x);
	mBlockOffsets[17] = Vector3(-inner_x,  0, inner_x);
	mBlockOffsets[18] = Vector3(inner_x,  0, -inner_x);
	mBlockOffsets[19] = Vector3(inner_x,  0,  inner_x);
	// Inner central offset
	mBlockOffsets[20] = Vector3(-half_x, 0, 0);
	mBlockOffsets[21] = Vector3(half_x,  0, 0);
	mBlockOffsets[22] = Vector3(0,  0, -half_x);
	mBlockOffsets[23] = Vector3(0,  0,  half_x);
	// T crack filler offset
	mBlockOffsets[24] = Vector3(0, 0, 2 * m_1 + 1);
	mBlockOffsets[25] = Vector3(0, 0, -(2 * m_1 + 1));
	mBlockOffsets[26] = Vector3(2 * m_1 + 1, 0, 0);
	mBlockOffsets[27] = Vector3(-(2 * m_1 + 1), 0, 0);
}

template<typename T, typename T1, typename T2>
T clampa(T i, T1 min, T2 max)
{
	if (i < min)
		i = min;
	else if (i > max)
		i = max;
	return i;
}

Vector3 GeoClipmapPatch::getLocalCamPos()
{
	// calc the camera pos in patch local space(texture space coord)
	Vector3 vCamWorldPos = mCam->getDerivedPosition();
	//vCamWorldPos = Vector3(0, 0, 0);
	Matrix4 matPatchTransformMatrix, matScale = Matrix4::IDENTITY;
	getParentNode()->getWorldTransforms(&matPatchTransformMatrix);
	matScale.setScale(mScale);
	matPatchTransformMatrix = matPatchTransformMatrix * matScale;
	vCamWorldPos = matPatchTransformMatrix.inverse() * vCamWorldPos;
	vCamWorldPos.x = clampa(vCamWorldPos.x, -(mCoarsestClipmapSize / 2), mCoarsestClipmapSize / 2);
	vCamWorldPos.z = clampa(vCamWorldPos.z, -(mCoarsestClipmapSize / 2), mCoarsestClipmapSize / 2);
	return vCamWorldPos;
}

double round(double x)
{
	if(x>=0.5){return ceil(x);}else{return floor(x);}
}

void GeoClipmapPatch::update()
{
	Vector3 vCamPatchPos = getLocalCamPos();
	

	// determind the lod lvl
	int maxLodLvl = 3;
	int lodLvl = maxLodLvl;//(100 - vCamPatchPos.y) / 5;
	if (lodLvl < 0) lodLvl = 0;
	if (lodLvl > maxLodLvl) lodLvl = maxLodLvl;
	
	vCamPatchPos.y = 0;


	// start placing the blocks from coarse to fine
	Real activeRes = 1;
	// max view distance = finest resolution * 2^depth
	BlockList::iterator it = mBlocks.begin();

	Vector3 vLvlOrigin; // middle of the grid
	// align the origin to the active lvl texture resolution
	vLvlOrigin.x = (int)vCamPatchPos.x;
	vLvlOrigin.y = 0;
	vLvlOrigin.z = (int)vCamPatchPos.z;

	// is update really needed?
	//if (mLastUpdatePos[0] == vLvlOrigin.x && mLastUpdatePos[2] == vLvlOrigin.z && mLastUpdatePos[1] == lodLvl)
	//	return;
	mLastUpdatePos[0] = vLvlOrigin.x;
	mLastUpdatePos[1] = lodLvl;
	mLastUpdatePos[2] = vLvlOrigin.z;

	Vector2 coarseOffset;

	bool bFill = true;
	// create the rings from coarse to fine
	for(int lvl = 0; lvl <= lodLvl; lvl++) {	
		Vector2 vLvlOrigin2D = Vector2(vLvlOrigin.x, vLvlOrigin.z);
		// calc the size of patch in coarsest patch local space
		Vector2 vMinPatchCoord = -(mN - 1) / 2.0 * activeRes + vLvlOrigin2D;
		Vector2 vMaxPatchCoord =  (mN - 1) / 2.0 * activeRes + vLvlOrigin2D;
		// clip the coord in coarsest patch local space
		if (vMinPatchCoord.x < -(mCoarsestClipmapSize-1) / 2.0)
			vMinPatchCoord.x = -(mCoarsestClipmapSize-1) / 2.0;
		if (vMinPatchCoord.y < -(mCoarsestClipmapSize-1) / 2.0)
			vMinPatchCoord.y = -(mCoarsestClipmapSize-1) / 2.0;
		if (vMaxPatchCoord.x > (mCoarsestClipmapSize-1) / 2.0)
			vMaxPatchCoord.x = (mCoarsestClipmapSize-1) / 2.0;
		if (vMaxPatchCoord.y > (mCoarsestClipmapSize-1) / 2.0)
			vMaxPatchCoord.y = (mCoarsestClipmapSize-1) / 2.0;
		// scale the coarsest patch local space into correct lvl local space
		int texSize = pow(2.0, lvl) * mCoarsestClipmapSize - pow(2.0, lvl) + 1;
		Vector2 vMinMapCoord = (vMinPatchCoord + (mCoarsestClipmapSize-1) / 2.0) / (mCoarsestClipmapSize-1) * (texSize-1);//(vMinPatchCoord + mN / 2.0) / activeRes;
		Vector2 vMaxMapCoord = (vMaxPatchCoord + (mCoarsestClipmapSize-1) / 2.0) / (mCoarsestClipmapSize-1) * (texSize-1);//(vMaxPatchCoord + mN / 2.0) / activeRes;
		//Vector2 vMinMapCoord = (vMinPatchCoord + mN / 2.0) / activeRes;
		//Vector2 vMaxMapCoord = (vMaxPatchCoord + mN / 2.0) / activeRes;

		// calc the visible part of texture buffer
		Vector2 vMinTexCoord = (vMinPatchCoord - vLvlOrigin2D) / activeRes + (mN - 1) / 2.0;
		Vector2 vMaxTexCoord = (vMaxPatchCoord - vLvlOrigin2D) / activeRes + (mN - 1) / 2.0;

		//if (lvl >= 0) {
		//	mDataSrc->updateDepth(lvl, Rect(vMinTexCoord.x, vMinTexCoord.y, vMaxTexCoord.x, vMaxTexCoord.y),
		//		Rect(vMinMapCoord.x, vMinMapCoord.y, vMaxMapCoord.x, vMaxMapCoord.y));
		//}

		// fill the coarsest level
		if (bFill) {
			// fill in -x to +x
			/*Vector3 vx = vLvlOrigin;
			vx.x -= mN / 2.0 - (mM - 1) * 0.5;
			while (vx.x > -mN / 2.0) {
				// fill in -z to + z
				if (it == mBlocks.end()) {
					it = mBlocks.insert(it, new GeoClipmapBlock(this));
				}
				(*it)->update(vx, Vector3(activeRes, 1, activeRes), vx - vLvlOrigin, lvl, GCM_BLK_MXM, coarseOffset);
				if ((*it)->isInsidePatch(mAABB))
					it++;
				vx.x -= (mM - 1);
			}*/
		}

		// place the t boundary crack filler
		for(int blockID = 0; blockID < 4; blockID++) {
			if (it == mBlocks.end()) {
				it = mBlocks.insert(it, new GeoClipmapBlock(this));
			}
			BlockType bt;
			if (blockID < 2) {
				bt = GCM_BLK_TFILLER_HORI;
			} else if (blockID < 42) {
				bt = GCM_BLK_TFILLER_VERT;
			}
			(*it)->update(mBlockOffsets[24 + blockID] * activeRes + vLvlOrigin, Vector3(activeRes, 1, activeRes), mBlockOffsets[24 + blockID], lvl, bt, coarseOffset);
			if ((*it)->isInsidePatch(mAABB))
				it++;
		}
		// place the rings around the origin
		for(int blockID = 0; blockID < 16; blockID++) {
			if (it == mBlocks.end()) {
				it = mBlocks.insert(it, new GeoClipmapBlock(this));
			}
			BlockType bt;
			if (blockID < 12) { // MxM
				bt = GCM_BLK_MXM;
			} else if (blockID < 14) { // Mx3
				bt = GCM_BLK_MX3;
			} else { // 3xM
				bt = GCM_BLK_3XM;
			}
			(*it)->update(mBlockOffsets[blockID] * activeRes + vLvlOrigin, Vector3(activeRes, 1, activeRes), mBlockOffsets[blockID], lvl, bt, coarseOffset);
			if ((*it)->isInsidePatch(mAABB))
				it++;
		}
		if (lvl == lodLvl) // current max lvl, create core blocks then quit
		{
			// create the central block
			for(int blockID = 0; blockID < 8; blockID++) {
				if (it == mBlocks.end()) {
					it = mBlocks.insert(it, new GeoClipmapBlock(this));
				}
				BlockType bt;
				if (blockID < 4) {
					bt = GCM_BLK_MXM;
				} else if (blockID < 6) {
					bt = GCM_BLK_INT_VERT;
				} else {
					bt = GCM_BLK_INT_HORI;
				}
				(*it)->update(mBlockOffsets[blockID + 16] * activeRes + vLvlOrigin, Vector3(activeRes, 1, activeRes), mBlockOffsets[blockID + 16], lvl, bt, coarseOffset);
				if ((*it)->isInsidePatch(mAABB))
					it++;
			}
			continue;
		}
			
		// place the L shape blocks and calc the correct origin for next finer lvl
		Vector3 vOrgDelta = vCamPatchPos - vLvlOrigin;
		int XSign = 1, YSign = 1;
		// compare the origin in finer lvl and current lvl, place the L shape blocks accordingly
		if (vOrgDelta.x < 0)
			XSign = -1;
		if (vOrgDelta.z < 0)
			YSign = -1;

		Vector3 Loffsets[2];
		Loffsets[0] = -Vector3(XSign * (mM - 0.5), 0, 0);
		Loffsets[1] = -Vector3(0, 0, YSign * (mM - 0.5));
		// place L
		for(int blockID = 0; blockID < 2; blockID++) {
			if (it == mBlocks.end()) {
				it = mBlocks.insert(it, new GeoClipmapBlock(this));
			}
			BlockType bt;
			if (blockID == 0)
				bt = GCM_BLK_INT_VERT;
			else
				bt = GCM_BLK_INT_HORI;
			(*it)->update(Loffsets[blockID] * activeRes + vLvlOrigin, Vector3(activeRes, 1, activeRes), Loffsets[blockID], lvl, bt, coarseOffset);
			if ((*it)->isInsidePatch(mAABB))
				it++;
		}
		
		activeRes /= 2;
		coarseOffset = Vector2(XSign * activeRes, YSign * activeRes);
		vLvlOrigin += Vector3(coarseOffset.x, 0, coarseOffset.y);
	}

	// free unused blocks
	for(BlockList::iterator itStartToFree = it; itStartToFree != mBlocks.end(); itStartToFree++) {
		delete *itStartToFree;
	}
	mBlocks.erase(it, mBlocks.end());
}

void GeoClipmapPatch::_updateRenderQueue(RenderQueue* queue)
{
	update();

	for(BlockList::iterator it = mBlocks.begin(); it != mBlocks.end(); it++) {
		if (mCam->isVisible((*it)->getWorldAABB()))
			queue->addRenderable(*it);
	}
}

void GeoClipmapPatch::_notifyAttached(Node* parent, bool isTagPoint)
{
	if (parent == 0) {
		// detach, free the memory and destory the mesh
		for(BlockList::iterator it = mBlocks.begin(); it != mBlocks.end(); it++) {
			delete *it;
		}
		mBlocks.clear();
		removeBlockMeshes();
	} else {
		// attached, create the mesh and update the patches transformation
		createBlockMeshes(reinterpret_cast<SceneNode*>(parent)->getCreator()); // ** assume no one attach it to a tagpoint
		_notifyMoved();
	}
	MovableObject::_notifyAttached(parent, isTagPoint);
}

void GeoClipmapPatch::_notifyMoved(void)
{
	// no need to update any tx matrices
	// since the world matrix is accessed separate
	// update the clip plane coeff only
	Plane p[4];
	p[0] = Plane(1, 0, 0, mN / 2.0);
	p[1] = Plane(-1, 0, 0, mN / 2.0);
	p[2] = Plane(0, 0, 1, mN / 2.0);
	p[3] = Plane(0, 0, -1, mN / 2.0);
	Matrix4 matScale(Matrix4::IDENTITY);
	matScale.setScale(mScale);
	for (int i = 0; i < 4; i++)
	{
		p[i] = _getParentNodeFullTransform() * matScale * p[i];
		mClipPlanes[i][0] = p[i].normal.x;
		mClipPlanes[i][1] = p[i].normal.y;
		mClipPlanes[i][2] = p[i].normal.z;
		mClipPlanes[i][3] = p[i].d;
	}
}

void GeoClipmapPatch::createGrid(SceneManager* sceneMgr, BlockType bt, int VertexCountX, int VertexCountY)
{
	ManualObject* manual = sceneMgr->createManualObject("Geoclipmap");

	// Use identity view/projection matrices
	manual->setUseIdentityProjection(true);
	manual->setUseIdentityView(true);

	// calc how many vertex and idx are needed
	int SegX = VertexCountX - 1, SegY = VertexCountY - 1;
	int BlkMeshWidth = SegX, BlkMeshHeight = SegY; // let them be equal, this saves lots of crazy maths
	int VertexCount = VertexCountX * VertexCountY;
	int RectCount = SegX * SegY;
	int IdxCount = RectCount * 2 * 3;

	manual->estimateVertexCount(VertexCount);
	manual->estimateIndexCount(IdxCount);

	manual->begin("Geoclipmap", RenderOperation::OT_TRIANGLE_LIST);

	// gen vertex
	Vector3 vStart(-BlkMeshWidth / 2.0, 0, -BlkMeshHeight / 2.0);
	mMeshAABBs[bt].setMinimum(-BlkMeshWidth / 2.0, mAABB.getMinimum().y, -BlkMeshHeight / 2.0);
	mMeshAABBs[bt].setMaximum(BlkMeshWidth / 2.0, mAABB.getMaximum().y, BlkMeshHeight / 2.0);

	Vector3 vDeltaX((Real)BlkMeshWidth / SegX, 0, 0);
	Vector3 vDeltaY(0, 0, (Real)BlkMeshHeight / SegY);

	Vector3 v(vStart);

	for(int y = 0; y < VertexCountY; y++)
	{
		for(int x = 0; x < VertexCountX; x++)
		{
			manual->position(v);
			v += vDeltaX;
		}
		v.x = vStart.x;
		v += vDeltaY;
	}
	

	// gen idx
	for(int y = 0; y < SegY; y++)
	{
		for(int x = y * VertexCountX; x < y * VertexCountX + SegX; x++)
		{	
			manual->index(x);
			manual->index(x + VertexCountX);
			manual->index(x + VertexCountX + 1);
			manual->index(x);
			manual->index(x + VertexCountX + 1);
			manual->index(x + 1);
		}
	}
	
	manual->end();
	manual->convertToMesh(getMeshName(bt));

	sceneMgr->destroyManualObject(manual);
}

void GeoClipmapPatch::createBlockMeshes(SceneManager* sceneMgr)
{
	createGrid(sceneMgr, GCM_BLK_MXM, mM, mM);
	createGrid(sceneMgr, GCM_BLK_MX3, mM, 3);
	createGrid(sceneMgr, GCM_BLK_3XM, 3, mM);
	createGrid(sceneMgr, GCM_BLK_INT_HORI, 2 * mM + 1, 2);
	createGrid(sceneMgr, GCM_BLK_INT_VERT, 2, 2 * mM + 1);
	createTFillingGrid(sceneMgr, GCM_BLK_TFILLER_HORI, 2 * mM);
	createTFillingGrid(sceneMgr, GCM_BLK_TFILLER_VERT, 2 * mM, true);
}

void GeoClipmapPatch::removeBlockMeshes()
{
	for(int i = 0; i < 7; i++)
		MeshManager::getSingleton().remove(getMeshName((BlockType)i));
}

const String& Ogre::GeoClipmapPatch::getMeshName(BlockType bt) const
{
	static const String sBlkMxMSuffix = "MxM";
	static const String sBlkMx3Suffix = "Mx3";
	static const String sBlkInnerSuffix  = "Inner";
	static const String sBlk3xMSuffix = "3xM";
	static const String sBlkInnerRSuffix  = "InnerR";
	static const String sBlkTFillerSuffix  = "TFiller";
	static const String sBlkTFillerRSuffix  = "TFillerR";
	static String names[7] = {mBlkMeshNamePrefix + sBlkMxMSuffix, mBlkMeshNamePrefix + sBlkMx3Suffix, mBlkMeshNamePrefix + sBlk3xMSuffix, mBlkMeshNamePrefix + sBlkInnerSuffix, mBlkMeshNamePrefix + sBlkInnerRSuffix,
		mBlkMeshNamePrefix + sBlkTFillerSuffix, mBlkMeshNamePrefix + sBlkTFillerRSuffix};
	return names[bt];
}

void Ogre::GeoClipmapPatch::getPlaneCoeff(int i, float* pBuf) const
{
	if (i < 0 || i > 3)
		return;
	for(int j = 0; j < 4; j++)
		*(pBuf + j) = mClipPlanes[i][j];
}

void GeoClipmapPatch::createTFillingGrid(SceneManager* sceneMgr, BlockType bt, int coarserVertexCount, bool transpose)
{
	ManualObject* manual = sceneMgr->createManualObject("Geoclipmap");

	// Use identity view/projection matrices
	manual->setUseIdentityProjection(true);
	manual->setUseIdentityView(true);

	// calc how many vertex and idx are needed
	int SegX = coarserVertexCount - 1;
	int BlkMeshWidth = SegX * 2, BlkMeshHeight = 0; // let them be equal, this saves lots of crazy maths
	int VertexCount = coarserVertexCount + 2 * coarserVertexCount - 1;
	int RectCount = (coarserVertexCount - 1) * 3;
	int IdxCount = RectCount * 3;

	manual->estimateVertexCount(VertexCount);
	manual->estimateIndexCount(IdxCount);

	manual->begin("Geoclipmap", RenderOperation::OT_TRIANGLE_LIST);

	// Set aabb
	Real eps = 1;
	if (transpose) {
		mMeshAABBs[bt].setMinimum(-eps, mAABB.getMinimum().y, -BlkMeshWidth / 2.0);
		mMeshAABBs[bt].setMaximum(eps, mAABB.getMaximum().y, BlkMeshWidth / 2.0);
	} else {
		mMeshAABBs[bt].setMinimum(-BlkMeshWidth / 2.0, mAABB.getMinimum().y, -eps);
		mMeshAABBs[bt].setMaximum(BlkMeshWidth / 2.0, mAABB.getMaximum().y, eps);
	}

	// gen vertex

	Vector3 vStart(-BlkMeshWidth / 2.0, 10, 0);

	Vector3 vDeltaX((Real)BlkMeshWidth / SegX, 0, 0);
	Vector3 v(vStart);

	// gen the coarser vertex first
	for(int x = 0; x < coarserVertexCount; x++)
	{
		if (transpose)
			manual->position(v.z, v.y, v.x);
		else
			manual->position(v);
		v += vDeltaX;
	}

	v = vStart; //v.z = 10;
	vDeltaX = Vector3((Real)BlkMeshWidth / SegX / 2, 0, 0);

	// gen the finer vertex
	for(int x = 0; x < 2 * coarserVertexCount - 1; x++)
	{
		if (transpose)
			manual->position(v.z, v.y, v.x);
		else
			manual->position(v);
		v += vDeltaX;
	}
	
	// gen idx
	for(int x = 0; x < coarserVertexCount - 1; x++)
	{	
		int finerIdx = x * 2 + coarserVertexCount;
		manual->index(x);
		manual->index(finerIdx);
		manual->index(finerIdx + 1);
		manual->index(x);
		manual->index(finerIdx + 1);
		manual->index(x + 1);
		manual->index(x + 1);
		manual->index(finerIdx + 1);
		manual->index(finerIdx + 2);
	}

	manual->end();
	manual->convertToMesh(getMeshName(bt));

	sceneMgr->destroyManualObject(manual);
}

