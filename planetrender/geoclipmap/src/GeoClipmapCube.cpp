#include <OgreSceneManager.h>
#include <OgreManualObject.h>
#include <OgreMesh.h>
#include <OgreMeshManager.h>
#include <OgreCamera.h>
#include "GeoClipmapCube.h"
#include <vector>

using namespace Ogre;

GeoClipmapCube::GeoClipmapCube(float radius, float maxHeight, int n, SceneManager* sceneMgr, Camera* camera) :
	m_N(n),
	m_SceneMgr(sceneMgr),
	m_Camera(camera),
	m_Radius(radius),
	m_MaxHeight(maxHeight),
	m_SemiEdgeLen(radius * Math::Cos(Degree(45))),
	m_AABB(Vector3(-(radius + maxHeight) * Math::Cos(Degree(45))), Vector3((radius + maxHeight) * Math::Cos(Degree(45))))
{
	m_ClipmapSize = 1 * (m_N - 1)  - 10; // 64;//this is wrong, just a replacement value for debug
	// correct value of m_ClipmapSize is a even integer > N
	m_ResNamePrefix = StringConverter::toString(reinterpret_cast<unsigned long>(this)) + "_";

	// create the patches
	memset(m_Patches, 0, sizeof(GeoClipmapPatch*)  * 6);
	for(int i = 0; i < 6; i++)
		m_Patches[i] = new GeoClipmapPatch(*this, i);

	// creates meshes
	createGrids();
}

GeoClipmapCube::~GeoClipmapCube(void)
{
	for(int i = 0; i < 6; i++)
		delete m_Patches[i];
	removeBlockMeshes();
}

const String& GeoClipmapCube::getMovableType(void) const
{
	static String movType = "GeoClipmapCube";
	return movType;
}

const AxisAlignedBox& GeoClipmapCube::getBoundingBox(void) const
{
	return m_AABB;
}

Real GeoClipmapCube::getBoundingRadius(void) const
{
	return m_Radius + m_MaxHeight;
}

void GeoClipmapCube::_updateRenderQueue(RenderQueue* queue)
{
	computePatchViewpoints();
	for(int i = 0; i < 6; i++)
		m_Patches[i]->_updateRenderQueue(queue);
}
/* *********** VERY SERIOIS PROBLEM:
THE TRANSFORMATION OF FACE TX SHOULD BE SEPARATED INTO 2 MAT
THE SOLELY FACE MATRIX
AND THE CUBE WORLD MATIX
*/
void GeoClipmapCube::computeFaceTxMat(Node* parent)
{
	// init the transformation matrix
	Matrix3 rot[6];
	Vector3 trans[6] = {
		Vector3( 1, 0, 0), Vector3(-1, 0, 0),
		Vector3( 0, 1, 0), Vector3( 0,-1, 0),
		Vector3( 0, 0, 1), Vector3( 0, 0,-1)
	};

	//float clipmapSize = (m_N - 1); // this is wrong, just a replacement value for debug
	float scaleGeoToClipmap = m_SemiEdgeLen / (m_ClipmapSize / 2.0);

	rot[0].FromAxes(Vector3( 0, 0,-1), Vector3( 0, 1, 0), Vector3( 1, 0, 0));
	rot[1].FromAxes(Vector3( 0, 0, 1), Vector3( 0, 1, 0), Vector3(-1, 0, 0));
	rot[2].FromAxes(Vector3( 1, 0, 0), Vector3( 0, 0,-1), Vector3( 0, 1, 0));
	rot[3].FromAxes(Vector3( 1, 0, 0), Vector3( 0, 0, 1), Vector3( 0,-1, 0));
	rot[4].FromAxes(Vector3( 1, 0, 0), Vector3( 0, 1, 0), Vector3( 0, 0, 1));
	rot[5].FromAxes(Vector3(-1, 0, 0), Vector3( 0, 1, 0), Vector3( 0, 0,-1));

	// computes....
	for (int i = 0; i < 6; i++)
	{
		m_xForm[i] = Matrix4::IDENTITY;
		// rot
		m_xForm[i] = rot[i] * scaleGeoToClipmap;
		// scale
		// trans
		m_xForm[i].setTrans(m_SemiEdgeLen * trans[i]);

		// combine it with the parent tx
		//m_xForm[i] =  m_xForm[i];
	}

	// finally, update the clip planes, since it is in world space...
	/*Plane clipPlanes[4] = { // old code for rect plane
		Plane( 1, 0, 0, m_ClipmapSize / 2.0),
		Plane(-1, 0, 0, m_ClipmapSize / 2.0),
		Plane( 0, 1, 0, m_ClipmapSize / 2.0),
		Plane( 0,-1, 0, m_ClipmapSize / 2.0),
	};*/

	float cp_nelm = Math::InvSqrt(2);
	float cp_d = m_ClipmapSize / 2.0 * Math::Sin(Degree(45));
	Plane clipPlanes[4] = {
		Plane( cp_nelm, 0, cp_nelm, cp_d),
		Plane(-cp_nelm, 0, cp_nelm, cp_d),
		Plane( 0, cp_nelm, cp_nelm, cp_d),
		Plane( 0,-cp_nelm, cp_nelm, cp_d),
	};

	for(int i = 0; i < 6; i++) {
		for(int j = 0; j < 4; j++) {
			m_Patches[i]->setClipPlanes(j, _getParentNodeFullTransform() * m_xForm[i] * clipPlanes[j]);
		}
	}
}

