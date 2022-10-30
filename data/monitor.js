/* Script for remote browser oscilloscope / MOGLabs laser control

Ideas:
- could allow changing the horizontal position by dragging the screen.
*/

// DOM
const canvas = document.getElementById("display");
const ctx = canvas.getContext("2d");

const divSlider = document.getElementById("div-range");
const posSlider = document.getElementById("pos-range");
const persistSlider = document.getElementById("persist-range");
const thickSlider = document.getElementById("thick-range");
const useCookies = document.getElementById("use-cookies");

const resSlider = document.getElementById("sample-resolution");
const durationSlider = document.getElementById("sample-duration");
const resText = document.getElementById("resolution-text");
const durText = document.getElementById("duration-text");
const divText = document.getElementById("div-text");

// Could use wss:// (secure socket).
const gateway = `ws://${window.location.hostname}/ws`;
let websocket;

// Data storage
const maxTriggers = 20; /* Number of triggers' worth of data we remember.*/
let packets = []; // History of received herald+data packets.
let triggers = []; // Indices of trigger packets
let lastDrawnPacket = -1; // Index of most recently drawn packet (in packets)
let lastDrawnTrigger = -1; // " " (in triggers)
let herald = null; // Pending herald

// Drawing
const hiddenDataCanvas = document.createElement('canvas'); // For drawing data.
const dataCtx = hiddenDataCanvas.getContext('2d');
/* Rendering data first to a separate, hidden canvas allows us to fade it
without affecting the underlay, and to reposition it on the screen without
re-rendering it. Once the OffscreenCanvas API is no longer experimental, may use that.*/

let needsResize = true; // Full re-rendering of the underlay and data
let needsDataRender = true; // Re-render all data, instead of just new packets.
let needsReposition = true; // Redraw canvas with new position.
// New data is rendered by default on each update.

let pendingDisplayUpdate = 0; /* Nonzero long integer request id of a
requested AnimationFrame, or zero if no redraw pending.*/

function requestDisplayUpdate() {
  if (!pendingDisplayUpdate) {
    pendingDisplayUpdate = requestAnimationFrame(updateDisplay);
    // Manually reset pendingDisplay if the animation frame fails.
    setTimeout(() => { pendingDisplayUpdate = 0; }, 60);
  }
}

function requestResize() {
  needsResize = true;
  requestDisplayUpdate();
}

function updatePixelRatio() {
  /* Monitor change in pixel ratio, e.g. from zoom change or dragging
  to a screen with different pixel density*/
  requestResize();
  // Add new one-time listener for if the ratio changes from its current value.
  matchMedia(`(resolution: ${window.devicePixelRatio}dppx)`).addEventListener("change", updatePixelRatio, { once: true });
}

/* To avoid drawing more than necessary, we update the canvas 
at most once per screen refresh (via requestAnimationFrame). This
also ensures that the screen doesn't refresh midway through a redraw, and
avoids the need for debouncing high-frequency burst events (like canvas resize).

Each update combines all (and only) changes which need to be made, including
to the grid underlay, existing data, adding any new data packets and changing
display position. Events which trigger updates are:

- zoom/dpi change -> everything
- canvas resize -> everything.
- div, line-thickness or persistence change -> re-render data
- position change -> redraw canvas, copying data to new position
- new data packet -> fade old data and add new.

Each requestAnimationFrame call *adds* a callback to the next frame. We want
at most one display callback, so we check to see if one was already requested.

If the tab is not visible, most browsers will not run requestAnimationFrame callbacks. Our redraw flags will remain until the next successful
callback, and we request a full re-render when visibility is returned.
*/

