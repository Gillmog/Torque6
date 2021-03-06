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

#include "console/consoleTypes.h"
#include "particleEmitter.h"
#include "graphics/core.h"
#include "rendering/rendering.h"

// Script bindings.
#include "particleEmitter_ScriptBinding.h"

// Debug Profiling.
#include "debug/profiler.h"

// bgfx/bx
#include <bgfx/bgfx.h>
#include <bx/fpumath.h>

// Assimp - Asset Import Library
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/types.h>

namespace Scene
{
   IMPLEMENT_PLUGIN_CONOBJECT(ParticleEmitter);

   bgfx::VertexBufferHandle ParticleEmitter::vertexBuffer = BGFX_INVALID_HANDLE;
   bgfx::IndexBufferHandle  ParticleEmitter::indexBuffer = BGFX_INVALID_HANDLE;

   ParticleEmitter::ParticleEmitter()
   {
      mCount = 100;
      mRange = 100;
      mSpeed = 0.1f;

      mShader.idx = bgfx::invalidHandle;
      mRenderData = NULL;
      mTexture.idx = bgfx::invalidHandle;
   }

   void ParticleEmitter::initPersistFields()
   {
      // Call parent.
      Parent::initPersistFields();

      addField("count", Torque::Con.TypeS32, Offset(mCount, ParticleEmitter), "");
      addField("range", Torque::Con.TypeS32, Offset(mRange, ParticleEmitter), "");
      addField("speed", Torque::Con.TypeF32, Offset(mSpeed, ParticleEmitter), "");
   }

   void ParticleEmitter::onAddToScene()
   {  
      setProcessTicks(true);

      // Load Shader
      Graphics::ShaderAsset* particleShaderAsset = Torque::Graphics.getShaderAsset("Particles:particleShader");
      if ( particleShaderAsset )
         mShader = particleShaderAsset->getProgram();

      // Load Texture
      TextureObject* texture_obj = Torque::Graphics.loadTexture("smoke.png", TextureHandle::BitmapKeepTexture, BGFX_TEXTURE_NONE, false, false);
      if ( texture_obj )
         mTexture = texture_obj->getBGFXTexture();

      refresh();
   }

   void ParticleEmitter::refresh()
   {
      Parent::refresh();

      if ( mShader.idx == bgfx::invalidHandle ||
           mTexture.idx == bgfx::invalidHandle ||
           mCount < 1 )
         return;

      if ( mRenderData == NULL )
         mRenderData = Torque::Rendering.createRenderData();

      mRenderData->indexBuffer = indexBuffer;
      mRenderData->vertexBuffer = vertexBuffer;

      // Render in Forward (for now) with our custom terrain shader.
      mRenderData->shader = mShader;
      mRenderData->flags = 0 | Rendering::RenderData::Transparent;
      mRenderData->state = 0
         | BGFX_STATE_RGB_WRITE
         | BGFX_STATE_ALPHA_WRITE
         | BGFX_STATE_DEPTH_TEST_LESS
         | BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ZERO, BGFX_STATE_BLEND_INV_SRC_ALPHA);

      // Transform of emitter.
      mRenderData->transformTable = &mTransformMatrix[0];
      mRenderData->transformCount = 1;

      // Instances
      mParticles.clear();
      mRenderData->instances = &mInstanceData;
   
      // Generate 50k particle instances for stress testing.
      for( S32 n = 0; n < mCount; ++n )
      {
         Particle part;
         emitParticle(&part);
         mParticles.push_back(part);
      }

      // Textures
      mTextureData.clear();
      mRenderData->textures = &mTextureData;
      Rendering::TextureData* texture = mRenderData->addTexture();
      texture->handle = mTexture;
      texture->uniform = Torque::Graphics.getTextureUniform(0);
   }

   void ParticleEmitter::interpolateTick( F32 delta )
   {  
      //
   }

   void ParticleEmitter::processTick()
   {  
      // 
   }

   void ParticleEmitter::advanceTime( F32 timeDelta )
   {  
      mInstanceData.clear();

      //
      for(S32 n = 0; n < mParticles.size(); ++n)
      {
         Particle* part = &mParticles[n];
         part->velocity += Point3F(0.0f, 0.0f, -9.81f) * timeDelta;
         part->position += part->velocity * timeDelta;

         part->lifetime -= timeDelta;
         if ( part->lifetime < 0 )
            emitParticle(part);

         Rendering::InstanceData particle;
         particle.i_data0.set(part->position.x, part->position.y, part->position.z, 0.0f);
         particle.i_data1.set(part->color.red, part->color.green, part->color.blue, mClampF(part->lifetime, 0.0f, 1.0f));
         mInstanceData.push_back(particle);
      }
   }

   void ParticleEmitter::emitParticle(Particle* part)
   {
      F32 range = (F32)mRange;
      part->position.set(mRandF(-range, range), mRandF(-range, range), mRandF(-range, range));
      part->velocity.set(mRandF(-5.0f, 5.0f), mRandF(-5.0f, 5.0f), mRandF(0.0f, 10.0f));
      part->color.set(mRandF(0.0f, 1.0f), mRandF(0.0f, 1.0f), mRandF(0.0f, 1.0f), 1.0f);
      part->lifetime = mRandF(1.0f, 10.0f);
   }
}
