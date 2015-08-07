DEPS ?= gyp nodeuv-fs nodeuv-http nodeuv-path debug json11

all: build

.PHONY: deps
	deps: $(DEPS)

gyp: deps/gyp
deps/gyp:
	git clone --depth 1 https://chromium.googlesource.com/external/gyp.git ./deps/gyp

docopt: deps/docopt
./deps/docopt:
	git clone --depth 1 https://github.com/docopt/docopt.cpp ./deps/docopt

nodeuv-path: deps/nodeuv-path
./deps/nodeuv-path:
	git clone --depth 1 http://github.com/hij1nx/nodeuv-path ./deps/nodeuv-path

nodeuv-http: deps/nodeuv-http
./deps/nodeuv-http:
	git clone --depth 1 http://github.com/hij1nx/nodeuv-http ./deps/nodeuv-http
	cd ./deps/nodeuv-http && make

nodeuv-fs: deps/nodeuv-fs
./deps/nodeuv-fs:
	git clone --depth 1 http://github.com/hij1nx/nodeuv-fs ./deps/nodeuv-fs
	cd ./deps/nodeuv-fs && make

json11: deps/json11
./deps/json11:
	git clone --depth 1 http://github.com/dropbox/json11 ./deps/json11

debug: deps/debug
./deps/debug:
	git clone --depth 1 http://github.com/hij1nx/debug ./deps/debug

build: $(DEPS)
	deps/gyp/gyp --depth=. -Goutput_dir=./out -Icommon.gypi --generator-output=./build -Dlibrary=static_library -Duv_library=static_library -f make

bale: bale.cc
	make -C ./build/ bale
	cp ./build/out/Release/bale ./bale

distclean:
	make clean
	rm -rf ./build

test:
	./bale ./test/input/main.cc ./test/output

clean:
	rm -rf ./build/out/Release/obj.target/server/
	rm -f ./build/out/Release/server
