vtk_module(vtkFiltersGeneric
  GROUPS
    StandAlone
  DEPENDS
    vtkFiltersCore
    vtkFiltersSources
  TEST_DEPENDS
    vtkIOXML
    vtkRenderingOpenGL
    vtkFiltersModeling
    vtkIOXML
    vtkRenderingLabel
    vtkTestingRendering
    vtkTestingGenericBridge)