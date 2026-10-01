/* Runs the downloadable ROM unchanged in binjgb. */
'use strict';
const canvas = document.querySelector('#screen');
const context = canvas.getContext('2d', { alpha: false });
const launch = document.querySelector('#launch');
const status = document.querySelector('#status');
const soundButton = document.querySelector('#sound');
const pauseButton = document.querySelector('#pause');
const menuButton = document.querySelector('#menu');
const gameSlug = document.body.dataset.game;
const gameTitle = document.body.dataset.title;
const CLOCK = 4194304;
const AUDIO_FRAMES = 2048;
let moduleGB, emulator, romPointer, joypadPointer, audio, audioTime = 0;
let sound = false, started = false, previous = 0, totalFrames = 0;
let ramFile = 0, ramSize = 0, ramDirty = false, lastSavedRam = '', saveWarning = false;
const saveKey = 'chromatic:ram:v1:' + gameSlug;
const held = new Map();
const sources = new Set();
const keys = { ArrowUp: 'up', KeyW: 'up', ArrowDown: 'down', KeyS: 'down', ArrowLeft: 'left', KeyA: 'left', ArrowRight: 'right', KeyD: 'right', KeyX: 'A', Space: 'A', KeyZ: 'B', ShiftLeft: 'B', ShiftRight: 'B', Enter: 'start', KeyP: 'start', KeyC: 'select' };
function key(name, pressed, owner) {
  if (!emulator) return;
  const owners = held.get(name) || new Set();
  if (pressed) owners.add(owner); else owners.delete(owner);
  held.set(name, owners);
  moduleGB['_set_joyp_' + name](emulator, owners.size > 0);
}
function releaseAll() {
  for (const name of held.keys()) moduleGB['_set_joyp_' + name](emulator, false);
  held.clear();
}
function stopAudio() {
  for (const source of sources) source.stop();
  sources.clear();
  audioTime = audio ? audio.currentTime : 0;
}
function pulse(name) {
  key(name, true, 'pulse');
  setTimeout(() => key(name, false, 'pulse'), 100);
}
async function enableAudio() {
  if (!audio) audio = new AudioContext({ sampleRate: 48000 });
  await audio.resume();
}
function setSound(value) {
  sound = value;
  if (!sound) stopAudio();
  soundButton.textContent = sound ? 'SOUND ON' : 'SOUND OFF';
  soundButton.setAttribute('aria-pressed', String(sound));
}
function queueAudio() {
  if (!sound || !audio || audio.state !== 'running') return;
  const bytes = new Uint8Array(moduleGB.HEAPU8.buffer, moduleGB._get_audio_buffer_ptr(emulator), moduleGB._get_audio_buffer_capacity(emulator));
  const buffer = audio.createBuffer(2, AUDIO_FRAMES, 48000);
  for (let channel = 0; channel < 2; channel++) {
    const samples = buffer.getChannelData(channel);
    for (let i = 0; i < AUDIO_FRAMES; i++) samples[i] = (bytes[i * 2 + channel] / 255) * 0.25;
  }
  audioTime = Math.max(audio.currentTime + 0.015, audioTime);
  if (audioTime > audio.currentTime + 0.25) { stopAudio(); audioTime = audio.currentTime + 0.015; }
  const source = audio.createBufferSource();
  const filter = audio.createBiquadFilter();
  filter.type = 'highpass'; filter.frequency.value = 20;
  source.buffer = buffer; source.connect(filter); filter.connect(audio.destination);
  sources.add(source);
  source.onended = () => { sources.delete(source); source.disconnect(); filter.disconnect(); };
  source.start(audioTime); audioTime += AUDIO_FRAMES / 48000;
}
function render() {
  const ptr = moduleGB._get_frame_buffer_ptr(emulator);
  const pixels = new Uint8ClampedArray(moduleGB.HEAPU8.buffer, ptr, 160 * 144 * 4);
  context.putImageData(new ImageData(pixels, 160, 144), 0, 0);
}
function loadCartridgeSave() {
  ramFile = moduleGB._ext_ram_file_data_new(emulator);
  ramSize = moduleGB._get_file_data_size(ramFile);
  if (!ramSize) return;
  try {
    const encoded = localStorage.getItem(saveKey);
    if (!encoded) return;
    const bytes = atob(encoded);
    if (bytes.length !== ramSize) return;
    const offset = moduleGB._get_file_data_ptr(ramFile);
    for (let i = 0; i < ramSize; i++) moduleGB.HEAPU8[offset + i] = bytes.charCodeAt(i);
    if (moduleGB._emulator_read_ext_ram(emulator, ramFile) === 0) lastSavedRam = encoded;
  } catch (error) { console.warn('Cartridge save could not be restored', error); }
}
function saveCartridge() {
  if (!ramFile || !ramSize || !ramDirty) return;
  try {
    if (moduleGB._emulator_write_ext_ram(emulator, ramFile) !== 0) return;
    const offset = moduleGB._get_file_data_ptr(ramFile);
    const bytes = moduleGB.HEAPU8.subarray(offset, offset + ramSize);
    const encoded = btoa(String.fromCharCode(...bytes));
    if (encoded !== lastSavedRam) localStorage.setItem(saveKey, encoded);
    lastSavedRam = encoded;
    ramDirty = false;
    saveWarning = false;
  } catch (error) {
    if (!saveWarning) console.warn('Cartridge save could not be stored', error);
    saveWarning = true;
  }
}
function advance(seconds) {
  const target = moduleGB._emulator_get_ticks_f64(emulator) + seconds * CLOCK;
  let event;
  do {
    event = moduleGB._emulator_run_until_f64(emulator, target);
    if (event & 1) totalFrames++;
    if (event & 2) queueAudio();
  } while (!(event & 4));
  if (ramFile && moduleGB._emulator_was_ext_ram_updated(emulator)) ramDirty = true;
  saveCartridge();
  render();
  canvas.dataset.frames = String(totalFrames);
}
function frame(now) {
  requestAnimationFrame(frame);
  if (!emulator || !started || document.hidden) { previous = 0; return; }
  pollGamepad();
  if (previous) advance(Math.min((now - previous) / 1000, 0.05));
  previous = now;
}
async function start() {
  if (!emulator || started) return;
  try { await enableAudio(); setSound(true); } catch { setSound(false); }
  started = true; launch.hidden = true; pauseButton.disabled = false;
  if (menuButton) menuButton.disabled = false;
  status.textContent = 'CARTRIDGE RUNNING'; canvas.focus(); pulse('start');
}
launch.addEventListener('click', start);
pauseButton.addEventListener('click', () => { pulse('start'); canvas.focus(); });
/* The anthology returns to its launcher on Start and Select together. */
if (menuButton) menuButton.addEventListener('click', () => { pulse('start'); pulse('select'); canvas.focus(); });
soundButton.addEventListener('click', async () => {
  try { await enableAudio(); setSound(!sound); } catch { status.textContent = 'AUDIO UNAVAILABLE'; }
});
window.addEventListener('keydown', event => {
  if (event.code === 'KeyM' && !event.repeat && started) { event.preventDefault(); soundButton.click(); return; }
  if (!keys[event.code]) return;
  if (!started) { if ((event.code === 'Enter' || event.code === 'Space') && !launch.disabled) { event.preventDefault(); start(); } return; }
  if (event.target instanceof HTMLButtonElement && (event.code === 'Space' || event.code === 'Enter')) return;
  event.preventDefault(); key(keys[event.code], true, event.code);
});
window.addEventListener('keyup', event => { if (keys[event.code]) key(keys[event.code], false, event.code); });
window.addEventListener('blur', () => { if (emulator) releaseAll(); });
document.addEventListener('visibilitychange', () => { previous = 0; if (emulator) releaseAll(); saveCartridge(); stopAudio(); });
for (const button of document.querySelectorAll('[data-key]')) {
  button.addEventListener('pointerdown', event => {
    event.preventDefault(); if (!started) return;
    button.setPointerCapture(event.pointerId); key(button.dataset.key, true, event.pointerId);
  });
  for (const type of ['pointerup', 'pointercancel', 'lostpointercapture']) button.addEventListener(type, event => key(button.dataset.key, false, event.pointerId));
}
(async () => {
  try {
    const response = await fetch('/roms/' + gameSlug + '.gbc');
    if (!response.ok) throw new Error('ROM download failed');
    const rom = new Uint8Array(await response.arrayBuffer());
    moduleGB = await Binjgb();
    romPointer = moduleGB._malloc(rom.length);
    moduleGB.HEAPU8.set(rom, romPointer);
    emulator = moduleGB._emulator_new_simple(romPointer, rom.length, 48000, AUDIO_FRAMES, 0);
    if (!emulator) throw new Error('Emulator could not start');
    if (rom[0x149]) loadCartridgeSave();
    joypadPointer = moduleGB._joypad_new();
    moduleGB._emulator_set_default_joypad_callback(emulator, joypadPointer);
    advance(2);
    launch.disabled = false;
    launch.querySelector('strong').textContent = 'Play ' + gameTitle;
    status.textContent = 'CARTRIDGE READY';
    requestAnimationFrame(frame);
  } catch (error) {
    console.error(error); status.textContent = 'COULD NOT LOAD GAME';
    launch.querySelector('strong').textContent = 'Please reload to try again.';
    launch.querySelector('small').textContent = 'You can also download the ROM above.';
  }
})();
window.addEventListener('pagehide', () => { if (emulator) releaseAll(); saveCartridge(); stopAudio(); });

document.querySelector('#fullscreen').addEventListener('click', async () => {
  try { if(document.fullscreenElement) await document.exitFullscreen(); else await document.querySelector('.screen-wrap').requestFullscreen(); }
  catch { status.textContent = 'FULLSCREEN UNAVAILABLE'; }
});
document.querySelector('#reset').addEventListener('click', () => { saveCartridge(); location.reload(); });
function pollGamepad() {
  const pad = navigator.getGamepads?.()[0];
  const buttons = {up:12,down:13,left:14,right:15,A:0,B:1,select:8,start:9};
  for(const [name,index] of Object.entries(buttons)) {
    let down=!!pad?.buttons[index]?.pressed;
    if(pad&&name==='left')down ||= pad.axes[0] < -0.4;
    if(pad&&name==='right')down ||= pad.axes[0] > 0.4;
    if(pad&&name==='up')down ||= pad.axes[1] < -0.4;
    if(pad&&name==='down')down ||= pad.axes[1] > 0.4;
    key(name,down,'gamepad');
  }
}
