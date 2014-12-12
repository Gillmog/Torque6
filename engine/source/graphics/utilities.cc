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

#include "graphics/utilities.h"
#include <bgfx.h>

namespace Graphics
{
   bgfx::VertexDecl PosTexcoordVertex::ms_decl;
   bgfx::VertexDecl PosColorVertex::ms_decl;
   bgfx::VertexBufferHandle cubeVB;
   bgfx::IndexBufferHandle  cubeIB;

   // Common Shape: Cube
   static Graphics::PosColorVertex s_cubeVertices[8] =
   {
	   {-1.0f,  1.0f,  1.0f, 0xffffffff },
	   { 1.0f,  1.0f,  1.0f, 0xffffffff },
	   {-1.0f, -1.0f,  1.0f, 0xffffffff },
	   { 1.0f, -1.0f,  1.0f, 0xffffffff },
	   {-1.0f,  1.0f, -1.0f, 0xffffffff },
	   { 1.0f,  1.0f, -1.0f, 0xffffffff },
	   {-1.0f, -1.0f, -1.0f, 0xffffffff },
	   { 1.0f, -1.0f, -1.0f, 0xffffffff },
   };

   static const uint16_t s_cubeIndices[36] =
   {
	   0, 1, 2, // 0
	   1, 3, 2,
	   4, 6, 5, // 2
	   5, 6, 7,
	   0, 2, 4, // 4
	   4, 2, 6,
	   1, 5, 3, // 6
	   5, 7, 3,
	   0, 4, 1, // 8
	   4, 5, 1,
	   2, 3, 6, // 10
	   6, 3, 7,
   };

   void initUtilities()
   {
      // Vertex Layouts
      PosTexcoordVertex::init();
      PosColorVertex::init();

      // Common Shapes
      const bgfx::Memory* mem;

      // Create static vertex buffer.
      cubeVB.idx = bgfx::invalidHandle;
	   mem = bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices) );
	   cubeVB = bgfx::createVertexBuffer(mem, Graphics::PosColorVertex::ms_decl);

	   // Create static index buffer.
      cubeIB.idx = bgfx::invalidHandle;
	   mem = bgfx::makeRef(s_cubeIndices, sizeof(s_cubeIndices) );
	   cubeIB = bgfx::createIndexBuffer(mem);
   }

   void destroyUtilities()
   {
      if ( cubeVB.idx != bgfx::invalidHandle )
         bgfx::destroyVertexBuffer(cubeVB);
      if ( cubeIB.idx != bgfx::invalidHandle )
         bgfx::destroyIndexBuffer(cubeIB);
   }
}