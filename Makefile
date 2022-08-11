debug:
	cmake --build build/debug-make
	./compile-shaders-macos.sh
	./build/debug-make/RachitText++

release:
	cmake --build build/release-make
	./compile-shaders-macos.sh
	./build/release-make/RachitText++
