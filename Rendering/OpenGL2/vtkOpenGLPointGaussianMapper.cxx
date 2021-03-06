/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLPointGaussianMapper.h"

#include "vtkglVBOHelper.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"

#include "vtkPointGaussianVS.h"
#include "vtkglPolyDataFSNoLighting.h"

using vtkgl::replace;


class vtkOpenGLPointGaussianMapperHelper : public vtkOpenGLPolyDataMapper
{
public:
  static vtkOpenGLPointGaussianMapperHelper* New();
  vtkTypeMacro(vtkOpenGLPointGaussianMapperHelper, vtkOpenGLPolyDataMapper)

  vtkPointGaussianMapper *Owner;

protected:
  vtkOpenGLPointGaussianMapperHelper();
  ~vtkOpenGLPointGaussianMapperHelper();

  // Description:
  // Create the basic shaders before replacement
  virtual void GetShaderTemplate(std::string &VertexCode,
                           std::string &fragmentCode,
                           std::string &geometryCode,
                           int lightComplexity,
                           vtkRenderer *ren, vtkActor *act);

  // Description:
  // Perform string replacments on the shader templates
  virtual void ReplaceShaderValues(std::string &VertexCode,
                           std::string &fragmentCode,
                           std::string &geometryCode,
                           int lightComplexity,
                           vtkRenderer *ren, vtkActor *act);

