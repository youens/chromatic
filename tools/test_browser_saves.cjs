/* Verify the browser player uses the real binjgb battery RAM API. */
const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const root = require('node:path').resolve(__dirname, '..');
const Binjgb = require(root + '/games/hello-dot/web/vendor/binjgb.js');
const wasmBinary = fs.readFileSync(root + '/games/hello-dot/web/vendor/binjgb.wasm');
const rom = fs.readFileSync(root + '/games/arcade/dist/chromatic-arcade.gbc');
const player = fs.readFileSync(root + '/collection/web/player.js', 'utf8');
const stored = new Map();
const localStorage = {getItem:k=>stored.get(k)||null,setItem:(k,v)=>stored.set(k,v)};
async function boot(slug='chromatic-arcade') {
  const elements = new Map();
  const element = selector => {
    if (!elements.has(selector)) elements.set(selector, {disabled:true,dataset:{},addEventListener(){},querySelector(){return {textContent:''}},getContext(){return {putImageData(){}}},focus(){},setAttribute(){}});
    return elements.get(selector);
  };
  const ctx = vm.createContext({console:{...console,warn(){}},Uint8Array,Uint8ClampedArray,ImageData:class{},HTMLButtonElement:class{},Map,Set,String,Math,Number,Promise,document:{body:{dataset:{game:slug,title:'Test'}},querySelector:element,querySelectorAll:()=>[],addEventListener(){}},window:{addEventListener(){}},localStorage,atob,btoa,setTimeout,requestAnimationFrame(){},fetch:async()=>({ok:true,arrayBuffer:async()=>rom.buffer.slice(rom.byteOffset,rom.byteOffset+rom.byteLength)}),Binjgb:()=>Binjgb({wasmBinary,print(){},printErr(){}})});
  vm.runInContext(player,ctx);
  for(let i=0;i<10000 && vm.runInContext('!emulator',ctx);i++) await new Promise(setImmediate);
  for(let i=0;i<100 && element('#launch').disabled;i++) await new Promise(setImmediate);
  assert.equal(element('#launch').disabled,false,'real WASM boot completed');
  return ctx;
}
function mutate(c,address,value) { vm.runInContext(`moduleGB._emulator_write_mem(emulator,0,10);moduleGB._emulator_write_mem(emulator,0x4000,0);moduleGB._emulator_write_mem(emulator,${address},${value});moduleGB._emulator_write_mem(emulator,0,0);advance(0.02)`,c); }
function read(c,address) { return vm.runInContext(`moduleGB._emulator_write_mem(emulator,0,10);moduleGB._emulator_read_mem(emulator,${address})`,c); }
(async()=>{
 const first=await boot();
 mutate(first,0xA207,77);
 assert.ok(stored.has('chromatic:ram:v1:chromatic-arcade'));
 assert.equal(atob(stored.get('chromatic:ram:v1:chromatic-arcade')).charCodeAt(0x207),77);
 const second=await boot();
 assert.equal(read(second,0xA207),77,'battery SRAM persists through browser reload');
 const other=await boot('different-cartridge');
 assert.notEqual(read(other,0xA207),77,'cartridge slugs isolate save RAM');
 stored.set('chromatic:ram:v1:corrupt','invalid-base64!');
 const corrupt=await boot('corrupt');
 assert.ok(vm.runInContext('emulator > 0',corrupt),'invalid stored save does not stop play');
 console.log('PASS browser saves with real binjgb: SRAM writes stored, reload restored, per-cartridge isolation, malformed save recovery');
 for(const c of [first,second,other,corrupt]) vm.runInContext('moduleGB._file_data_delete(ramFile);moduleGB._emulator_delete(emulator);moduleGB._joypad_delete(joypadPointer);moduleGB._free(romPointer)',c);
})().catch(e=>{console.error(e);process.exitCode=1});
