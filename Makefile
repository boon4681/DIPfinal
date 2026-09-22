
main:
	@if not exist "bin" mkdir "bin"
	@if not exist "out" mkdir "out"
	g++ -std=c++17 gun.cpp -o bin/build.a
	@bin/build.a
	