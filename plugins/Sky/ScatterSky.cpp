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

#include "ScatterSky.h"
#include "graphics/core.h"
#include "lighting/lighting.h"
#include "plugins/plugins_shared.h"
#include "rendering/rendering.h"
#include "sim/simObject.h"

#include <bx/fpumath.h>

IMPLEMENT_PLUGIN_CONOBJECT(ScatterSkyComponent);

ScatterSkyComponent::ScatterSkyComponent()
   : mEnabled(false),
	  mSkyCubeReady(false)
{
   //mName = "Scatter Sky";

   // Load Skybox Shader
   mShader.idx = bgfx::invalidHandle;
   Graphics::ShaderAsset* skyboxShaderAsset = Torque::Graphics.getShaderAsset("Sky:skyboxShader");
   if (skyboxShaderAsset)
   {
      mShader = skyboxShaderAsset->getProgram();
      mMatrixUniform = Torque::Graphics.getUniformMat4("u_mtx", 1);
   }

	// Load Scatter Sky Shader
	mGenerateSkyCubeShader.idx = bgfx::invalidHandle;
	Graphics::ShaderAsset* scatterSkyShaderAsset = Torque::Graphics.getShaderAsset("Sky:scatterSkyShader");
	if (scatterSkyShaderAsset)
	{
		mGenerateSkyCubeShader = scatterSkyShaderAsset->getProgram();
		mCubeParamsUniform = Torque::Graphics.getUniformMat4("u_cubeParams", 1);
      mSkyParams1Uniform = Torque::Graphics.getUniformMat4("u_skyParams1", 1);
      mSkyParams2Uniform = Torque::Graphics.getUniformMat4("u_skyParams2", 1);
      mSkyParams3Uniform = Torque::Graphics.getUniformMat4("u_skyParams3", 1);
      mSkyParams4Uniform = Torque::Graphics.getUniformMat4("u_skyParams4", 1);
      mSkyParams5Uniform = Torque::Graphics.getUniformMat4("u_skyParams5", 1);
	}

   mView    = NULL;
	mTexture = Torque::bgfx.createTextureCube(512, 1, bgfx::TextureFormat::RGBA16, BGFX_TEXTURE_RT, NULL);

   // Default Settings
   mIntensity                 = 2.0f;
   mSunBrightness             = 1000.0f;
   mSurfaceHeight             = 0.994f;
   mScatterStrength           = 0.028f;
   mMieBrightness             = 0.1f;
   mMieDistribution           = 0.63f;
   mMieCollectionPower        = 0.39f;
   mMieStrength               = 0.0264f;
   mRayleighBrightness        = 3.3f;
   mRayleighCollectionPower   = 0.81f;
   mRayleighStrength          = 0.139f;
   mStepCount                 = 6.0f;
   mAirColor.set(0.18867780436772762f, 0.4978442963618773f, 0.6616065586417131f);

	mGenerateSkyCube = true; 

   for (U32 side = 0; side < 6; ++side)
   {
      mTempSkyCubeView[side] = NULL;
      mTempSkyCubeBuffers[side].idx = bgfx::invalidHandle;
   }
}

ScatterSkyComponent::~ScatterSkyComponent()
{
   if (bgfx::isValid(mTexture))
      Torque::bgfx.destroyTexture(mTexture);
}

void ScatterSkyComponent::initPersistFields()
{
   // Call parent.
   Parent::initPersistFields();

   addGroup("Scatter Sky");

      addField("Intensity",               Torque::Con.TypeF32, Offset(mIntensity, ScatterSkyComponent), "");
      addField("SunBrightness",           Torque::Con.TypeF32, Offset(mSunBrightness, ScatterSkyComponent), "");
      addField("SurfaceHeight",           Torque::Con.TypeF32, Offset(mSurfaceHeight, ScatterSkyComponent), "");
      addField("ScatterStrength",         Torque::Con.TypeF32, Offset(mScatterStrength, ScatterSkyComponent), "");
      addField("MieBrightness",           Torque::Con.TypeF32, Offset(mMieBrightness, ScatterSkyComponent), "");
      addField("MieDistribution",         Torque::Con.TypeF32, Offset(mMieDistribution, ScatterSkyComponent), "");
      addField("MieCollectionPower",      Torque::Con.TypeF32, Offset(mMieCollectionPower, ScatterSkyComponent), "");
      addField("MieStrength",             Torque::Con.TypeF32, Offset(mMieStrength, ScatterSkyComponent), "");
      addField("RayleighBrightness",      Torque::Con.TypeF32, Offset(mRayleighBrightness, ScatterSkyComponent), "");
      addField("RayleighCollectionPower", Torque::Con.TypeF32, Offset(mRayleighCollectionPower, ScatterSkyComponent), "");
      addField("RayleighStrength",        Torque::Con.TypeF32, Offset(mRayleighStrength, ScatterSkyComponent), "");
      addField("StepCount",               Torque::Con.TypeF32, Offset(mStepCount, ScatterSkyComponent), "");
      addField("AirColor",                Torque::Con.TypeColorF, Offset(mAirColor, ScatterSkyComponent), "");

   endGroup("Scatter Sky");
}

void ScatterSkyComponent::onAddToScene()
{
   Torque::Rendering.addRenderHook(this);
   mEnabled = true;
}

void ScatterSkyComponent::onRemoveFromScene()
{
   Torque::Rendering.removeRenderHook(this);
   mEnabled = false;
}

void ScatterSkyComponent::refresh()
{
   //mGenerateSkyCube = true;
}