void GeoClipmapCube::_notifyAttached(Node* parent, bool isTagPoint)
{
	if (parent != NULL)
		computeFaceTxMat(parent);
	MovableObject::_notifyAttached(parent, isTagPoint);
}

void GeoClipmapCube::_notifyMoved(void)
{
	computeFaceTxMat(getParentNode());
	MovableObject::_notifyMoved();
}

void GeoClipmapCube::getFaceTransformMatrix(int faceID, Matrix4* mat) const
{
	// place the patches in correct location, scale and orientation
	assert(faceID >= 0 && faceID < 6);
	*mat = m_xForm[faceID];
}

void GeoClipmapCube::createGrids()
{
	int m = (m_N + 1) / 4;
	int l = 2 * (m - 1) + 3;
	createGrid(GCM_MESH_MXM, m, m);
	createGrid(GCM_MESH_MX3, m, 3);
	createGrid(GCM_MESH_3XM, 3, m);
	createGrid(GCM_MESH_2XL, 2, l);
	createGrid(GCM_MESH_LX2, l, 2);
}

void GeoClipmapCube::createGrid(MeshType meshType, int vertexCountX, int vertexCountY)
{
	ManualObject* manual = m_SceneMgr->createManualObject("Geoclipmap");

	// Use identity view/projection matrices
	manual->setUseIdentityProjection(true);
	manual->setUseIdentityView(true);

	// calc how many vertex and idx are needed
	int SegX = vertexCountX - 1, SegY = vertexCountY - 1;
	int BlkMeshWidth = SegX, BlkMeshHeight = SegY; // let them be equal, this saves lots of crazy maths
	int VertexCount = vertexCountX * vertexCountY;
	int RectCount = SegX * SegY;
	int IdxCount = RectCount * 2 * 3;

	manual->estimateVertexCount(VertexCount);
	manual->estimateIndexCount(IdxCount);

	manual->begin(m_ResNamePrefix + "TempObj", RenderOperation::OT_TRIANGLE_LIST);

	// gen vertex
	Vector3 vStart(-BlkMeshWidth / 2.0, BlkMeshHeight / 2.0, 0);
	m_MeshAABBs[meshType].setMinimum(-BlkMeshWidth / 2.0, -BlkMeshHeight / 2.0, -m_MaxHeight);
	m_MeshAABBs[meshType].setMaximum(BlkMeshWidth / 2.0, BlkMeshHeight / 2.0, m_MaxHeight);

	Vector3 vDeltaX((Real)BlkMeshWidth / SegX, 0, 0);
	Vector3 vDeltaY(0, -(Real)BlkMeshHeight / SegY, 0);

	Vector3 v(vStart);

	for(int y = 0; y < vertexCountY; y++)
	{
		for(int x = 0; x < vertexCountX; x++)
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
		for(int x = y * vertexCountX; x < y * vertexCountX + SegX; x++)
		{	
			manual->index(x);
			manual->index(x + vertexCountX);
			manual->index(x + vertexCountX + 1);
			manual->index(x);
			manual->index(x + vertexCountX + 1);
			manual->index(x + 1);
		}
	}

	manual->end();
	manual->convertToMesh(getMeshName(meshType));

	m_SceneMgr->destroyManualObject(manual);
}

