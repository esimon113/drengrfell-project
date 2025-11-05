if [ -d build ]; then
    rm -rf build
    echo "Removed build directory."
else
    echo "Build directory does not exist."
fi


echo "Proceeding to create a new build directory."
mkdir build
cd build


cmake ..
make


echo "Running program..."
./vgp-template
