#include <OgreStringConverter.h>
#include <OgreTextureManager.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreImage.h>
#include "GeoClipmapStack.h"

using namespace Ogre;

#define S(a) StringConverter::toString(a)

GeoClipmapStack::GeoClipmapStack(const TextureNameList& textureNames, int activeClipmapSize) : mTextureNames(textureNames), mActiveClipmapSize(activeClipmapSize)
{
	mMaterialPrefix = "GeoClipmapStack" + StringConverter::toString((long)this);
	for(unsigned int i = 0; i < mTextureNames.size(); i++) {
		// first create the material
		MaterialPtr mat = MaterialManager::getSingleton().create(mMaterialPrefix + S(i), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		Pass* pass = mat->getTechnique(0)->getPass(0);
		pass->setCullingMode(CullingMode::CULL_NONE);
		pass->setVertexProgram("GeoclipmapVS");
		// then create the texture
		TexturePtr tex = TextureManager::getSingleton().createManual(mMaterialPrefix + S(i), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, activeClipmapSize, activeClipmapSize, 0, PF_BYTE_RGBA, TU_RENDERTARGET);
		mTextures.push_back(tex);
		// attach the texture
		TextureUnitState* tus = pass->createTextureUnitState(mMaterialPrefix + S(i));
		tus->setBindingType(Ogre::TextureUnitState::BindingType::BT_VERTEX);
		// no filtering
		//tus->setTextureFiltering(FO_NONE, FO_NONE, FO_NONE);
		tus->setTextureFiltering(FO_POINT, FO_POINT, FO_NONE);
		//tus->setTextureAddressingMode(TextureUnitState::TextureAddressingMode::TAM_BORDER);
		//tus->setTextureBorderColour(ColourValue::Blue);
		// attack the courser texture
		/*if (i > 0) {
			TextureUnitState* tus = pass->createTextureUnitState(mMaterialPrefix + S(i - 1));
			tus->setBindingType(Ogre::TextureUnitState::BindingType::BT_VERTEX);
tus->setTextureFiltering(FO_POINT, FO_POINT, FO_NONE);
// no filtering
			//tus->setTextureFiltering(FO_NONE, FO_NONE, FO_NONE);
		}*/
		
		mat->compile();
		mMaterials.push_back(mat);
	}
}

GeoClipmapStack::~GeoClipmapStack(void)
{
}

const MaterialPtr& GeoClipmapStack::getMaterial(int depth) const
{
	return mMaterials[depth];
}

int clamp(int i, int min, int max)
{
	if (i < min)
		i = min;
	else if (i > max)
		i = max;
	return i;
}

// redesign
void GeoClipmapStack::updateDepth(int depth, const Rect& visibleTextureRect, const Rect& heightMapRect)
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
				OGRE_EXCEPT(0, "It is not a bmp file", "GeoClipmapStack::updateDepth");

			if (mBmpHeader.bits != 24 && mBmpHeader.bits != 32)
				OGRE_EXCEPT(0, "Only 24bit and 32bit bmp file is supported.", "GeoClipmapStack::updateDepth");

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
		HardwarePixelBufferSharedPtr texBuf = mTextures[depth]->getBuffer();
		texBuf->lock(HardwareBuffer::LockOptions::HBL_NORMAL);
		const PixelBox& pixelBox = texBuf->getCurrentLock();
		//Image::Box visibleTexBox(visibleTextureRect.left, visibleTextureRect.top, visibleTextureRect.right, visibleTextureRect.bottom);
		//const PixelBox& pb = texBuf->lock(visibleTexBox, HardwareBuffer::LockOptions::HBL_NORMAL);

		// load bmp
		Bitmap bmpTex(mTextureNames[depth]);
		// then copy heightMapRect to visibleTextureRect
		bmpTex.copyData(heightMapRect, pixelBox, visibleTextureRect);

		texBuf->unlock();
	} catch(...) {

	}








	//	try {
	//		HardwarePixelBufferSharedPtr texBuf = mTextures[depth]->getBuffer();
	//		//char* hwbuf = (char*)texBuf->lock(0, mActiveClipmapSize * mActiveClipmapSize * 4, HardwareBuffer::LockOptions::HBL_NORMAL);
	//		Image::Box rect(0, 0, mActiveClipmapSize, mActiveClipmapSize);
	//		const PixelBox& pb = texBuf->lock(rect);
	//		
	//		//BMPCopy(mTextureNames[depth], x0, y0, mActiveClipmapSize, mActiveClipmapSize, 0, 0, mActiveClipmapSize, mActiveClipmapSize, hwbuf);
	//		texBuf->unlock();
	//	} catch(...) {
	//
	//	}


	/*Image img;
	img.load(mTextureNames[depth - 1], ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	HardwarePixelBufferSharedPtr texBuf = mTextures[depth - 1]->getBuffer();
	x0 = clamp(x0, 0, img.getWidth());
	x1 = clamp(x1, 0, img.getWidth());
	y0 = clamp(y0, 0, img.getHeight());
	y1 = clamp(y1, 0, img.getHeight());*/
	//texBuf->blitFromMemory(img.getPixelBox().getSubVolume(Box(x0, y0, x1, y1)));
}
