all:
	rm -rf ./bin
	mkdir ./bin
	clang ./src/manifest.c -g -O0 -o ./bin/embellish -F/System/Library/PrivateFrameworks -framework SkyLight -framework Carbon -framework IOKit