  // Description:
  // Set the shader parameters related to the Camera
  virtual void SetCameraShaderParameters(vtkgl::CellBO &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Set the shader parameters related to the actor/mapper
  virtual void SetMapperShaderParameters(vtkgl::CellBO &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Update the VBO to contain point based values
  virtual void UpdateVBO(vtkRenderer *ren, vtkActor *act);

  virtual void RenderPieceDraw(vtkRenderer *ren, vtkActor *act);

private:
  vtkOpenGLPointGaussianMapperHelper(const vtkOpenGLPointGaussianMapperHelper&); // Not implemented.
  void operator=(const vtkOpenGLPointGaussianMapperHelper&); // Not implemented.
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLPointGaussianMapperHelper)

//-----------------------------------------------------------------------------
vtkOpenGLPointGaussianMapperHelper::vtkOpenGLPointGaussianMapperHelper()
{
  this->Owner = NULL;
}


//-----------------------------------------------------------------------------
void vtkOpenGLPointGaussianMapperHelper::GetShaderTemplate(std::string &VSSource,
                                          std::string &FSSource,
                                          std::string &GSSource,
                                          int vtkNotUsed(lightComplexity),
                                          vtkRenderer* vtkNotUsed(ren),
                                          vtkActor *vtkNotUsed(actor))
{
  VSSource = vtkPointGaussianVS;
  FSSource = vtkglPolyDataFSNoLighting;
  GSSource = "";
}

void vtkOpenGLPointGaussianMapperHelper::ReplaceShaderValues(std::string &VSSource,
                                                 std::string &FSSource,
                                                 std::string &GSSource,
                                                 int lightComplexity,
                                                 vtkRenderer* ren,
                                                 vtkActor *actor)
{
  FSSource = replace(FSSource,
    "//VTK::PositionVC::Dec",
    "varying vec2 offsetVC;");

  FSSource = replace(FSSource,"//VTK::Color::Impl",
    // compute the eye position and unit direction
    "//VTK::Color::Impl\n"
    "  float dist2 = dot(offsetVC.xy,offsetVC.xy);\n"
    "  float gaussian = exp(-0.5*16.0*dist2);\n"
//    "  diffuseColor = vertexColor.rgb;\n"
//    "  ambientColor = vertexColor.rgb;\n"
    "  opacity = opacity*gaussian;"
    , false);

  this->Superclass::ReplaceShaderValues(VSSource,FSSource,GSSource,lightComplexity,ren,actor);
}

//-----------------------------------------------------------------------------
vtkOpenGLPointGaussianMapperHelper::~vtkOpenGLPointGaussianMapperHelper()
{
}

//-----------------------------------------------------------------------------
void vtkOpenGLPointGaussianMapperHelper::SetCameraShaderParameters(vtkgl::CellBO &cellBO,
                                                    vtkRenderer* ren, vtkActor *actor)
{
  // do the superclass and then reset a couple values
  this->Superclass::SetCameraShaderParameters(cellBO,ren,actor);

  // add in uniforms for parallel and distance
  vtkCamera *cam = ren->GetActiveCamera();
  cellBO.Program->SetUniformi("cameraParallel", cam->GetParallelProjection());
}


//-----------------------------------------------------------------------------
void vtkOpenGLPointGaussianMapperHelper::SetMapperShaderParameters(vtkgl::CellBO &cellBO,
                                                         vtkRenderer *ren, vtkActor *actor)
{
  if (cellBO.indexCount && (this->OpenGLUpdateTime > cellBO.attributeUpdateTime ||
      cellBO.ShaderSourceTime > cellBO.attributeUpdateTime))
    {
    vtkgl::VBOLayout &layout = this->Layout;
    cellBO.vao.Bind();
    if (!cellBO.vao.AddAttributeArray(cellBO.Program, this->VBO,
                                    "offsetMC", layout.ColorOffset+sizeof(float),
                                    layout.Stride, VTK_FLOAT, 2, false))
      {
      vtkErrorMacro(<< "Error setting 'offsetMC' in shader VAO.");
      }
    }

  this->Superclass::SetMapperShaderParameters(cellBO,ren,actor);
}


namespace
{
// internal function called by CreateVBO
vtkgl::VBOLayout vtkOpenGLPointGaussianMapperHelperCreateVBO(float * points, vtkIdType numPts,
              unsigned char *colors, int colorComponents,
              float *sizes,
              vtkgl::BufferObject &vertexBuffer)
{
  vtkgl::VBOLayout layout;
  // Figure out how big each block will be, currently 6 floats.
  int blockSize = 3;  // x y z
  layout.VertexOffset = 0;
  layout.NormalOffset = 0;
  layout.TCoordOffset = 0;
  layout.TCoordComponents = 0;
  layout.ColorComponents = colorComponents;
  layout.ColorOffset = sizeof(float) * blockSize;
  ++blockSize; // color

  // two more floats
  blockSize += 2;  // offset
  layout.Stride = sizeof(float) * blockSize;

  // Create a buffer, and copy the data over.
  std::vector<float> packedVBO;
  packedVBO.resize(blockSize * numPts*3);
  std::vector<float>::iterator it = packedVBO.begin();

  float *pointPtr;
  unsigned char *colorPtr;

  float cos30 = cos(vtkMath::RadiansFromDegrees(30.0));

  for (vtkIdType i = 0; i < numPts; ++i)
    {
    pointPtr = points + i*3;
    colorPtr = colors + i*colorComponents;
    float radius = sizes[i];

    // Vertices
    *(it++) = pointPtr[0];
    *(it++) = pointPtr[1];
    *(it++) = pointPtr[2];
    *(it++) = *reinterpret_cast<float *>(colorPtr);
    *(it++) = -2.0f*radius*cos30;
    *(it++) = -radius;

    *(it++) = pointPtr[0];
    *(it++) = pointPtr[1];
    *(it++) = pointPtr[2];
    *(it++) = *reinterpret_cast<float *>(colorPtr);
    *(it++) = 2.0f*radius*cos30;
    *(it++) = -radius;

    *(it++) = pointPtr[0];
    *(it++) = pointPtr[1];
    *(it++) = pointPtr[2];
    *(it++) = *reinterpret_cast<float *>(colorPtr);
    *(it++) = 0.0f;
    *(it++) = 2.0f*radius;
    }
  vertexBuffer.Upload(packedVBO, vtkgl::BufferObject::ArrayBuffer);
  layout.VertexCount = numPts*3;
  return layout;
}
}

size_t vtkOpenGLPointGaussianMapperHelperCreateTriangleIndexBuffer(
  vtkgl::BufferObject &indexBuffer,
  int numPts)
{
  std::vector<unsigned int> indexArray;
  indexArray.reserve(numPts * 3);

  for (int i = 0; i < numPts*3; i++)
    {
    indexArray.push_back(i);
    }
  indexBuffer.Upload(indexArray, vtkgl::BufferObject::ElementArrayBuffer);
  return indexArray.size();
}

//-------------------------------------------------------------------------
void vtkOpenGLPointGaussianMapperHelper::UpdateVBO(vtkRenderer *vtkNotUsed(ren), vtkActor *act)
{
  vtkPolyData *poly = this->CurrentInput;

  if (poly == NULL)// || !poly->GetPointData()->GetNormals())
    {
    return;
    }

  // For vertex coloring, this sets this->Colors as side effect.
  // For texture map coloring, this sets ColorCoordinates
  // and ColorTextureMap as a side effect.
  // I moved this out of the conditional because it is fast.
  // Color arrays are cached. If nothing has changed,
  // then the scalars do not have to be regenerted.
  this->MapScalars(act->GetProperty()->GetOpacity());

  // Iterate through all of the different types in the polydata, building OpenGLs
  // and IBOs as appropriate for each type.
  this->Layout =
    vtkOpenGLPointGaussianMapperHelperCreateVBO(static_cast<float *>(poly->GetPoints()->GetVoidPointer(0)),
              poly->GetPoints()->GetNumberOfPoints(),
              this->Colors ? (unsigned char *)this->Colors->GetVoidPointer(0) : NULL,
              this->Colors ? this->Colors->GetNumberOfComponents() : 0,
              static_cast<float *>(poly->GetPointData()->GetArray(
                this->Owner->GetScaleArray())->GetVoidPointer(0)),
              this->VBO);

  // create the IBO
  this->Points.indexCount = 0;
  this->Lines.indexCount = 0;
  this->TriStrips.indexCount = 0;
  this->Tris.indexCount = this->Layout.VertexCount;
}

//-----------------------------------------------------------------------------
void vtkOpenGLPointGaussianMapperHelper::RenderPieceDraw(vtkRenderer* ren, vtkActor *actor)
{
  vtkgl::VBOLayout &layout = this->Layout;

  // draw polygons
  glDepthMask(GL_FALSE);
  glBlendFunc( GL_SRC_ALPHA, GL_ONE);  // additive for emissive sources
  if (layout.VertexCount)
    {
    // First we do the triangles, update the shader, set uniforms, etc.
    this->UpdateShader(this->Tris, ren, actor);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLuint>(layout.VertexCount));
    }
}


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLPointGaussianMapper)

//-----------------------------------------------------------------------------
vtkOpenGLPointGaussianMapper::vtkOpenGLPointGaussianMapper()
{
  this->Helper = vtkOpenGLPointGaussianMapperHelper::New();
  this->Helper->Owner = this;
}

vtkOpenGLPointGaussianMapper::~vtkOpenGLPointGaussianMapper()
{
  this->Helper->Delete();
  this->Helper = 0;
}

void vtkOpenGLPointGaussianMapper::RenderPiece(vtkRenderer *ren, vtkActor *act)
{
  if (this->GetMTime() > this->HelperUpdateTime)
    {
    this->Helper->SetInputData(this->GetInput());
    this->Helper->vtkPolyDataMapper::ShallowCopy(this);
    this->HelperUpdateTime.Modified();
    }
  this->Helper->RenderPiece(ren,act);
}

//-----------------------------------------------------------------------------
void vtkOpenGLPointGaussianMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
