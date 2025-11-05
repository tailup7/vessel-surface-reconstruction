# vessel-surface-reconstruction

## environment
+ Windows11
+ Visual Studio 2022
+ CMake 3.30
+ VTK



## useful libraries; VTK

https://examples.vtk.org/site/Cxx/VisualizationAlgorithms/TubesWithVaryingRadiusAndColors/

https://examples.vtk.org/site/Cxx/VisualizationAlgorithms/TubesFromSplines/

https://examples.vtk.org/site/Cxx/PolyData/TubeFilter/

## usage
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


### some memo for me

<p align="left">
  <img src="pictures/stenosis.png" width="40%">
</p>
