var THREEx = THREEx || {}

//var ApriltagContext = function(parameters) {
THREEx.ApriltagContext = function(parameters){
	// handle default parameters
	parameters = parameters || {}
	this.parameters = {
		// TODO: debug - true if one should display artoolkit debug canvas, false otherwise
		//debug: parameters.debug !== undefined ? parameters.debug : false,
		// resolution of at which we detect pose in the source image
		canvasWidth: parameters.canvasWidth !== undefined ? parameters.canvasWidth : 640,
                canvasHeight: parameters.canvasHeight !== undefined ? parameters.canvasHeight : 480,
                maxDetectedTags: parameters.maxDetectedTags !== undefined ? parameters.maxDetectedTags : 10,
                decimate: parameters.decimate !== undefined ? parameters.decimate : 2.0,
                sigma: parameters.sigme !== undefined ? parameters.sigma : 0.0,
                nthreads: parameters.nthreads !== undefined ? parameters.nthreads : 1,
                refine_edges: parameters.refine_edges !== undefined ? parameters.refine_edges : 1,
                tag_size_meters: parameters.tag_size_meters !== undefined ? parameters.tag_size_meters : 0.1,
                camera_matrix: parameters.camera_matrix !== undefined ? parameters.camera_matrix : [[1466.857817,0.0,1984.688899],[0.0,1475.411916,1234.781818],[0.0,0.0,1.0]]
	}

        this.sceneEl = document.querySelector('a-scene');

        this.canvas = document.createElement('canvas');
        this.max_det_points_len = 150 * this.parameters.maxDetectedTags; 
        _this = this;
        AprilTag().then(function(Module) {
                // this is reached when everything is ready
                // init aprilTag object with calls to wasm module and some othe useful info
                _this.aprilTag = {
                        // save a reference to the module here
                        Module: Module,
                        
                        //int init(); Init the apriltag detector with given family and default options 
                        init: Module.cwrap('init', 'number', ['number', 'number', 'number', 'number', 'number']),
                        //int destroy(); Releases resources allocated by the wasm module
                        destroy: Module.cwrap('destroy', 'number', []),
                        //int set_detector_options(float decimate, float sigma, int nthreads, int refine_edges, int return_pose); Sets the given detector options
                        set_detector_options: Module.cwrap('set_detector_options', 'number', ['number', 'number', 'number', 'number', 'number']),
                        //int set_pose_info(double tagsize, double fx, double fy, double cx, double cy); Sets the tag size (meters) and camera intrinsics (in pixels) for tag pose estimation
                        set_pose_info: Module.cwrap('set_pose_info', 'number', ['number', 'number', 'number', 'number', 'number']),
                        //uint8_t* set_img_buffer(int width, int height, int stride); Creates/changes size of the image buffer where we receive the images to process
                        set_img_buffer: Module.cwrap('set_img_buffer', 'number', ['number', 'number', 'number']),
                        //uint8_t* detect(int bool_return_pose); Detect tags in image previously stored in the buffer. 
                        //returns pointer to buffer starting with an int32 indicating the size of the remaining buffer (a string of chars with the json describing the detections)
                        detect: Module.cwrap('detect', 'number', []),

                        // free convenience function
                        destroy_buffer: function(buf_ptr) {
                                this.Module._free(buf_ptr);
                        },

                        // supported tag families
                        family_id: {
                                tag36h11: 0,
                                tag25h9: 1,
                                tag16h5: 2,
                                tagCircle21h7: 3,
                                tagStandard41h12: 4
                        }
                      };                
                // inits detector with given family and default options: quad_decimate=2.0; quad_sigma=0.0; nthreads=1; refine_edges=1; return_pose=1
                _this.aprilTag.init(_this.aprilTag.family_id.tag36h11); // NOTE: no need to set_detector_options() if javasctipt options are different from default..
                // sets tag size and camera intrinsics for pose estimation
                //_this.aprilTag.set_pose_info(_this.parameters.tag_size_meters, _this.parameters.camera_matrix[0][0], _this.parameters.camera_matrix[1][1], _this.parameters.camera_matrix[0][2], _this.parameters.camera_matrix[1][2]);  
        });

        /*
        // setup THREEx.ApriltagDebug if needed
        this.debug = null;
        if( this.parameters.debug == true ){
                this.debug = new THREEx.ApriltagDebug(this);
        }
	*/
	// honor parameters.canvasWidth/.canvasHeight
	this.setSize(this.parameters.canvasWidth, this.parameters.canvasHeight);
}

THREEx.ApriltagContext.prototype.setSize = function (width, height) {
        if (this.canvas.width == width && this.canvas.height == height) return;

        this.canvas.width = width;
        this.canvas.height = height;

        /*
        if( this.debug !== null ){
                this.debug.setSize(width, height)
        }
        */
}

THREEx.ApriltagContext.prototype.detect = function (videoElement) {
        let canvas = this.canvas;

        // get imageData from videoElement
        let context = canvas.getContext('2d');
        context.drawImage(videoElement, 0, 0, canvas.width, canvas.height);
        var imageData = context.getImageData(0, 0, canvas.width, canvas.height);
        let grayscaleImg = new Uint8Array(canvas.width*canvas.height);

        // compute grayscale pixels
        for (j=0; j<imageData.height; j++)
        {
            for (i=0; i<imageData.width; i++)
            {
                let index=(i*4)*imageData.width+(j*4);
                let red=imageData.data[index];
                let green=imageData.data[index+1];
                let blue=imageData.data[index+2];
                let alpha=imageData.data[index+3];
                let average=(red+green+blue)/3;
                grayscaleImg[i*imageData.width+j] = average;                
            }
        }    
        imgBuffer = _this.aprilTag.set_img_buffer(canvas.width, canvas.height, canvas.width); // set_img_buffer allocates the buffer for image and returns it; just returns the previously allocated buffer if size has not changed
        this.aprilTag.Module.HEAPU8.set(grayscaleImg, imgBuffer); // copy grayscale image data
        let detectionsBuffer = this.aprilTag.detect();
        if (detectionsBuffer == 0) {
                this.aprilTag.destroy_buffer(detectionsBuffer);
                return [];                
        }
        let detectionsBufferSize = _this.aprilTag.Module.getValue(detectionsBuffer, "i32");
        if (detectionsBufferSize == 0) {
                this.aprilTag.destroy_buffer(detectionsBuffer);
                return [];
        }
        const resultView = new Uint8Array(this.aprilTag.Module.HEAP8.buffer, detectionsBuffer+4, detectionsBufferSize);
        let detectionsJson = '';
        for (let i = 0; i < detectionsBufferSize; i++) {
                detectionsJson += String.fromCharCode(resultView[i]);
        }
        this.aprilTag.destroy_buffer(detectionsBuffer);
        let detections = JSON.parse(detectionsJson);
        //console.log(detections);
        
        this.sceneEl.emit('apriltag-detection', detections, false);
        /*
        if( this.debug !== null ){
                this.debug.drawDetectedTags(detections);
        }
        */
	return detections;
};

/**
 * TODO: crappy function to update a object3d with a detectedMarker 
 */
/*
THREEx.ApriltagContext.prototype.updateObject3D = function(object3D, detectedMarker){

}
*/
