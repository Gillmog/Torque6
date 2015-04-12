//-----------------------------------------------------------------------------
// Copyright (c) 2015 Andrew Mac
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "TerrainCell.h"
#include <plugins/plugins_shared.h>

#include <sim/simObject.h>
#include <3d/rendering/common.h>
#include <graphics/utilities.h>

#include <bx/fpumath.h>

Vector<TerrainCell> terrainGrid;

TerrainCell::TerrainCell(bgfx::TextureHandle* _megaTexture, Vector<Rendering::UniformData>* _uniformData, S32 _gridX, S32 _gridY)
{
   heightMap = NULL;
   blendMap = NULL;

   mTexture = _megaTexture;
   mUniformData = _uniformData;
   gridX = _gridX;
   gridY = _gridY;

   mDynamicVB.idx = bgfx::invalidHandle;
   mDynamicIB.idx = bgfx::invalidHandle;
   mVB.idx = bgfx::invalidHandle;
   mIB.idx = bgfx::invalidHandle;
   mBlendTexture.idx = bgfx::invalidHandle;

   mRenderData = NULL;

   maxTerrainHeight = 0;

   // Load Shader
   Graphics::ShaderAsset* terrainShaderAsset = Plugins::Link.Graphics.getShaderAsset("Terrain:terrainShader");
   if ( terrainShaderAsset )
      mShader = terrainShaderAsset->getProgram();
}

TerrainCell::~TerrainCell()
{
   SAFE_DELETE(heightMap);
   SAFE_DELETE(blendMap);

   if ( mVB.idx != bgfx::invalidHandle )
      Plugins::Link.bgfx.destroyVertexBuffer(mVB);

   if ( mIB.idx != bgfx::invalidHandle )
      Plugins::Link.bgfx.destroyIndexBuffer(mIB);

   if ( mDynamicVB.idx != bgfx::invalidHandle )
      Plugins::Link.bgfx.destroyDynamicVertexBuffer(mDynamicVB);

   if ( mDynamicIB.idx != bgfx::invalidHandle )
      Plugins::Link.bgfx.destroyDynamicIndexBuffer(mDynamicIB);

   if ( mBlendTexture.idx != bgfx::invalidHandle )
      Plugins::Link.bgfx.destroyTexture(mBlendTexture);
}

void TerrainCell::refreshVertexBuffer()
{
   if ( mVerts.size() <= 0 ) return;

   if ( mVB.idx != bgfx::invalidHandle )
      Plugins::Link.bgfx.destroyVertexBuffer(mVB);

   const bgfx::Memory* mem;
   mem = Plugins::Link.bgfx.makeRef(&mVerts[0], sizeof(PosUVColorVertex) * mVerts.size(), NULL, NULL );
   mVB = Plugins::Link.bgfx.createVertexBuffer(mem, *Plugins::Link.Graphics.PosUVColorVertex, BGFX_BUFFER_NONE);

   //const bgfx::Memory* mem;
   //mem = Plugins::Link.bgfx.makeRef(&mVerts[0], sizeof(PosUVColorVertex) * mVerts.size(), NULL, NULL );

   //if ( mDynamicVB.idx == bgfx::invalidHandle )
   //   mDynamicVB = Plugins::Link.bgfx.createDynamicVertexBuffer(mem, *Plugins::Link.Graphics.PosUVColorVertex, BGFX_BUFFER_NONE);
   //else
   //   Plugins::Link.bgfx.updateDynamicVertexBuffer(mDynamicVB, mem);
}

void TerrainCell::refreshIndexBuffer()
{
   if ( mIndices.size() <= 0 ) return;

   if ( mIB.idx != bgfx::invalidHandle )
      Plugins::Link.bgfx.destroyIndexBuffer(mIB);

   const bgfx::Memory* mem;
	mem = Plugins::Link.bgfx.makeRef(&mIndices[0], sizeof(uint16_t) * mIndices.size(), NULL, NULL );
   mIB = Plugins::Link.bgfx.createIndexBuffer(mem, BGFX_BUFFER_NONE);

   //const bgfx::Memory* mem;
	//mem = Plugins::Link.bgfx.makeRef(&mIndices[0], sizeof(uint16_t) * mIndices.size(), NULL, NULL );

   //if ( mDynamicIB.idx == bgfx::invalidHandle )
   //   mDynamicIB = Plugins::Link.bgfx.createDynamicIndexBuffer(mem, BGFX_BUFFER_NONE);
   //else
   //   Plugins::Link.bgfx.updateDynamicIndexBuffer(mDynamicIB, mem);
}

void TerrainCell::refreshBlendMap()
{
   const bgfx::Memory* mem;

	mem = Plugins::Link.bgfx.alloc(width * height * 4);
   dMemcpy(mem->data, &blendMap[0].red, width * height * 4);

   if ( mBlendTexture.idx == bgfx::invalidHandle )
      mBlendTexture = Plugins::Link.bgfx.createTexture2D(width, height, 0, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_U_CLAMP | BGFX_TEXTURE_V_CLAMP, mem);
   else
      Plugins::Link.bgfx.updateTexture2D(mBlendTexture, 0, 0, 0, width, height, mem, width * 4);
}

void TerrainCell::loadEmptyTerrain(S32 _width, S32 _height)
{
   height = _height;
   width = _width;
   heightMap = new F32[height * width];
   blendMap = new ColorI[height * width];
   dMemset(heightMap, 0, height * width * sizeof(F32));
   maxTerrainHeight = 0;
}

