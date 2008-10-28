#pragma once
#include <OgreNode.h>

namespace Ogre
{
	class GeoClipmapPatch;
	enum BlockType {GCM_BLK_MXM = 0, GCM_BLK_MX3, GCM_BLK_3XM, GCM_BLK_INT_HORI, GCM_BLK_INT_VERT, GCM_BLK_TFILLER_HORI, GCM_BLK_TFILLER_VERT};
	class GeoClipmapBlock : public Renderable
	{
	private:
		GeoClipmapPatch* mPatch;
		// local transformation 
		Vector3 mScale, mPos;
		Vector4 mLocalPos; // the w is N
		Vector4 mCourserPosOffset; // it is indeed used as a vector2
		// cached local transformation
		Matrix4 mCachedLocalTransform;
		// internal state
		String mMeshName;
		int mDepth;
		AxisAlignedBox mLocalAABB, mWorldAABB;
	public:
		inline const AxisAlignedBox& getWorldAABB() const {
			return mWorldAABB;
		}
		GeoClipmapBlock(GeoClipmapPatch* patch);
		virtual ~GeoClipmapBlock();
		void update(const Vector3& pos, const Vector3& scale, const Vector3& localBlockPos, int depth, BlockType bt, const Vector2& courserPosOffset);
		inline bool isInsidePatch(AxisAlignedBox aabbPatch) const {
			return aabbPatch.contains(mLocalAABB) || aabbPatch.intersects(mLocalAABB);
		}
		// override
		virtual const MaterialPtr& getMaterial(void) const;
		virtual void getRenderOperation(RenderOperation &op);
        virtual void getWorldTransforms(Matrix4* xform) const;
		virtual Real getSquaredViewDepth(const Camera *cam) const;
		const LightList& getLights(void) const;

		virtual void _updateCustomGpuParameter(const GpuProgramParameters::AutoConstantEntry &constantEntry, GpuProgramParameters *params) const;
		virtual bool preRender(SceneManager*  sm, RenderSystem* rsys);
		virtual void postRender(SceneManager*  sm, RenderSystem* rsys);

		//WireBoundingBox* mWireBoundingBox;
	};
}
