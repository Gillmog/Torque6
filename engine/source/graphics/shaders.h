//-----------------------------------------------------------------------------
// Copyright (c) 2013 GarageGames, LLC
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

#ifndef _SHADERS_H_
#define _SHADERS_H_

#ifndef _FILEOBJECT_H_
#include "io/fileObject.h"
#endif

#ifndef BGFX_H_HEADER_GUARD
#include <bgfx.h>
#endif

#ifndef _HASHTABLE_H
#include "collection/hashTable.h"
#endif

namespace Graphics
{

   class Shader
   {
      public:
         Shader(const char* vertex_shader_path, const char* fragment_shader_path);
         ~Shader();

         FileObject mVertexShaderFile;
         FileObject mPixelShaderFile;

         bgfx::ShaderHandle mVertexShader;
         bgfx::ShaderHandle mPixelShader;

         bgfx::ProgramHandle mProgram;

         static bgfx::UniformHandle textureUniforms[16];
         static bgfx::UniformHandle getTextureUniform(U32 slot);
         static HashMap<const char*, bgfx::UniformHandle> uniformMap;
         static bgfx::UniformHandle getUniform(const char* name);
         static bgfx::UniformHandle getUniformArray(const char* name, U32 count);
   };

   void initUniforms();
   void destroyUniforms();

}
#endif //_SHADERS_H_