/* INITIALISATION AND STATUS CHECKS */
window.addEventListener('load', async () => {
  updateStatus();
  setInterval(updateStatus, 10000);
  updatePixelRatio();
  initWebSocket();
  updateSampleSettings();
  (new ResizeObserver(requestResize)).observe(canvas, { box: "content-box" });
  document.addEventListener('visibilitychange', requestResize);

  // Load display-settings cookies
  const displaySettings = {};
  for (cookie of document.cookie.split(";")) {
    const [key, value] = cookie.trim().split("=");
    displaySettings[key] = value;
  }
  // Restore saved settings if enabled
  if (displaySettings["use-cookies"] == "true") {
    divSlider.value = displaySettings["div"] || divSlider.value;
    persistSlider.value = displaySettings["persist"] || persistSlider.value;
    posSlider.value = displaySettings["pos"] || posSlider.value;
    thickSlider.value = displaySettings["thick"] || thickSlider.value;
    useCookies.checked = true;
  } else {
    useCookies.checked = false;
  }
});

// Initialise WebSocket
function initWebSocket() {
  console.log('Trying to open a WebSocket connection...');
  websocket = new WebSocket(gateway);
  websocket.binaryType = "arraybuffer";
  websocket.onopen = () => {
    console.log("WebSocket connection opened.");
  }
  websocket.onclose = () => {
    console.log("WebSocket connection closed.");
    setTimeout(initWebSocket, 2000); //TODO: better scheme for this.
  }
  websocket.onmessage = onMessage;
}

// Check the laser's name and state.
async function updateStatus() {
  fetch("/status")
    .then((response) => {
      if (response.ok) {
        const status = response.json();
        // Add laser name to page title
        document.title = (status.name) ? "Laser: " + status.name : "Laser";
        document.getElementById("title").innerText = document.title;
        // Update switch states
        slowSwitch.setState(status.slow);
        fastSwitch.setState(status.fast);
        holdSwitch.setState(status.hold);
      } else {
        console.warn("Status check failed.");
      }
    })
    .catch((error) => {
      console.warn("A connection error occurred.");
    });
}

async function updateSampleSettings(write = false) {
  /* Retrieve the board's sampling settings and bounds, 
  after optionally attempting to set their values (if write=true)*/

  // Temporarily disable settings sliders
  resSlider.disabled = true;
  durationSlider.disabled = true;
  // Check if sending or merely reading settings values
  const url = write ? "/set_sample_settings" : "/get_sample_settings";
  const options = (!write) ? { method: "GET" } :
    {
      method: "POST",
      headers: {
        'Content-Type': 'application/json', /* Required for ESP's 
        AsyncCallbackJsonWebHandler to trigger*/
      },
      body: JSON.stringify({
        resolution: Number(resSlider.value),
        duration: Number(durationSlider.value)
      })
    }
  // Send request
  fetch(url, options)
    .then(async (response) => {
      if (response.ok) {
        /* Use the retrieved sampling settings, which may differ from the ones
        we sent (if we sent any)*/
        const settings = await response.json();
        resSlider.value = settings.resolution;
        resText.innerText = `${settings.resolution.toFixed(1)}ms`;
        durationSlider.value = settings.duration;
        durText.innerText = `${settings.duration.toFixed(1)}ms`;
      } else {
        console.warn("Settings update failed.");
      }
    })
    .catch((error) => {
      console.warn("A connection error occurred.");
    })
    .finally(() => { // Re-enable sliders after a delay.
      setTimeout(() => {
        resSlider.disabled = false;
        durationSlider.disabled = false;
      }, 500);
    });
}

/* SETTINGS */
function saveDisplaySettings() {
  if (useCookies.checked) {
    document.cookie = `max-age=${365 * 24 * 3600}`;
    document.cookie = `persist=${persistSlider.value}`;
    document.cookie = `pos=${posSlider.value}`;
    document.cookie = `div=${divSlider.value}`;
    document.cookie = `thick=${thickSlider.value}`;
    document.cookie = `use-cookies=true`;
  }
}

function checkCookies() {
  if (useCookies.checked) {
    saveDisplaySettings();
  } else {
    document.cookie = `max-age=${365 * 24 * 3600}`;
    document.cookie = `use-cookies=false`;
  }
}

function updateDisplaySettings(...settingNames) {
  if (settingNames.includes("div")) { needsResize = true; }
  if (settingNames.includes("pos")) { needsReposition = true; }
  if (settingNames.includes("persist")) { needsDataRender = true; }
  if (settingNames.includes("line")) { needsDataRender = true; }
  requestDisplayUpdate();
}

