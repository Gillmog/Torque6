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

#include "ssaoComponent.h"
#include "console/consoleInternal.h"
#include "graphics/dgl.h"
#include "graphics/shaders.h"
#include "graphics/core.h"
#include "scene/scene.h"

#include <bgfx/bgfx.h>
#include <bx/fpumath.h>
#include <bx/timer.h>

namespace Scene
{
   SSAOComponent::SSAOComponent()
   {
      //mName       = "SSAO";
      mPriority   = 4100;
      mBias       = 0.005f;
      mIntensity  = 1.0f;
      mScale      = 1.0f;
      mRadius     = 0.03f;

      // Buffers
      mOcclusionBuffer.idx     = bgfx::invalidHandle;
      mOcclusionBlurBuffer.idx = bgfx::invalidHandle;

      // Uniforms
      mSSAOParamsUniform = bgfx::createUniform("u_SSAOParams", bgfx::UniformType::Vec4);

      // Views
      mAccumulateView   = NULL;
      mBlurXView        = NULL;
      mBlurYView        = NULL;
      mApplyView        = NULL;

      // Shaders
      mAccumulateShader = Graphics::getDefaultShader("components/ssao/ssao_vs.tsh", "components/ssao/ssao_accumulate_fs.tsh");
      mBlurXShader      = Graphics::getDefaultShader("components/ssao/ssao_vs.tsh", "components/ssao/ssao_blurx_fs.tsh");
      mBlurYShader      = Graphics::getDefaultShader("components/ssao/ssao_vs.tsh", "components/ssao/ssao_blury_fs.tsh");
      mApplyShader      = Graphics::getDefaultShader("components/ssao/ssao_vs.tsh", "components/ssao/ssao_apply_fs.tsh");

      initBuffers();
   }

   SSAOComponent::~SSAOComponent()
   {
      destroyBuffers();
   }

   /*void SSAO::initPersistFields()
   {
      // Call parent.
      Parent::initPersistFields();

      addGroup("SSAO");

         addField("Bias", TypeF32, Offset(mBias, SSAO), "");
         addField("Intensity", TypeF32, Offset(mIntensity, SSAO), "");
         addField("Scale", TypeF32, Offset(mScale, SSAO), "");
         addField("Radius", TypeF32, Offset(mRadius, SSAO), "");

      endGroup("SSAO");
   }*/

   void SSAOComponent::initBuffers()
   {
      destroyBuffers();

      // Framebuffers
      mOcclusionBuffer = bgfx::createFrameBuffer(bgfx::BackbufferRatio::Equal, bgfx::TextureFormat::RGBA8);
      mOcclusionBlurBuffer = bgfx::createFrameBuffer(bgfx::BackbufferRatio::Equal, bgfx::TextureFormat::RGBA8);
   }

   void SSAOComponent::destroyBuffers()
   {
      if (isValid(mOcclusionBuffer))
         bgfx::destroyFrameBuffer(mOcclusionBuffer);
      if (isValid(mOcclusionBlurBuffer))
         bgfx::destroyFrameBuffer(mOcclusionBlurBuffer);
   }

   void SSAOComponent::onAddToCamera()
   {
      // Views
      mAccumulateView   = Graphics::getView("SSAO_Accumulate", 4100, mCamera);
      mBlurXView        = Graphics::getView("SSAO_BlurX");
      mBlurYView        = Graphics::getView("SSAO_BlurY");
      mApplyView        = Graphics::getView("SSAO_Apply");
   }

   void SSAOComponent::onRemoveFromCamera()
   {
      // Delete Views
      Graphics::deleteView(mAccumulateView);
      Graphics::deleteView(mBlurXView);
      Graphics::deleteView(mBlurYView);
      Graphics::deleteView(mApplyView);
   }

   void SSAOComponent::process()
   {
      F32 proj[16];
      bx::mtxOrtho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f);

      // Params
      float params[4] = { mBias, mIntensity, mScale, mRadius };
      bgfx::setUniform(mSSAOParamsUniform, params);

      // Accumulate
      bgfx::setViewTransform(mAccumulateView->id, NULL, proj);
      bgfx::setViewRect(mAccumulateView->id, 0, 0, mCamera->width, mCamera->height);
      bgfx::setViewFrameBuffer(mAccumulateView->id, mOcclusionBuffer);
      bgfx::setTexture(0, Graphics::Shader::getTextureUniform(0), mCamera->getRenderPath()->getDepthTexture());
      bgfx::setTexture(1, Graphics::Shader::getTextureUniform(1), mCamera->getRenderPath()->getNormalTexture());
      bgfx::setTexture(2, Graphics::Shader::getTextureUniform(2), Graphics::noiseTexture);
      bgfx::setState(BGFX_STATE_RGB_WRITE | BGFX_STATE_ALPHA_WRITE);
      fullScreenQuad((float)mCamera->width, (float)mCamera->height);
      bgfx::submit(mAccumulateView->id, mAccumulateShader->mProgram);

      // Blur X
      bgfx::setViewTransform(mBlurXView->id, NULL, proj);
      bgfx::setViewRect(mBlurXView->id, 0, 0, mCamera->width, mCamera->height);
      bgfx::setViewFrameBuffer(mBlurXView->id, mOcclusionBlurBuffer);
      bgfx::setTexture(0, Graphics::Shader::getTextureUniform(0), mOcclusionBuffer);
      bgfx::setTexture(1, Graphics::Shader::getTextureUniform(1), mCamera->getRenderPath()->getNormalTexture());
      bgfx::setState(BGFX_STATE_RGB_WRITE | BGFX_STATE_ALPHA_WRITE);
      fullScreenQuad((float)mCamera->width, (float)mCamera->height);
      bgfx::submit(mBlurXView->id, mBlurXShader->mProgram);

      // Blur Y
      bgfx::setViewTransform(mBlurYView->id, NULL, proj);
      bgfx::setViewRect(mBlurYView->id, 0, 0, mCamera->width, mCamera->height);
      bgfx::setViewFrameBuffer(mBlurYView->id, mOcclusionBuffer);
      bgfx::setTexture(0, Graphics::Shader::getTextureUniform(0), mOcclusionBlurBuffer);
      bgfx::setTexture(1, Graphics::Shader::getTextureUniform(1), mCamera->getRenderPath()->getNormalTexture());
      bgfx::setState(BGFX_STATE_RGB_WRITE | BGFX_STATE_ALPHA_WRITE);
      fullScreenQuad((float)mCamera->width, (float)mCamera->height);
      bgfx::submit(mBlurYView->id, mBlurYShader->mProgram);

      // Apply
      bgfx::setViewTransform(mApplyView->id, NULL, proj);
      bgfx::setViewRect(mApplyView->id, 0, 0, mCamera->width, mCamera->height);
      bgfx::setViewFrameBuffer(mApplyView->id, mCamera->getPostTarget());
      bgfx::setTexture(0, Graphics::Shader::getTextureUniform(0), mCamera->getPostSource());
      bgfx::setTexture(1, Graphics::Shader::getTextureUniform(1), mOcclusionBuffer);
      bgfx::setState(BGFX_STATE_RGB_WRITE | BGFX_STATE_ALPHA_WRITE);
      fullScreenQuad((float)mCamera->width, (float)mCamera->height);
      bgfx::submit(mApplyView->id, mApplyShader->mProgram);
   }

   void SSAOComponent::resize()
   {

   }
}
