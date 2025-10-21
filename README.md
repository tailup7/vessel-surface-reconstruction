# environment
+ Windows11
+ Visual Studio 2022
+ 

# vessel-surface-reconstruction

<p align="left">
  <img src="pictures/stenosis.png" width="40%">
</p>


# useful libraries; VTK

https://examples.vtk.org/site/Cxx/VisualizationAlgorithms/TubesWithVaryingRadiusAndColors/

https://examples.vtk.org/site/Cxx/VisualizationAlgorithms/TubesFromSplines/

https://examples.vtk.org/site/Cxx/PolyData/TubeFilter/

# usage
Copy `TubeFromCenterline` folder to your directory (More precisely, you only need the `TubeFromCenterline.cpp` and  `CMakeLists.txt` in your folder), and
```
cd TubeFromCenterline
```
then, build 
```
mkdir build
cd build
cmake ..
cmake --build .
```
and execute the program
```
cd Debug
./TubeFromCenterline.exe
```
