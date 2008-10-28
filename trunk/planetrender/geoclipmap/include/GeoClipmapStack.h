#pragma once
#include <vector>

namespace Ogre {
	class GeoClipmapStack
	{
	public:
		typedef std::vector<String> TextureNameList;
		typedef std::vector<MaterialPtr> MaterialList;
		typedef std::vector<TexturePtr> TextureList;
	private:
		TextureNameList mTextureNames;
		MaterialList mMaterials;
		TextureList mTextures;
		int mActiveClipmapSize;
		String mMaterialPrefix;
		
	public:
		void updateDepth(int depth, const Rect& visibleTextureRect, const Rect& heightMapRect);
		void update(Vector3 camPos);
		const MaterialPtr& getMaterial(int depth) const;
		GeoClipmapStack(const TextureNameList& textureNames, int activeClipmapSize); // textureNames is listed in dec order of details, i.e. the highest res texture is listed the first
		virtual ~GeoClipmapStack(void);
	};
}
