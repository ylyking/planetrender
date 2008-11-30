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
	m_ClipmapSize = 2 * (m_N - 1); // this is wrong, just a replacement value for debug
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
		m_xForm[i] = _getParentNodeFullTransform() * m_xForm[i];
	}

	// finally, update the clip planes, since it is in world space...
	Plane clipPlanes[4] = {
		Plane( 1, 0, 0, m_ClipmapSize / 2.0),
		Plane(-1, 0, 0, m_ClipmapSize / 2.0),
		Plane( 0, 1, 0, m_ClipmapSize / 2.0),
		Plane( 0,-1, 0, m_ClipmapSize / 2.0),
	};

	for(int i = 0; i < 6; i++) {
		for(int j = 0; j < 4; j++) {
			m_Patches[i]->setClipPlanes(j, m_xForm[i] * clipPlanes[j]);
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
	return (n + 0.5);
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

	int faceID = -1;

	// first part, find the active face
	if (Math::Abs(camPosLocal.x) > Math::Abs(camPosLocal.y)) {
		if (Math::Abs(camPosLocal.x) > Math::Abs(camPosLocal.z)) {
			if (camPosLocal.x >= 0) {
				faceID = 0;
			} else {
				faceID = 1;
			}
		} else {
			if (camPosLocal.z >= 0) {
				faceID = 4;
			} else {
				faceID = 5;
			}
		}
	} else {
		if (Math::Abs(camPosLocal.y) > Math::Abs(camPosLocal.z)) {
			if (camPosLocal.y >= 0) {
				faceID = 2;
			} else {
				faceID = 3;
			}
		} else {
			if (camPosLocal.z >= 0) {
				faceID = 4;
			} else {
				faceID = 5;
			}
		}
	}

	// find out the lod level
	int maxLodLvl = 3;
	std::vector<Vector2> viewPosLists[6];
	
	// resize the view post lists
	for (int i = 0; i < 6; i++)
	{
		viewPosLists[i].resize(maxLodLvl);
	}
	
	bool updated = false;

	for (int lodLvl = 0; lodLvl < maxLodLvl; lodLvl++)
	{
		switch (faceID)
		{
		case 0:
			viewPosLists[faceID][lodLvl].x = -camPosLocal.z * m_ClipmapSize / 2 / camPosLocal.x;
			viewPosLists[faceID][lodLvl].y = camPosLocal.y * m_ClipmapSize / 2 / camPosLocal.x;
			break;
		case 1:
			viewPosLists[faceID][lodLvl].x = camPosLocal.z * m_ClipmapSize / 2 / -camPosLocal.x;
			viewPosLists[faceID][lodLvl].y = camPosLocal.y * m_ClipmapSize / 2 / -camPosLocal.x;
			break;
		case 2:
			viewPosLists[faceID][lodLvl].x = camPosLocal.x * m_ClipmapSize / 2 / camPosLocal.y;
			viewPosLists[faceID][lodLvl].y = -camPosLocal.z * m_ClipmapSize / 2 / camPosLocal.y;
			break;
		case 3:
			viewPosLists[faceID][lodLvl].x = camPosLocal.x * m_ClipmapSize / 2 / -camPosLocal.y;
			viewPosLists[faceID][lodLvl].y = camPosLocal.z * m_ClipmapSize / 2 / -camPosLocal.y;
			break;
		case 4:
			viewPosLists[faceID][lodLvl].x = camPosLocal.x * m_ClipmapSize / 2 / camPosLocal.z;
			viewPosLists[faceID][lodLvl].y = camPosLocal.y * m_ClipmapSize / 2 / camPosLocal.z;
			break;
		case 5:
			viewPosLists[faceID][lodLvl].x = -camPosLocal.x * m_ClipmapSize / 2 / -camPosLocal.z;
			viewPosLists[faceID][lodLvl].y = camPosLocal.y * m_ClipmapSize / 2 / -camPosLocal.z;
			break;
		}
		if (lodLvl == 0) {
			viewPosLists[faceID][lodLvl].x = roundup(viewPosLists[faceID][lodLvl].x);
			viewPosLists[faceID][lodLvl].y = roundup(viewPosLists[faceID][lodLvl].y);
		} else {
			Vector2 offsetSign = viewPosLists[faceID][lodLvl] - viewPosLists[faceID][lodLvl - 1];
			offsetSign.x = sign(offsetSign.x);
			offsetSign.y = sign(offsetSign.y);

			Vector2 offsetMag = Vector2(Math::Pow(2, -lodLvl));

			Vector2 offset = offsetSign * offsetMag;
			viewPosLists[faceID][lodLvl] = viewPosLists[faceID][lodLvl - 1] + offset;
		}
		const std::vector<Vector2>& oldViewPosList = m_Patches[faceID]->getViewPosList();
		if (oldViewPosList.size() <= lodLvl) {
			updated = true;
			continue;
		}
		Vector2 oldViewPos = oldViewPosList[lodLvl];
		if ((oldViewPos - viewPosLists[faceID][lodLvl]).length() > 1)
			updated = true;
	}

	if (!updated) return;

	// then calculate the uv for the active face for all lod levels
	for (int lodLvl = 0; lodLvl < maxLodLvl; lodLvl++)
	{
		Vector2 st[6];
		st[faceID].x = viewPosLists[faceID][lodLvl].x;
		st[faceID].y = viewPosLists[faceID][lodLvl].y;
		switch (faceID)
		{
		case 0:
			st[1].x = 0;
			st[1].y = 0;
			st[2].x = m_ClipmapSize - st[faceID].y;
			st[2].y = st[faceID].x;
			st[3].x = st[faceID].y + m_ClipmapSize;
			st[3].y = -st[faceID].x;
			st[4].x = st[faceID].x + m_ClipmapSize;
			st[4].y = st[faceID].y;
			st[5].x = st[faceID].x - m_ClipmapSize;
			st[5].y = st[faceID].y;
			break;
		case 1:
			st[0].x = 0;
			st[0].y = 0;
			st[2].x = st[faceID].y - m_ClipmapSize;
			st[2].y = -st[faceID].x;
			st[3].x = -m_ClipmapSize - st[faceID].y;
			st[3].y = st[faceID].x;
			st[4].x = st[faceID].x - m_ClipmapSize;
			st[4].y = st[faceID].y;
			st[5].x = st[faceID].x + m_ClipmapSize;
			st[5].y = st[faceID].y;
			break;
		case 2:
			st[0].x = st[faceID].y;
			st[0].y = m_ClipmapSize - st[faceID].x;
			st[1].x = -st[faceID].y;
			st[1].y = st[faceID].x + m_ClipmapSize;
			st[3].x = 0;
			st[3].y = 0;
			st[4].x = st[faceID].x;
			st[4].y = st[faceID].y + m_ClipmapSize;
			st[5].x = -st[faceID].x;
			st[5].y = m_ClipmapSize - st[faceID].y;
			break;
		case 3:
			st[0].x = -st[faceID].y;
			st[0].y = st[faceID].x - m_ClipmapSize;
			st[1].x = st[faceID].y;
			st[1].y = -m_ClipmapSize - st[faceID].x;
			st[2].x = 0;
			st[2].y = 0;
			st[4].x = st[faceID].x;
			st[4].y = st[faceID].y - m_ClipmapSize;
			st[5].x = -st[faceID].x;
			st[5].y = -m_ClipmapSize - st[faceID].y;
			break;
		case 4:
			st[0].x = st[faceID].x - m_ClipmapSize;
			st[0].y = st[faceID].y;
			st[1].x = st[faceID].x + m_ClipmapSize;
			st[1].y = st[faceID].y;
			st[2].x = st[faceID].x;
			st[2].y = st[faceID].y - m_ClipmapSize;
			st[3].x = st[faceID].x;
			st[3].y = st[faceID].y + m_ClipmapSize;
			st[5].x = 0;
			st[5].y = 0;
			break;
		case 5:
			st[0].x = st[faceID].x + m_ClipmapSize;
			st[0].y = st[faceID].y;
			st[1].x = st[faceID].x - m_ClipmapSize;
			st[1].y = st[faceID].y;
			st[2].x = -st[faceID].x;
			st[2].y = m_ClipmapSize - st[faceID].y;
			st[3].x = -st[faceID].x;
			st[3].y = -m_ClipmapSize - st[faceID].y;
			st[4].x = 0;
			st[4].y = 0;
			break;
		}
		for (int i = 0; i < 6; i++)
			viewPosLists[i][lodLvl] = st[i];
	}

	{
		for(int i = 0; i < 6; i++) {
			/*std::vector<Vector2> viewPosList(2);

			viewPosList[0] = Vector2(0, 0);
			viewPosList[1] = Vector2(0.5, 0.5);
			//viewPosList[2] = Vector2(0.75, 0.75);
			m_Patches[i]->setViewPosList(viewPosList);
			
			*/
			
			m_Patches[i]->setViewPosList(viewPosLists[i]);
		}
	}

}
