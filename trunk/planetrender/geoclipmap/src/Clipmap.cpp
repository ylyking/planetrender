#include "Clipmap.h"

#include <OgreTextureManager.h>
#include <OgreStringConverter.h>

using namespace std;
using namespace Ogre;

Clipmap::Clipmap(unsigned int depth, unsigned int baseSize, unsigned int maxActiveSize) :
	m_Depth(depth), m_BaseSize(baseSize), m_MaxActiveSize(maxActiveSize)
{
	m_Layers.reserve(depth);
}

Clipmap::~Clipmap(void)
{
	for (int i = 0; i < m_Layers.size(); i++)
		delete m_Layers[i];
}

void Clipmap::addTexture(const String& fileName)
{
	m_Layers.push_back(new ClipmapLayer(m_Layers.size(), fileName, this));
}

unsigned int Clipmap::getLayerSize(unsigned int level) const
{
	assert(level < m_Depth);
	unsigned int size = m_BaseSize;
	for(unsigned int i = 0; i < level; i++) {
		size = size * 2 - 1; // to do: add a subdivision flag
	}
	return size;
}

Ogre::TexturePtr Clipmap::getLayerTexture(unsigned int level) const
{
	assert(level < m_Depth);
	return m_Layers[level]->m_Tex;
}

Clipmap::ClipmapLayer::ClipmapLayer(unsigned int level, const String& fileName, Clipmap* cm)
{
	unsigned int size = cm->getLayerSize(level);

	if (level > 0)
		size = max(size, cm->getMaxActiveSize());

	m_Tex = TextureManager::getSingleton().createManual(
				"ClipmapLayerTex" + StringConverter::toString((long)this),
				ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
				TEX_TYPE_2D,
				size,
				size,
				0,
				PF_R8G8B8A8);

}
