New-Item -ItemType Directory -Force -Path "build"
cd build

cmake -G"Visual Studio 17 2022" ${COMMON_CMAKE_CONFIG_PARAMS} ../
cmake --build . --config Debug

cd ..
