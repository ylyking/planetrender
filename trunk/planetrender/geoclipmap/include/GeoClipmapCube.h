#pragma once

#include <OgreString.h>
#include <OgreMovableObject.h>
#include "GeoClipmapPatch.h"
#include "GeoClipmapBlock.h"
#include <map>

namespace Ogre
{
	class SceneManager;
	class Camera;
	class GeoClipmapCube : public MovableObject
	{
	public:
		//// Types
		enum MeshType {GCM_MESH_MXM = 0, GCM_MESH_MX3, GCM_MESH_3XM, GCM_MESH_2XL, GCM_MESH_LX2, GCM_MESH_COUNT};
	private:
		//// Varibles
		SceneManager* m_SceneMgr;
		Camera* m_Camera;
		// Bounding info
		float m_Radius, m_MaxHeight, m_SemiEdgeLen;
		int m_N; int m_ClipmapSize;
		AxisAlignedBox m_AABB;
		// Transformation matrix for faces
		Matrix4 m_xForm[6];
		// Faces
		GeoClipmapPatch* m_Patches[6];
		// AxisAlignedBox of Meshes
		std::map<MeshType, AxisAlignedBox> m_MeshAABBs;
		Ogre::String m_ResNamePrefix;
		//// Methods
		void createGrids();
		void createGrid(MeshType meshType, int vertexCountX, int vertexCountY);
		void removeBlockMeshes() const;
		void computeFaceTxMat(Node* parent);
		void computePatchViewpoints();
	public:
		//// Con/Destrs
		GeoClipmapCube(float radius, float maxHeight, int n, SceneManager* sceneMgr, Camera* camera);
		virtual ~GeoClipmapCube(void);
		//// Getters/setters
		inline float getRadius() const { return m_Radius; }		
		inline int getN() const { return m_N; }
		int getClipmapSize() const { return m_ClipmapSize; }
		//// Override
		virtual const String& getMovableType(void) const;
		virtual const AxisAlignedBox& getBoundingBox(void) const;
		virtual Real getBoundingRadius(void) const;
		virtual void _updateRenderQueue(RenderQueue* queue);
		virtual void _notifyAttached(Node* parent, bool isTagPoint = false);
		virtual void _notifyMoved(void);
		virtual void visitRenderables(Renderable::Visitor* visitor, 
			bool debugRenderables = false) {
		}
		//// Methods
		void getFaceTransformMatrix(int faceID, Matrix4* mat) const;
		const Ogre::String& getMeshName(MeshType meshType) const;
	};
}
