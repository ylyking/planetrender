#pragma once
#include <vector>
#include <OgreString.h>
#include <OgreTexture.h>

namespace Ogre
{
	class Clipmap
	{
	public:
		class ClipmapLayer
		{
			friend class Clipmap;
		private:
			Ogre::TexturePtr m_Tex;
			Ogre::String m_FileName;
			Clipmap* m_Clipmap;
		public:
			ClipmapLayer(unsigned int level, const Ogre::String& fileName, Clipmap* cm);
			void loadVisibleArea(const Rect& visibleTextureRect, const Rect& heightMapRect);
		};
		
		//// cstr/dstr
		Clipmap(unsigned int depth, unsigned int baseSize, unsigned int maxActiveSize);
		~Clipmap(void);

		//// getters/setters
		unsigned int getMaxActiveSize() const { return m_MaxActiveSize; }
		unsigned int getDepth() const { return m_Depth; }
		//// methods
		void addTexture(const Ogre::String& fileName);
		unsigned int getLayerSize(unsigned int level) const;
		Ogre::TexturePtr getLayerTexture(unsigned int level) const;
		void updateVisibleArea(unsigned int level, Vector2 centralUV);
	private:
		unsigned int m_Depth, m_BaseSize, m_MaxActiveSize;
		std::vector<ClipmapLayer*> m_Layers;
	};
}