if [ -d build ]; then
    rm -rf build
    echo "Removed build directory."
else
    echo "Build directory does not exist."
fi

echo "Proceeding to create a new build directory."
mkdir build
cd build

echo "Running cmake commands..."
cmake -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address -g" \
    -DCMAKE_C_FLAGS="-fsanitize=address -g" ..
make
echo "Running cmake completed..."

echo "Log sanitizer report"
export ASAN_OPTIONS=log_path=asan_report:detect_leaks=1
export ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer

echo "Starting program..."
./drengrfell

echo "Leak Report:"

awk '
/Direct leak/ {
    leak_line = $0
    getline

    # Only count lines for files from src/
    if ($0 ~ /drengrfell-project\/src\//) {
        direct_count++

        # Count leaked bytes
        match(leak_line, /Direct leak of ([0-9]+)/, a)
        direct_bytes += a[1]
        print leak_line
        print $0
    }
}
/Indirect leak/ { next } # skip indirect leaks
END {
    print "Direct leaks in your code: " direct_count " | bytes: " direct_bytes
}' asan_report*