void ScatterSkyComponent::preRender(Rendering::RenderCamera* camera)
{
   // Generate SkyCube Begin
   if (mGenerateSkyCube)
      generateSkyCubeBegin();

   mView = Torque::Graphics.getView("Skybox", 2010, camera);
}

void ScatterSkyComponent::render(Rendering::RenderCamera* camera)
{
   // Generate SkyCube
   if (mGenerateSkyCube)
      generateSkyCube();

   // Render SkyCube
   if (!mEnabled || !bgfx::isValid(mTexture) || !bgfx::isValid(mShader) || !mSkyCubeReady)
      return;

   F32 proj[16];
   bx::mtxOrtho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1000.0f);
   Torque::bgfx.setViewFrameBuffer(mView->id, camera->getRenderPath()->getBackBuffer());
   Torque::bgfx.setViewTransform(mView->id, NULL, proj, BGFX_VIEW_STEREO, NULL);
   Torque::bgfx.setViewRect(mView->id, 0, 0, camera->width, camera->height);

   // Calculate view matrix based on current view matrix.
   float viewMtx[16];
   bx::mtxInverse(viewMtx, camera->viewMatrix);
   Torque::bgfx.setUniform(mMatrixUniform, viewMtx, 1);

   Torque::bgfx.setTexture(0, Torque::Graphics.getTextureUniform(0), mTexture, UINT32_MAX);
   Torque::bgfx.setState(0 | BGFX_STATE_RGB_WRITE | BGFX_STATE_ALPHA_WRITE | BGFX_STATE_DEPTH_TEST_LESS, 0);

   // Render skybox as fullscreen quad.
   Torque::Graphics.fullScreenQuad((F32)camera->width, (F32)camera->height, 999.999f);
   Torque::bgfx.submit(mView->id, mShader, 0, false);
}

void ScatterSkyComponent::postRender(Rendering::RenderCamera* camera)
{
   // Generate SkyCube End
   if (mGenerateSkyCube)
      generateSkyCubeEnd();
}

void ScatterSkyComponent::generateSkyCubeBegin()
{
   // Initialize temporary buffers to use to generate sky cube.
   for (U32 side = 0; side < 6; ++side)
   {
      char viewName[64];
      dSprintf(viewName, 64, "GenerateSkyCubeSide%d", side);

      mTempSkyCubeView[side] = Torque::Graphics.getTemporaryView(Torque::StringTableLink->insert(viewName), 350, NULL);
      bgfx::Attachment fbAttachment;
      fbAttachment.handle = mTexture;
      fbAttachment.layer = side;
      fbAttachment.mip = 0;
      mTempSkyCubeBuffers[side] = Torque::bgfx.createFrameBufferA(1, &fbAttachment, false);
   }
}

void ScatterSkyComponent::generateSkyCube()
{
   // Uniforms
   F32 skyParams1[4] = { mIntensity, mSunBrightness, mSurfaceHeight, mScatterStrength };
   Torque::bgfx.setUniform(mSkyParams1Uniform, skyParams1, 1);
   F32 skyParams2[4] = { mMieBrightness, mMieDistribution, mMieCollectionPower, mMieStrength };
   Torque::bgfx.setUniform(mSkyParams2Uniform, skyParams2, 1);
   F32 skyParams3[4] = { mRayleighBrightness, mRayleighCollectionPower, mRayleighStrength, 0.0f };
   Torque::bgfx.setUniform(mSkyParams3Uniform, skyParams3, 1);
   F32 skyParams4[4] = { mStepCount, Torque::Lighting.directionalLight->direction.x, Torque::Lighting.directionalLight->direction.z, -1.0f * Torque::Lighting.directionalLight->direction.y };
   Torque::bgfx.setUniform(mSkyParams4Uniform, skyParams4, 1);
   F32 skyParams5[4] = { mAirColor.red, mAirColor.green, mAirColor.blue, 0.0f };
   Torque::bgfx.setUniform(mSkyParams5Uniform, skyParams5, 1);

   // Process
   for (U16 side = 0; side < 6; ++side)
   {
      F32 generateParams[4] = { (F32)side, 0.0f, 0.0f, 0.0f };
      Torque::bgfx.setUniform(mCubeParamsUniform, generateParams, 1);

      // This projection matrix is used because its a full screen quad.
      F32 proj[16];
      bx::mtxOrtho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f);
      Torque::bgfx.setViewTransform(mTempSkyCubeView[side]->id, NULL, proj, BGFX_VIEW_STEREO, NULL);
      Torque::bgfx.setViewRect(mTempSkyCubeView[side]->id, 0, 0, 512, 512);
      Torque::bgfx.setViewFrameBuffer(mTempSkyCubeView[side]->id, mTempSkyCubeBuffers[side]);
      Torque::bgfx.setState(0 | BGFX_STATE_RGB_WRITE | BGFX_STATE_ALPHA_WRITE, 0);

      Torque::Graphics.fullScreenQuad(512.0f, 512.0f, 0.0f);
      Torque::bgfx.submit(mTempSkyCubeView[side]->id, mGenerateSkyCubeShader, 0, false);
   }

   mSkyCubeReady = true;
}

void ScatterSkyComponent::generateSkyCubeEnd()
{
   for (U32 side = 0; side < 6; ++side)
   {
      if (bgfx::isValid(mTempSkyCubeBuffers[side]))
         Torque::bgfx.destroyFrameBuffer(mTempSkyCubeBuffers[side]);
   }

   mGenerateSkyCube = false;
}