/* RECEIVING & DISPLAYING DATA */
function onMessage(event) { // Handle Websocket message
  if (event.data instanceof ArrayBuffer) { // Measurement packet (binary)
    if (herald) { // Make sure a herald came first. Otherwise ignore.
      herald.measurements = new Uint8Array(event.data);
      const i = packets.push(herald) - 1; //Record packet and get index
      if (herald.trigTime !== 0) { //TODO: use NaN or something instead of 0.
        triggers.push(i);
        cullData();
      }
      herald = null; // Ready for next herald
      requestDisplayUpdate();
    }
  } else { // Herald packet (text)
    herald = JSON.parse(event.data);
  }
}

function cullData() { // Remove excess data if required.
  if (triggers.length > maxTriggers + 10) { //TODO: add data-length condition too
    // Remove any triggers/packets older than maxTriggers
    const oldestTriggerToKeep = triggers.length - maxTriggers - 1; // Can assume >= 0
    const oldestPacketToKeep = triggers[oldestTriggerToKeep - 1] + 1; /* We 
    remove all packets up to the most recent removed trigger packet.
    */
    packets = packets.slice(oldestPacketToKeep);
    triggers = triggers.slice(oldestTriggerToKeep).map(i => i - oldestPacketToKeep);

    lastDrawnPacket -= oldestPacketToKeep;
    lastDrawnTrigger -= oldestTriggerToKeep;
  }
}

