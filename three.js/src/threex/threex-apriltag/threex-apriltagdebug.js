var THREEx = THREEx || {}

THREEx.ApriltagDebug = function(apriltagContext){
	this.apriltagContext = apriltagContext

// TODO to rename canvasElement into canvas
	this.canvasElement = document.createElement('canvas');
	this.canvasElement.width = this.apriltagContext.canvas.width
	this.canvasElement.height = this.apriltagContext.canvas.height

	document.body.appendChild(this.canvasElement );
}

THREEx.ApriltagDebug.prototype.setSize = function (width, height) {
        if( this.canvasElement.width !== width )	this.canvasElement.width = width
        if( this.canvasElement.height !== height )	this.canvasElement.height = height
}

THREEx.ApriltagDebug.prototype.clear = function(){
	var canvas = this.canvasElement
	var context = canvas.getContext('2d');
	context.clearRect(0,0,canvas.width, canvas.height)
	
}

THREEx.ApriltagDebug.prototype.drawDetectedTags = function(detectedTags){

	if (detectedTags.length == 0) return;

	var canvas = this.canvasElement

	var context = canvas.getContext('2d');

	context.save();

	context.strokeStyle = "DODGERBLUE";
	context.lineWidth = 5;

		for (i=0; i< detectedTags.length; i++) {
			let det_corners = detectedTags[i].corners; 
			context.beginPath();
			context.moveTo(det_corners[0].x, det_corners[0].y);
			context.lineTo(det_corners[1].x, det_corners[1].y);
			context.lineTo(det_corners[2].x, det_corners[2].y);
			context.lineTo(det_corners[3].x, det_corners[3].y);
			context.lineTo(det_corners[0].x, det_corners[0].y);
			context.stroke();

			context.strokeText(detectedTags[i].id, detectedTags[i].center.x, detectedTags[i].center.y)
		}

	context.restore();

}
