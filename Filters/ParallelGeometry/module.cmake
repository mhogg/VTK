vtk_module(vtkFiltersParallelGeometry
  GROUPS
    MPI
  DEPENDS
    vtkFiltersGeometry
    vtkParallelMPI
  TEST_DEPENDS
    vtkIOXML
    vtkIOParallel
    vtkCommonDataModel
    vtkParallelMPI
  )