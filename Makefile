DEPS ?= gyp nodeuv-fs bale

all: build

.PHONY: deps
	deps: $(DEPS)

gyp: deps/gyp
deps/gyp:
	git clone --depth 1 https://chromium.googlesource.com/external/gyp.git ./deps/gyp

nodeuv-fs: deps/nodeuv-fs
./deps/nodeuv-fs:
	git clone --depth 1 http://github.com/hij1nx/nodeuv-fs ./deps/nodeuv-fs
	cd ./deps/nodeuv-fs && make

bale: bale.cc
	make -C ./build/ bale
	cp ./build/out/Release/bale ./bale

build: $(DEPS)
	deps/gyp/gyp --depth=. -Goutput_dir=./out -Icommon.gypi --generator-output=./build -Dlibrary=static_library -Duv_library=static_library -f make

distclean:
	make clean
	rm -rf ./build

clean:
	rm -rf ./build/out/Release/obj.target/server/
	rm -f ./build/out/Release/server