// Note that updateDisplay is only called when necessary.
const updateDisplay = (() => {
  /* Persistent drawing variables (NOTE 'width' EXCLUDES LEFT MARGIN) */
  let height, width, leftOffset, underlay, px_per_ms, vOffset;

  // Div timescale options
  const divScales = [1,5,10, 25, 50, 100, 250, 500]; //milliseconds
  divSlider.max = divScales.length - 1;
  const numDivs = 6; // Horizontal divs

  dataCtx.globalAlpha = 1;
  // For gridlines and trigger position line.
  ctx.globalAlpha = 1;
  ctx.lineWidth = "1px";

  // Private helper function for drawing a packet
  function renderPacket(packet, trigtime) {
    // TODO: highlight clipped signals in red.
    dataCtx.strokeStyle = "#ffff00"; // Colour for data
    const meas = packet.measurements;
    const px_per_datapoint = px_per_ms * packet.elapsed / meas.length;
    const offset = px_per_ms * (packet.start - trigtime);
    const px_per_voltbit = 0.75 * height / 255;
    dataCtx.beginPath();
    dataCtx.moveTo(offset, meas[0] * px_per_voltbit);
    for (let i = 1; i < meas.length; i++) {
      dataCtx.lineTo(offset + i * px_per_datapoint, meas[i] * px_per_voltbit);
    }
    dataCtx.stroke();
  }

  return async () => {
    if (needsResize) { // Triggered by canvas resize or zoom/dpi change
      // Resize requires full re-render/draw, so force the corresponding flags.
      needsDataRender = true;
      needsReposition = true;
      // Achieve correct resolution by setting 'drawing width' to pixel width.
      canvas.width = canvas.clientWidth;
      canvas.height = canvas.clientHeight;
      leftOffset = 0.05 * canvas.width; // Space for volt markings
      height = canvas.height;
      width = canvas.width - leftOffset;
      // Set +y to be upwards with origin at the bottom left.
      ctx.setTransform(1, 0, 0, -1, 0, height);
      const dpr = window.devicePixelRatio;
      ctx.scale(dpr, dpr); /* Handle devicePixelRatio behind-the-scenes, so we
      can work with CSS pixels instead of device pixels */

      /* RenderingContext2D considers a line to be *centred* at a coordinate, so
      offset by 0.5px to get clear lines when drawn at integer pixel coordinates.*/
      ctx.translate(0.5, 0.5);
      /* The data-rendeing canvas has twice the
      width, and the centre corresponds with the trigger point. Its origin is
      set at the middle, vOffset from the top. The vertical axis is *not* flipped, to make it easier to copy it right-way-up to the main canvas. */
      vOffset = height / 10; // Distance of 0V from the bottom
      hiddenDataCanvas.width = 2 * width;
      hiddenDataCanvas.height = 2 * height;
      dataCtx.setTransform(1, 0, 0, 1, width, vOffset);
      dataCtx.scale(dpr, dpr);
      dataCtx.translate(0.5, 0.5);

      /* Now redraw the underlay, and save it so we can use it to overwrite
      the screen when needed. */

      /* Gridline positions are rounded so they don't alias over multiple 
      pixels.*/
      ctx.strokeStyle = "cyan";
      ctx.beginPath();
      //  Horizontal
      const vSpacing = height / 4;
      for (let i = 0; i < 4; i++) {
        const y = Math.round(vOffset + i * vSpacing);
        ctx.moveTo(leftOffset, y);
        ctx.lineTo(canvas.width, y);
      }
      //  Vertical
      const hSpacing = (width * 0.9) / numDivs;
      const hOffset = leftOffset + width / 2;
      for (let i = -numDivs / 2; i <= numDivs / 2; i++) {
        const x = Math.round(hOffset + i * hSpacing);
        ctx.moveTo(x, 0);
        ctx.lineTo(x, height);
      }
      ctx.stroke();

      /* Voltage markings. Because I've inverted the y-axis, we also
      have to invert the text drawing. */
      ctx.fillStyle = "cyan";
      ctx.textAlign = "right";
      ctx.textBaseline = "middle";
      ctx.font = `${0.6 * leftOffset}px sans-serif`;
      ctx.save(); // Save un-rotated context
      ctx.translate(leftOffset, vOffset);
      ctx.scale(1, -1);
      for (let i = 0; i < 4; i++) {
        const y = Math.round(i * vSpacing);
        ctx.fillText(`${i}V `, 0, -y);
      }
      ctx.restore();

      // Save underlay for later
      //underlay = ctx.getImageData(0, 0, canvas.width, height);
      underlay = await createImageBitmap(canvas, { imageOrientation: "flipY" });
      needsResize = false;
    }

    if (needsDataRender) { // Redraw *all* data to hidden canvas
      // Re-compute data scale
      const divInterval = divScales[divSlider.value];
      divText.innerText = `${divInterval}ms`;
      px_per_ms = width / (divInterval * numDivs);

      // Say all packets/triggers need to be (re)drawn.
      lastDrawnPacket = -1;
      lastDrawnTrigger = -1;

      needsDataRender = false;
    }

    /* Draw all packets waiting to be drawn */
    dataCtx.lineWidth = thickSlider.value;
    if (lastDrawnTrigger >= 0) { // There's a previous trigger to draw from.
      let trigTime = packets[triggers[lastDrawnTrigger]].trigTime;
      // Packet to start from
      let packetIndex = (lastDrawnPacket < 0) ? 0 : lastDrawnPacket;
      // Packet to draw up to (exclusive) (stops at new trig or end of packets)
      const stopIndex = (lastDrawnTrigger < triggers.length - 1) ? triggers[lastDrawnTrigger + 1] : packets.length;
      // Rightmost time on screen
      const maxTime = trigTime + px_per_ms * width;
      while (packetIndex < stopIndex) {
        // Continue previous trigger (forwards only)
        const packet = packets[packetIndex++];
        if (packet.start > maxTime) { break; }
        renderPacket(packet, trigTime);
      }
      lastDrawnPacket = packetIndex;
    }
    //Any new trigger cycles
    let trigIndex = Math.max(lastDrawnTrigger + 1, 0);
    const trigCount = triggers.length;
    for (trigIndex; trigIndex < trigCount; trigIndex++) {
      // Fade previous data
      dataCtx.fillStyle = `rgba(255,255,255,${persistSlider.value})`;
      //dataCtx.fillStyle = window.getComputedStyle(canvas).backgroundColor;
      dataCtx.globalCompositeOperation = "destination-in";
      dataCtx.fillRect(-width, 0, hiddenDataCanvas.width, height);
      dataCtx.globalCompositeOperation = "source-over";

      const trigPacketIndex = triggers[trigIndex];
      const trigPacket = packets[trigPacketIndex];
      const trigTime = trigPacket.trigTime;
      const minTime = trigTime - px_per_ms * width; // Display bounds
      const maxTime = trigTime + px_per_ms * width;

      for (let i = trigPacketIndex-1; i >= 0; i--) { // Draw backwards from trigger point
        const packet = packets[i];
        if (packet.start + packet.elapsed < minTime) { break; }
        renderPacket(packet, trigTime);
      }
      for (let i = trigPacketIndex; i < packets.length; i++) { // Forwards from trigger point (including trigger packet itself)
        const packet = packets[i];
        if (packet.start > maxTime) { break; }
        renderPacket(packet, trigTime);
        lastDrawnPacket = i;
      }
      lastDrawnTrigger = trigIndex;
    }

    const pos = Math.round(leftOffset + Number(posSlider.value) * (width - 1));
    needsReposition = true;
    if (needsReposition) {
      // Clear canvas and redraw underlay
      ctx.clearRect(0, 0, canvas.width, height);
      ctx.drawImage(underlay, -0.5, -0.5);
      // Redraw position indicator
      const delta = 0.02 * height; // Size of indicator arrow
      ctx.strokeStyle = "#ffff00";
      ctx.fillStyle = "#ffff00";
      ctx.beginPath(); // Main line
      ctx.moveTo(pos, -1);
      ctx.lineTo(pos, height + 1);
      ctx.stroke();
      ctx.beginPath(); // Top triangle
      ctx.moveTo(pos - delta, height + 1);
      ctx.lineTo(pos, height - delta);
      ctx.lineTo(pos + delta, height + 1);
      ctx.fill();
      ctx.beginPath(); // Bottom triangle
      ctx.moveTo(pos - delta, -1);
      ctx.lineTo(pos, delta);
      ctx.lineTo(pos + delta, -1);
      ctx.fill();
      needsReposition = false;

      // Finally, in every display update we redraw the data onto the display
      ctx.drawImage(hiddenDataCanvas, leftOffset + width - pos, 0, width, height, leftOffset, 0, width, height)
      /* Necessary to respect transparency */
    }
    // Complete.
    pendingDisplayUpdate = 0;
  }
})();

