#pragma once

#include <OgreRenderable.h>
#include <OgreVector2.h>
#include <OgreMatrix4.h>

namespace Ogre
{
	class GeoClipmapPatch;

	class GeoClipmapBlock : public Renderable
	{
	private:
		//// Variables
		const GeoClipmapPatch& m_parentPatch;
		const MovableObject& m_parentMovObj;
		Matrix4 m_Tx;
	public:
		GeoClipmapBlock(const GeoClipmapPatch& patch, const MovableObject& movObj);
		virtual ~GeoClipmapBlock(void);
		//// Variables
		String m_MeshName;
		//int m_PosIdx, 
		int m_LodLvl;
		Vector2 m_Pos;
		//// Methods
		void computeTransform();
		// override
		virtual const MaterialPtr& getMaterial(void) const;
		virtual void getRenderOperation(RenderOperation &op);
		virtual void getWorldTransforms(Matrix4* xform) const;
		virtual Real getSquaredViewDepth(const Camera *cam) const;
		virtual const LightList& getLights(void) const;
		//virtual void _updateCustomGpuParameter(const GpuProgramParameters::AutoConstantEntry &constantEntry, GpuProgramParameters *params) const;
		virtual bool preRender(SceneManager*  sm, RenderSystem* rsys);
		virtual void postRender(SceneManager*  sm, RenderSystem* rsys);
	};
}
