catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Generate marching cubes head model (full resolution)

# get the interactor ui and colors
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# create pipeline
# reader reads slices
vtkVolume16Reader v16
    v16 SetDataDimensions 256 256
    v16 SetDataByteOrderToLittleEndian
    v16 SetFilePrefix "$VTK_DATA/fullHead/headsq"
    v16 SetDataSpacing 0.8 0.8 1.5
    v16 SetImageRange 30 50
    v16 SetDataMask 0x7fff

# write isosurface to file
#vtkSynchronizedTemplates3D stemp
vtkKitwareContourFilter stemp
    stemp SetInput [v16 GetOutput]
    stemp SetValue 0 1150
    stemp Update


vtkPolyDataMapper mapper
    mapper SetInput [stemp GetOutput]
	mapper ScalarVisibilityOff
    
vtkActor head
    head SetMapper mapper
    eval [head GetProperty] SetColor $raw_sienna

# Create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor head
ren1 SetBackground 1 1 1
renWin SetSize 500 500
eval ren1 SetBackground $slate_grey
[ren1 GetActiveCamera] Zoom 1.5
[ren1 GetActiveCamera] Elevation 90
[ren1 GetActiveCamera] OrthogonalizeViewUp

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

iren Initialize

#renWin SetFileName "genHead.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

