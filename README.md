# AR.js - Augmented Reality for the Web

AR.js is a lightweight library for Augmented Reality on the Web, coming with features like Marker based and Location based AR.

Check the [official repository](https://github.com/jeromeetienne/AR.js) for more information about AR.js.

# Apriltag detection 

This fork is bringing apriltag detection to AR.js

## Example: Getting Apriltag detection events in a component

See [it live](https://spatial.andrew.cmu.edu/AR.js/three.js/src/threex/threex-apriltag/examples/detection-events.html).

```html
<!doctype HTML>
<!-- AR.js by @jerome_etienne - github: https://github.com/jeromeetienne/ar.js - info: https://medium.com/arjs/augmented-reality-in-10-lines-of-html-4e193ea9fdbf -->
<script src="https://aframe.io/releases/0.8.0/aframe.min.js"></script>

<!-- include ar.js scripts -->
<script src="https://spatial.andrew.cmu.edu/AR.js/aframe/build/aframe-ar.js"></script>

<script>
/* this is a simple example showing how a component can listen to 'apriltag-detection' events */
AFRAME.registerComponent('apriltag-event-example', {
  init: function () {
	// apriltag events are emitted to the scene element (this.el.sceneEl)
	this.el.sceneEl.addEventListener('apriltag-detection', function (event) {
		  console.log(event.detail); // detail is an array with the id, corners, center (and pose, if enabled) of each detection
	});
  }
});
</script>
		
<body style='margin : 0px; overflow: hidden;'>
	<a-scene apriltag-event-example embedded arjs='sourceType: webcam; trackingMethod: apriltag'>
		<a-box position='0 0.5 0' material='opacity: 0.5;'></a-box>
	</a-scene>
</body>
```

Example detection object:
```
center: {x: 415.33, y: 415.33}
corners: Array(4)
0: {x: 374, y: 365.29}
1: {x: 456.15, y: 364.56}
2: {x: 455.15, y: 279.6}
3: {x: 374.23, y: 278.45}
id: 586
```
