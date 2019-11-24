watch: build
	fswatch -0 three.js/*.js | xargs -0 -n 1 -I {} make build
	
.PHONY: build
build:
	(cd  ../../../vendor/js-apriltag/emscripten && make)
	cat ../../../vendor/js-apriltag/emscripten/apriltag.js \
	 	threex-*.js > build/threex-apriltag.js

minify: build
	uglifyjs build/threex-apriltag.js > build/threex-apriltag-min.js

prepack: build
	prepack build/threex-apriltag.js --out build/threex-apriltag-prepacked.js

