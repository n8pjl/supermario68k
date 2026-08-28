const encodeInto = TextEncoder.prototype.encodeInto.bind(new TextEncoder);

let prevFrameTime = performance.now();
let truePrevFrameTime;