/* SENDING INSTRUCTIONS */
class LiveSwitch {
  /* A switch which avoids being out of sync with the board. */
  constructor(button, url_on, url_off) {
    this.button = button;
    this.button.addEventListener('click', this.onclick.bind(this));
    this.url_on = url_on;
    this.url_off = url_off;
    this.active = true;
  }

  setState(active) {
    if (active) {
      this.active = active;
      this.button.classList.add("active");
    } else {
      this.button.classList.remove("active");
      this.active = false;
    }
  }

  onclick() {
    this.button.disabled = true; // Temporarily disable switch
    const url = (this.active) ? this.url_off : this.url_on;

    fetch(url, { method: "POST" })
      .then((response) => {
        if (response.ok) { // Toggle switch value
          this.setState(!this.active);
        } else {
          console.warn("The ESP failed to execute the command.");
        }
      })
      .catch((error) => {
        console.warn("A connection error occurred.");
      })
      .finally(() => {
        /* After all is said and done, re-enable the button
        (after a short delay to prevent spamming) */
        setTimeout(() => { this.button.disabled = false; }, 500);
      });
  }
}

const slowSwitch = new LiveSwitch(
  document.getElementById("slow-lock"), "/enable_slow", "/disable_slow"
);
const fastSwitch = new LiveSwitch(
  document.getElementById("fast-lock"), "/enable_fast", "/disable_fast"
);
const holdSwitch = new LiveSwitch(
  document.getElementById("hold"), "/enable_hold", "/disable_hold"
);