const String& GeoClipmapCube::getMeshName(MeshType meshType) const
{
	static const String meshMxMSuffix = "MxM";
	static const String meshMx3Suffix = "Mx3";
	static const String mesh3xMSuffix = "3xM";
	static const String mesh2xLSuffix = "2xL";
	static const String meshLx2Suffix = "Lx2";
	static String names[] = {
		m_ResNamePrefix + meshMxMSuffix,
		m_ResNamePrefix + meshMx3Suffix,
		m_ResNamePrefix + mesh3xMSuffix,
		m_ResNamePrefix + mesh2xLSuffix,
		m_ResNamePrefix + meshLx2Suffix,
	};
	return names[meshType];
}

void GeoClipmapCube::removeBlockMeshes() const
{
	for(int i = 0; i < GCM_MESH_COUNT; i++)
		MeshManager::getSingleton().remove(getMeshName((MeshType)i));
}

inline int roundup(float n)
{
	if (n >= 0)
		return (n + 0.5);
	else
		return (n - 0.5);
}

inline int sign(float n)
{
	if (n >= 0)
		return 1;
	else
		return -1;
}

void Ogre::GeoClipmapCube::computePatchViewpoints()
{
	Matrix4 matLocalInv = _getParentNodeFullTransform().inverse();

	Vector3 camPosLocal = matLocalInv * m_Camera->getPosition();

	// scale it from geo space to clipmap space
	camPosLocal = camPosLocal / m_SemiEdgeLen * (m_ClipmapSize / 2.0);

	int activeFaceID = -1;

	// first part, find the active face
	if (Math::Abs(camPosLocal.x) > Math::Abs(camPosLocal.y)) {
		if (Math::Abs(camPosLocal.x) > Math::Abs(camPosLocal.z)) {
			if (camPosLocal.x >= 0) {
				activeFaceID = 0;
			} else {
				activeFaceID = 1;
			}
		} else {
			if (camPosLocal.z >= 0) {
				activeFaceID = 4;
			} else {
				activeFaceID = 5;
			}
		}
	} else {
		if (Math::Abs(camPosLocal.y) > Math::Abs(camPosLocal.z)) {
			if (camPosLocal.y >= 0) {
				activeFaceID = 2;
			} else {
				activeFaceID = 3;
			}
		} else {
			if (camPosLocal.z >= 0) {
				activeFaceID = 4;
			} else {
				activeFaceID = 5;
			}
		}
	}

	static int adjacentFaceTable[6][4] = {
		{5,	4,	2,	3},
		{4,	5,	2,	3},
		{0,	1,	5,	4},
		{0,	1,	4,	5},
		{0,	1,	2,	3},
		{1,	0,	2,	3}
	};

	// find out the lod level
	int maxLodLvl = getClipmapLevel(); // self exclusive
	std::vector<Vector2> viewPosLists[6];

	// resize the view post lists
	for (int i = 0; i < 6; i++)
	{
		viewPosLists[i].resize(maxLodLvl);
	}

	Vector2 camPosOfFaces[6];

	// init the view pos of every faces
	switch (activeFaceID)
	{
	case 0:
		camPosOfFaces[0].x = -camPosLocal.z * m_ClipmapSize / 2.0 / camPosLocal.x;
		camPosOfFaces[0].y = camPosLocal.y * m_ClipmapSize / 2.0 / camPosLocal.x;
		break;
	case 1:
		camPosOfFaces[0].x = camPosLocal.z * m_ClipmapSize / 2.0 / -camPosLocal.x;
		camPosOfFaces[0].y = camPosLocal.y * m_ClipmapSize / 2.0 / -camPosLocal.x;
		break;
	case 2:
		camPosOfFaces[0].x = camPosLocal.x * m_ClipmapSize / 2.0 / camPosLocal.y;
		camPosOfFaces[0].y = -camPosLocal.z * m_ClipmapSize / 2.0 / camPosLocal.y;
		break;
	case 3:
		camPosOfFaces[0].x = camPosLocal.x * m_ClipmapSize / 2.0 / -camPosLocal.y;
		camPosOfFaces[0].y = camPosLocal.z * m_ClipmapSize / 2.0 / -camPosLocal.y;
		break;
	case 4:
		camPosOfFaces[0].x = camPosLocal.x * m_ClipmapSize / 2.0 / camPosLocal.z;
		camPosOfFaces[0].y = camPosLocal.y * m_ClipmapSize / 2.0 / camPosLocal.z;
		break;
	case 5:
		camPosOfFaces[0].x = -camPosLocal.x * m_ClipmapSize / 2.0 / -camPosLocal.z;
		camPosOfFaces[0].y = camPosLocal.y * m_ClipmapSize / 2.0 / -camPosLocal.z;
		break;
	}
	for (int i = 1; i < 6; i++)
	{
		camPosOfFaces[i] = camPosOfFaces[0];
	}

	int maxLodLvlAdjacent[6];
	memset(maxLodLvlAdjacent, 0, sizeof(maxLodLvlAdjacent));
	maxLodLvlAdjacent[activeFaceID] = maxLodLvl - 1;

	bool updated = false;
	// now calculate the view pos at every lod level of active face first
	// also calculate the offset of the adjacent faces
	for (int lodLvl = 0; lodLvl < maxLodLvl; lodLvl++)
	{
		if (lodLvl == 0) {
			viewPosLists[activeFaceID][lodLvl].x = roundup(camPosOfFaces[activeFaceID].x * Math::Pow(2, lodLvl)) / Math::Pow(2, lodLvl);
			viewPosLists[activeFaceID][lodLvl].y = roundup(camPosOfFaces[activeFaceID].y * Math::Pow(2, lodLvl)) / Math::Pow(2, lodLvl);
		} else {
			Vector2 offsetSign = camPosOfFaces[activeFaceID] - viewPosLists[activeFaceID][lodLvl - 1];
			offsetSign.x = sign(offsetSign.x);
			offsetSign.y = sign(offsetSign.y);

			Vector2 offsetMag = Vector2(Math::Pow(2, -lodLvl));

			Vector2 offset = offsetSign * offsetMag;
			viewPosLists[activeFaceID][lodLvl] = viewPosLists[activeFaceID][lodLvl - 1] + offset;

			// Ensure continuous lod level
			int nl = m_N - 1;
			float lodLvlPow = Math::Pow(2, -lodLvl);
			float half_nl = nl / 2;
			float half_nl_lod = half_nl * lodLvlPow;
			float eps = 1e-3;
			float halfClipmapSize = m_ClipmapSize / 2;

			float xp, xn, yp, yn;
			xp = halfClipmapSize - viewPosLists[activeFaceID][lodLvl].x - half_nl_lod;
			xn = halfClipmapSize - -viewPosLists[activeFaceID][lodLvl].x - half_nl_lod;
			yp = halfClipmapSize - viewPosLists[activeFaceID][lodLvl].y - half_nl_lod;
			yn = halfClipmapSize - -viewPosLists[activeFaceID][lodLvl].y - half_nl_lod;

			if (Math::Abs(xp) <= eps)
				camPosOfFaces[adjacentFaceTable[activeFaceID][0]].x += 2 * lodLvlPow;
			if (Math::Abs(xn) <= eps)
				camPosOfFaces[adjacentFaceTable[activeFaceID][1]].x -= 2 * lodLvlPow;
			if (Math::Abs(yp) <= eps)
				camPosOfFaces[adjacentFaceTable[activeFaceID][2]].y += 2 * lodLvlPow;
			if (Math::Abs(yn) <= eps)
				camPosOfFaces[adjacentFaceTable[activeFaceID][3]].y -= 2 * lodLvlPow;

			if (xp <= eps)
				maxLodLvlAdjacent[adjacentFaceTable[activeFaceID][0]] = lodLvl;
			if (xn <= eps)
				maxLodLvlAdjacent[adjacentFaceTable[activeFaceID][1]] = lodLvl;
			if (yp <= eps)
				maxLodLvlAdjacent[adjacentFaceTable[activeFaceID][2]] = lodLvl;
			if (yn <= eps)
				maxLodLvlAdjacent[adjacentFaceTable[activeFaceID][3]] = lodLvl;

			const std::vector<Vector2>& oldViewPosList = m_Patches[activeFaceID]->getViewPosList();
			if (oldViewPosList.size() <= lodLvl) {
				updated = true;
				continue;
			}

			Vector2 oldViewPos = oldViewPosList[lodLvl];
			if ((oldViewPos - viewPosLists[activeFaceID][lodLvl]).length() > 1)
				updated = true;
		}
	}

	// update is not needed, return
	if (!updated) return;

	// now, calculate the view position of adjacent faces
	for(int i = 0; i < 6; i++)
	{
		if (i == activeFaceID) continue;
		for (int lodLvl = 0; lodLvl < maxLodLvl; lodLvl++)
		{
			if (lodLvl == 0) {
				viewPosLists[i][lodLvl].x = roundup(camPosOfFaces[i].x * Math::Pow(2, lodLvl)) / Math::Pow(2, lodLvl);
				viewPosLists[i][lodLvl].y = roundup(camPosOfFaces[i].y * Math::Pow(2, lodLvl)) / Math::Pow(2, lodLvl);
			} else {
				Vector2 offsetSign = camPosOfFaces[i] - viewPosLists[i][lodLvl - 1];
				offsetSign.x = sign(offsetSign.x);
				offsetSign.y = sign(offsetSign.y);

				Vector2 offsetMag = Vector2(Math::Pow(2, -lodLvl));

				Vector2 offset = offsetSign * offsetMag;
				viewPosLists[i][lodLvl] = viewPosLists[i][lodLvl - 1] + offset;
			}
		}
	}

	// remove cracks between adjacent faces
	if (maxLodLvl > 2) {
		int xpFaceIdx, xnFaceIdx, ypFaceIdx, ynFaceIdx;
		xpFaceIdx = adjacentFaceTable[activeFaceID][0];
		xnFaceIdx = adjacentFaceTable[activeFaceID][1];
		ypFaceIdx = adjacentFaceTable[activeFaceID][2];
		ynFaceIdx = adjacentFaceTable[activeFaceID][3];

		int nl = m_N - 1;
		float lodLvlPow = 1;//Math::Pow(2, -lodLvl);
		float half_nl = nl / 2;
		float half_nl_lod = half_nl * lodLvlPow;
		float eps = 0.5 + 1e-3; //why 0.5???
		float halfClipmapSize = m_ClipmapSize / 2;

		float xp, xn, yp, yn;
		xp = halfClipmapSize - viewPosLists[activeFaceID][0].x - half_nl_lod;
		xn = halfClipmapSize - -viewPosLists[activeFaceID][0].x - half_nl_lod;
		yp = halfClipmapSize - viewPosLists[activeFaceID][0].y - half_nl_lod;
		yn = halfClipmapSize - -viewPosLists[activeFaceID][0].y - half_nl_lod;

		if (xp <= eps && yp <= eps) {
			if (viewPosLists[ypFaceIdx][0].y > viewPosLists[xpFaceIdx][0].x)
				for(int lodLvl = 0; lodLvl < maxLodLvl; lodLvl++) {
					viewPosLists[xpFaceIdx][lodLvl].x = viewPosLists[ypFaceIdx][lodLvl].y;
				}
			else
				for(int lodLvl = 0; lodLvl < maxLodLvl; lodLvl++) {
					viewPosLists[ypFaceIdx][lodLvl].y = viewPosLists[xpFaceIdx][lodLvl].x;
				};
		}

		if (xp <= eps && yn <= eps) {
			if (-viewPosLists[ynFaceIdx][0].y > viewPosLists[xpFaceIdx][0].x)
				for(int lodLvl = 0; lodLvl < maxLodLvl; lodLvl++) {
					viewPosLists[xpFaceIdx][lodLvl].x = -viewPosLists[ynFaceIdx][lodLvl].y;
				}
			else
				for(int lodLvl = 0; lodLvl < maxLodLvl; lodLvl++) {
					viewPosLists[ynFaceIdx][lodLvl].y = -viewPosLists[xpFaceIdx][lodLvl].x;
				};
		}

		if (xn <= eps && yp <= eps) {
			if (viewPosLists[ypFaceIdx][0].y > -viewPosLists[xnFaceIdx][0].x)
				for(int lodLvl = 0; lodLvl < maxLodLvl; lodLvl++) {
					viewPosLists[xnFaceIdx][lodLvl].x = -viewPosLists[ypFaceIdx][lodLvl].y;
				}
			else
				for(int lodLvl = 0; lodLvl < maxLodLvl; lodLvl++) {
					viewPosLists[ypFaceIdx][lodLvl].y = -viewPosLists[xnFaceIdx][lodLvl].x;
				};
		}
		if (xn <= eps && yn <= eps) {
			if (-viewPosLists[ynFaceIdx][0].y > -viewPosLists[xnFaceIdx][0].x)
				for(int lodLvl = 0; lodLvl < maxLodLvl; lodLvl++) {
					viewPosLists[xnFaceIdx][lodLvl].x = viewPosLists[ynFaceIdx][lodLvl].y;
				}
			else
				for(int lodLvl = 0; lodLvl < maxLodLvl; lodLvl++) {
					viewPosLists[ynFaceIdx][lodLvl].y = viewPosLists[xnFaceIdx][lodLvl].x;
				};
		}
	}

	// transform them into correct face spaces
	for (int lodLvl = 0; lodLvl < maxLodLvl; lodLvl++)
	{
		Vector2 st[6];

		switch (activeFaceID)
		{
		case 0:
			st[0].x = viewPosLists[0][lodLvl].x;
			st[0].y = viewPosLists[0][lodLvl].y;
			st[1].x = 0;
			st[1].y = 0;
			st[2].x = m_ClipmapSize - viewPosLists[2][lodLvl].y;
			st[2].y = viewPosLists[2][lodLvl].x;
			st[3].x = viewPosLists[3][lodLvl].y + m_ClipmapSize;
			st[3].y = -viewPosLists[3][lodLvl].x;
			st[4].x = viewPosLists[4][lodLvl].x + m_ClipmapSize;
			st[4].y = viewPosLists[4][lodLvl].y;
			st[5].x = viewPosLists[5][lodLvl].x - m_ClipmapSize;
			st[5].y = viewPosLists[5][lodLvl].y;
			break;
		case 1:
			st[0].x = 0;
			st[0].y = 0;
			st[1].x = viewPosLists[1][lodLvl].x;
			st[1].y = viewPosLists[1][lodLvl].y;
			st[2].x = viewPosLists[2][lodLvl].y - m_ClipmapSize;
			st[2].y = -viewPosLists[2][lodLvl].x;
			st[3].x = -m_ClipmapSize - viewPosLists[3][lodLvl].y;
			st[3].y = viewPosLists[3][lodLvl].x;
			st[4].x = viewPosLists[4][lodLvl].x - m_ClipmapSize;
			st[4].y = viewPosLists[4][lodLvl].y;
			st[5].x = viewPosLists[5][lodLvl].x + m_ClipmapSize;
			st[5].y = viewPosLists[5][lodLvl].y;
			break;
		case 2:
			st[0].x = viewPosLists[0][lodLvl].y;
			st[0].y = m_ClipmapSize - viewPosLists[0][lodLvl].x;
			st[1].x = -viewPosLists[1][lodLvl].y;
			st[1].y = viewPosLists[1][lodLvl].x + m_ClipmapSize;
			st[2].x = viewPosLists[2][lodLvl].x;
			st[2].y = viewPosLists[2][lodLvl].y;
			st[3].x = 0;
			st[3].y = 0;
			st[4].x = viewPosLists[4][lodLvl].x;
			st[4].y = viewPosLists[4][lodLvl].y + m_ClipmapSize;
			st[5].x = -viewPosLists[5][lodLvl].x;
			st[5].y = m_ClipmapSize - viewPosLists[5][lodLvl].y;
			break;
		case 3:
			st[0].x = -viewPosLists[0][lodLvl].y;
			st[0].y = viewPosLists[0][lodLvl].x - m_ClipmapSize;
			st[1].x = viewPosLists[1][lodLvl].y;
			st[1].y = -m_ClipmapSize - viewPosLists[1][lodLvl].x;
			st[2].x = 0;
			st[2].y = 0;
			st[3].x = viewPosLists[3][lodLvl].x;
			st[3].y = viewPosLists[3][lodLvl].y;
			st[4].x = viewPosLists[4][lodLvl].x;
			st[4].y = viewPosLists[4][lodLvl].y - m_ClipmapSize;
			st[5].x = -viewPosLists[5][lodLvl].x;
			st[5].y = -m_ClipmapSize - viewPosLists[5][lodLvl].y;
			break;
		case 4:
			st[0].x = viewPosLists[0][lodLvl].x - m_ClipmapSize;
			st[0].y = viewPosLists[0][lodLvl].y;
			st[1].x = viewPosLists[1][lodLvl].x + m_ClipmapSize;
			st[1].y = viewPosLists[1][lodLvl].y;
			st[2].x = viewPosLists[2][lodLvl].x;
			st[2].y = viewPosLists[2][lodLvl].y - m_ClipmapSize;
			st[3].x = viewPosLists[3][lodLvl].x;
			st[3].y = viewPosLists[3][lodLvl].y + m_ClipmapSize;
			st[4].x = viewPosLists[4][lodLvl].x;
			st[4].y = viewPosLists[4][lodLvl].y;
			st[5].x = 0;
			st[5].y = 0;
			break;
		case 5:
			st[0].x = viewPosLists[0][lodLvl].x + m_ClipmapSize;
			st[0].y = viewPosLists[0][lodLvl].y;
			st[1].x = viewPosLists[1][lodLvl].x - m_ClipmapSize;
			st[1].y = viewPosLists[1][lodLvl].y;
			st[2].x = -viewPosLists[2][lodLvl].x;
			st[2].y = m_ClipmapSize - viewPosLists[2][lodLvl].y;
			st[3].x = -viewPosLists[3][lodLvl].x;
			st[3].y = -m_ClipmapSize - viewPosLists[3][lodLvl].y;
			st[4].x = 0;
			st[4].y = 0;
			st[5].x = viewPosLists[5][lodLvl].x;
			st[5].y = viewPosLists[5][lodLvl].y;
			break;
		}
		for(int i = 0; i < 6; i++) {
			viewPosLists[i][lodLvl] = st[i];
		}
	}

	for(int i = 0; i < 6; i++) {	
		viewPosLists[i].resize(maxLodLvlAdjacent[i] + 1);
		m_Patches[i]->setViewPosList(viewPosLists[i]);
	}
}

unsigned int Ogre::GeoClipmapCube::getClipmapLevel() const
{
	return 5; 
}