void TerrainCell::loadHeightMap(const char* path)
{
   GBitmap *bmp = dynamic_cast<GBitmap*>(Plugins::Link.ResourceManager->loadInstance(path));  
   if(bmp != NULL)
   {
      height = (bmp->getHeight() / 2) * 2;
      width = (bmp->getWidth() / 2) * 2;
      heightMap = new F32[height * width];
      blendMap = new ColorI[height * width];

      for(U32 y = 0; y < height; y++)
      {
         for(U32 x = 0; x < width; x++)
         {
            ColorI heightSample;
            bmp->getColor(x, y, heightSample);
            heightMap[(y * width) + x] = ((F32)heightSample.red) * 0.1f;

            // Temp blendmap for testing
            // Note: textures are loaded in as BGRA, so Layer2, Layer1, Layer0, Layer4
            if ( heightMap[(y * width) + x] > 10 )
               blendMap[(y * width) + x].set(0, 200, 55, 0);
            else
               blendMap[(y * width) + x].set(0, 0, 255, 0);

            if ( heightMap[(y * width) + x] > maxTerrainHeight )
               maxTerrainHeight = heightMap[(y * width) + x];
         }
      }

      rebuild();
   }
}

Point3F TerrainCell::getWorldSpacePos(U32 x, U32 y)
{
   U32 pos = (y * width) + x;
   if ( pos < 0 ) return Point3F::Zero;
   if ( pos >= (width * height) ) return Point3F::Zero;

   F32 heightValue = heightMap[pos];
   Point3F worldPos;
   worldPos.set(gridX * width - (1 * gridX) + x, heightValue, gridY * height - (2 * gridY) + y);
   return worldPos;
}

void TerrainCell::rebuild()
{
   mVerts.clear();
   mIndices.clear();

   for(U32 y = 0; y < height; y++ )
   {
      for(U32 x = 0; x < width; x++ )
      {
         PosUVColorVertex vert = {x, heightMap[(y * width) + x], y, (F32)x / (F32)width, (F32)y / (F32)height, 0xffffffff };
         mVerts.push_back(vert);
      }
   }

   for(U32 y = 0; y < (height - 1); y++ )
   {
      U32 y_offset = (y * width);
      for(U32 x = 0; x < (width - 1); x++ )
      {
         mIndices.push_back(y_offset + x);
         mIndices.push_back(y_offset + x + 1);
         mIndices.push_back(y_offset + x + width);
         mIndices.push_back(y_offset + x + 1);
         mIndices.push_back(y_offset + x + width + 1);
         mIndices.push_back(y_offset + x + width);
      }
   }

   refreshVertexBuffer();
   refreshIndexBuffer();
   refreshBlendMap();
   refresh();
}

void TerrainCell::refresh()
{
   if ( mRenderData == NULL )
      mRenderData = Plugins::Link.Rendering.createRenderData();

   //mRenderData->isDynamic = true;
   //mRenderData->dynamicIndexBuffer = mDynamicIB;
   //mRenderData->dynamicVertexBuffer = mDynamicVB;
   mRenderData->indexBuffer = mIB;
   mRenderData->vertexBuffer = mVB;

   mTextureData.clear();
   mRenderData->textures = &mTextureData;

   Rendering::TextureData* layer = mRenderData->addTexture();
   layer->uniform = Plugins::Link.Graphics.getTextureUniform(0);
   layer->handle = *mTexture;

   // Render in Forward (for now) with our custom terrain shader.
   mRenderData->shader = mShader;
   mRenderData->view = Graphics::ViewTable::RenderLayer2;
   mRenderData->uniforms.uniforms = mUniformData;

   // Transform
   F32* cubeMtx = new F32[16];
   bx::mtxSRT(cubeMtx, 1, 1, 1, 0, 0, 0, gridX * width - (1 * gridX), 0, gridY * height - (1 * gridY));
   mRenderData->transformTable = cubeMtx;
   mRenderData->transformCount = 1;
}

void stitchEdges(SimObject *obj, S32 argc, const char *argv[])
{
   for(U32 i = 0; i < terrainGrid.size(); ++i)
   {
      TerrainCell* curCell = &terrainGrid[i];

      for(U32 n = 0; n < terrainGrid.size(); ++n)
      {
         TerrainCell* compareCell = &terrainGrid[n];
         
         // Left
         if ( compareCell->gridX == (curCell->gridX - 1) && compareCell->gridY == curCell->gridY )
         {
            for(U32 y = 0; y < curCell->height; ++y)
            {
               U32 left_index = ((y + 1) * compareCell->width) - 1;
               U32 right_index = y * curCell->width;
               F32 average_height = (compareCell->heightMap[left_index] + curCell->heightMap[right_index]) / 2.0f;

               compareCell->heightMap[left_index] = average_height;
               curCell->heightMap[right_index] = average_height;
            }

            compareCell->rebuild();
            curCell->rebuild();
         }

         // Bottom
         if ( compareCell->gridY == (curCell->gridY - 1) && compareCell->gridX == curCell->gridX )
         {
            for(U32 x = 0; x < curCell->width; ++x)
            {
               U32 bottom_index = (curCell->width * (curCell->height - 2)) + x;
               U32 top_index = x;
               F32 average_height = (compareCell->heightMap[bottom_index] + curCell->heightMap[top_index]) / 2.0f;

               compareCell->heightMap[bottom_index] = average_height;
               curCell->heightMap[top_index] = average_height;
            }

            compareCell->rebuild();
            curCell->rebuild();
         }
      }
   }
}