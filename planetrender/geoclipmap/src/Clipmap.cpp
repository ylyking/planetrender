#include "Clipmap.h"

#include <OgreTextureManager.h>
#include <OgreStringConverter.h>
#include <OgreVector2.h>
#include <OgreHardwarePixelBuffer.h>

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

Clipmap::ClipmapLayer::ClipmapLayer(unsigned int level, const String& fileName, Clipmap* cm) :
	m_FileName(fileName)
{
	unsigned int size = cm->getLayerSize(level);

	if (level > 0)
		size = cm->getMaxActiveSize();

	m_Tex = TextureManager::getSingleton().createManual(
				"ClipmapLayerTex" + StringConverter::toString((long)this),
				ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
				TEX_TYPE_2D,
				size,
				size,
				0,
				PF_R8G8B8A8,
				TU_WRITE_ONLY);

	if (level == 0) {// whole texture should be keep in memory
		Image layer0Img;
		layer0Img.load(fileName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		//if (layer0Img.getHeight() != layer0Img.getWidth()) {}
		//if (layer0Img.getHeight() != m_BaseSize) {}
			// error
		m_Tex->loadImage(layer0Img);
	}
}

void Clipmap::updateVisibleArea(unsigned int level, Vector2 centralUV)
{
	int texSize = getLayerSize(level);

	centralUV = centralUV * (texSize-1) / (m_BaseSize-1);

	centralUV.x += (texSize-1) / 2.0;
	centralUV.y += (texSize-1) / 2.0;

	/*centralUV.x = (int)(centralUV.x + 0.5);
	centralUV.y = (int)(centralUV.y + 0.5);*/ // must be disabled, the +0.5 is wrong, fucking wrong, I wasted a night to track it down! fuck!!!!

	int halfActiveSize = (m_MaxActiveSize - 1) / 2;
	int src_x0 = centralUV.x - halfActiveSize;
	int src_y0 = centralUV.y - halfActiveSize;
	int src_x1 = centralUV.x + halfActiveSize;
	int src_y1 = centralUV.y + halfActiveSize;

	int dest_x0 = 0;
	int dest_y0 = 0;
	int dest_x1 = getMaxActiveSize();
	int dest_y1 = getMaxActiveSize();

	if (src_x0 < 0) {
		dest_x0 += -src_x0;
		src_x0 = 0;
	}

	if (src_y0 < 0) {
		dest_y0 += -src_y0;
		src_y0 = 0;
	}

	if (src_x1 > texSize) {
		dest_x1 -= src_x1 - texSize;
		src_x1 = texSize;
	}

	if (src_y1 > texSize) {
		dest_y1 -= src_y1 - texSize;
		src_y1 = texSize;
	}

	m_Layers[level]->loadVisibleArea(Rect(dest_x0, dest_y0, dest_x1, dest_y1),
		Rect(src_x0, src_y0, src_x1, src_y1));
}

void Clipmap::ClipmapLayer::loadVisibleArea(const Rect& textureBufferRect, const Rect& heightMapRect)
{
	class Bitmap {
	private:
		DataStreamPtr mBmpData;
#pragma pack(push)
#pragma pack(1)
		struct BMPHEADER {
			char type[2];
			unsigned int filesize;           /* File size in bytes          */
			unsigned short int reserved1, reserved2;
			unsigned int offset;             /* Offset to image data, bytes */
			unsigned int size;               /* Header size in bytes      */
			int width, height;               /* Width and height of image */
			unsigned short int planes;       /* Number of colour planes   */
			unsigned short int bits;         /* Bits per pixel            */
			unsigned int compression;        /* Compression type          */
			unsigned int imagesize;          /* Image size in bytes       */
			int xresolution,yresolution;     /* Pixels per meter          */
			unsigned int ncolours;           /* Number of colours         */
			unsigned int importantcolours;   /* Important colours         */
		} mBmpHeader;
#pragma pack(pop)
		int mBytesPerLine, mBPP;
		inline void bmpPixelSeek(DataStreamPtr stm, int x, int y) {
			int realy = mBmpHeader.height - y - 1;
			//int realy = y;
			stm->seek(mBmpHeader.offset + realy * mBytesPerLine + x * mBPP);
		};
		inline char* pixelBufferPixelSeek(const PixelBox& pixelBox, int x, int y) {
			return (char*)pixelBox.data + y * pixelBox.getWidth() * 4 + x * 4;
		};
	public:
		Bitmap(const String& fileName) {
			mBmpData = ResourceGroupManager::getSingleton().openResource(fileName);

			mBmpData->read(&mBmpHeader, sizeof(mBmpHeader));

			if (mBmpHeader.type[0] != 'B' || mBmpHeader.type[1] != 'M')
				OGRE_EXCEPT(0, "It is not a bmp file", "Bitmap::Bitmap");

			if (mBmpHeader.bits != 24 && mBmpHeader.bits != 32)
				OGRE_EXCEPT(0, "Only 24bit and 32bit bmp file is supported.", "Bitmap::Bitmap");

			mBPP = mBmpHeader.bits / 8;

			mBytesPerLine = mBmpHeader.width * mBPP;
			if (mBytesPerLine & 0x0003) 
			{
				mBytesPerLine |= 0x0003;
				++mBytesPerLine;
			}

		}
		void copyData(const Rect& areaToBeCopied, const PixelBox& pixelBox, const Rect& areaToBeWritten) {
			int rowToBeWritten = areaToBeWritten.top;
			for(int y = areaToBeCopied.top; y <= areaToBeCopied.bottom; ++y) {
				bmpPixelSeek(mBmpData, areaToBeCopied.left, y);
				char* rowData = pixelBufferPixelSeek(pixelBox, areaToBeWritten.left, rowToBeWritten);
				for(int x = areaToBeCopied.left; x <= areaToBeCopied.right; ++x) {
					char bgra[4];
					bgra[3] = 0xFF;
					mBmpData->read(bgra, mBPP);
					*rowData++ = bgra[0];
					*rowData++ = bgra[1];
					*rowData++ = bgra[2];
					*rowData++ = bgra[3];
				}
				rowToBeWritten++;
			}
		}
	};
	try {
		HardwarePixelBufferSharedPtr texBuf = m_Tex->getBuffer();
		texBuf->lock(HardwareBuffer::LockOptions::HBL_DISCARD);
		const PixelBox& pixelBox = texBuf->getCurrentLock();

		// load bmp
		Bitmap bmpTex(m_FileName);
		// then copy heightMapRect to textureBufferRect
		bmpTex.copyData(heightMapRect, pixelBox, textureBufferRect);

		texBuf->unlock();
	} catch(...) {

	}
}