#!/bin/sh

DEBUG=0

for arg in "$@"; do
    case "$arg" in
        --debug) # matches "--debug"
            DEBUG=1
            ;;
        *) # matches everything else
            echo "Unknown option: $arg"
            exit 1
            ;;
    esac
done

if [ -d build ]; then
    rm -rf build
    echo "Removed build directory."
else
    echo "Build directory does not exist."
fi

echo "Proceeding to create a new build directory."
mkdir build
cd build || exit 1

echo "Running cmake commands..."

if [ "$DEBUG" -eq 1 ]; then
    cmake -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_FLAGS="-fsanitize=address -g" \
        -DCMAKE_C_FLAGS="-fsanitize=address -g" ..
else
    echo "Debug mode disabled (no sanitizer checks)"
    cmake -DCMAKE_BUILD_TYPE=Release ..
fi

make
echo "Running cmake completed..."

if [ "$DEBUG" -eq 1 ]; then
    echo "Log sanitizer report"
    export ASAN_OPTIONS=log_path=asan_report:detect_leaks=1
    export ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer
fi

echo "Starting program..."
./drengrfell

if [ "$DEBUG" -eq 1 ]; then
    if compgen -G "asan_report*" >/dev/null; then
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
    else
        echo "No address sanitization report found."
    fi
fi
