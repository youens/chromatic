/* Exercise streamed scenery in the same WASM emulator used by the website.
   Compare both VRAM map banks with the sector artwork after every frame,
   including horizontal movement, map wraparound and pause/resume. Check
   boss arena maps and centered movement through a full sway cycle. */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const game = path.resolve(__dirname, '..');
const root = path.resolve(game, '../..');
const Binjgb = require(root + '/games/hello-dot/web/vendor/binjgb.js');
const collection = process.argv.includes('--collection');
const build = collection ? root + '/games/arcade/build/chromatic-arcade' : game + '/build/dotwing';
const symbols = Object.fromEntries([...fs.readFileSync(build + '.noi', 'utf8')
  .matchAll(/DEF (_\w+) 0x([\da-fA-F]+)/g)].map(m => [m[1], parseInt(m[2], 16)]));
const headers = ['terraina_data.h', 'terrainb_data.h'].map(name => fs.readFileSync(game + '/src/' + name, 'utf8'));
function table(sector, name) {
  const data = headers[sector >> 1].match(new RegExp(`s${sector}_${name}\\[\\] = \\{([\\s\\S]*?)\\};`))[1];
  return data.match(/\d+/g).map(Number);
}
function scenery(sector) {
  const base = table(sector, 'base'), stamps = table(sector, 'stamps');
  const places = table(sector, 'place'), cells = table(sector, 'cells');
  return Array.from({length: 300}, (_, row) => {
    const result = Array.from({length: 32}, () => base.slice(0, 2));
    const mix = ((row * 7) & 255) ^ (row >> 3);
    for (let col = 0; col < 24; col++) {
      const offset = ((mix ^ (col * 13)) & 31) * 2;
      result[(col - 2) & 31] = base.slice(offset, offset + 2);
    }
    for (let i = 0; i < places.length; i += 4) {
      const first = places[i] + places[i + 1] * 256;
      if (first > row) break;
      const col = places[i + 2], id = places[i + 3];
      const [width, height, lo, hi] = stamps.slice((id & 127) * 4, (id & 127) * 4 + 4);
      if (row >= first + height) continue;
      const offset = lo + hi * 256 + (first + height - 1 - row) * width;
      for (let x = 0; x < width; x++) {
        const cell = (offset + (id & 128 ? width - 1 - x : x)) * 2;
        result[(col + x - 2) & 31] = [cells[cell], cells[cell + 1] ^ (id & 128 ? 32 : 0)];
      }
    }
    return result;
  });
}
(async () => {
  const rom = fs.readFileSync(process.env.DOTWING_VIDEO_ROM || build + '.gbc');
  let checked = 0;
  for (let sector = 0; sector < 4; sector++) {
    const m = await Binjgb({wasmBinary: fs.readFileSync(root + '/games/hello-dot/web/vendor/binjgb.wasm'), print() {}, printErr() {}});
    const rp = m._malloc(rom.length); m.HEAPU8.set(rom, rp);
    const expected = scenery(sector);
    const e = m._emulator_new_simple(rp, rom.length, 48000, 4096, 0);
    const joy = m._joypad_new(); m._emulator_set_default_joypad_callback(e, joy);
    const read = addr => m._emulator_read_mem(e, addr);
    const get = name => read(symbols['_' + name]);
    const put = (name, value) => m._emulator_write_mem(e, symbols['_' + name], value);
    const word = name => get(name) + 256 * read(symbols['_' + name] + 1);
    function run(frames) {
      const target = m._emulator_get_ticks_f64(e) + frames * 70224;
      let event; do { event = m._emulator_run_until_f64(e, target); } while (!(event & 4));
    }
    function launcherTap(key) {
      m['_set_joyp_' + key](e, 1); run(6);
      m['_set_joyp_' + key](e, 0); run(60);
    }
    function tap(key) {
      const bit = {A: 16, start: 128}[key];
      m['_set_joyp_' + key](e, 1);
      let limit = 1000;
      do { run(1); } while (!(get('dw_keys') & bit) && --limit);
      assert.ok(limit, 'button press sampled');
      run(2); m['_set_joyp_' + key](e, 0);
      limit = 1000;
      do { run(1); } while ((get('dw_keys') || get('dw_fade') !== 8) && --limit);
      assert.ok(limit, `button released and screen settled: sector ${sector + 1}, state ${get('dw_state')}, fade ${get('dw_fade')}, keys ${get('dw_keys')}, pc ${m._emulator_get_PC(e).toString(16)}`);
      run(2);
    }
    function verify() {
      if (get('dw_state') !== 3 || word('dw_cam') < 144) return;
      // Debug memory reads obey the LCD bus lock too. Sample during VBlank.
      while ((read(0xFF41) & 3) !== 1) {
        const target = m._emulator_get_ticks_f64(e) + 128;
        let event; do { event = m._emulator_run_until_f64(e, target); } while (!(event & 4));
      }
      const cam = word('dw_cam'), scy = read(0xFF42), scx = read(0xFF43);
      const bank = read(0xFF4F) & 1;
      m._emulator_write_mem(e, 0xFF4F, 0);
      const tiles = Array.from({length: 1024}, (_, i) => read(0x9800 + i));
      m._emulator_write_mem(e, 0xFF4F, 1);
      const attrs = Array.from({length: 1024}, (_, i) => read(0x9800 + i));
      m._emulator_write_mem(e, 0xFF4F, bank);
      // Locate this physical map row in the current turn of the circular map.
      for (let y = 0; y < 128; y += 4) {
        const my = ((scy + y) & 255) >> 3;
        const near = Math.floor((143 + cam - y) / 8);
        const row = 31 - my + Math.round((near - (31 - my)) / 32) * 32;
        for (let x = 0; x < 160; x += 4) {
          const mx = ((scx + x) & 255) >> 3, index = my * 32 + mx;
          assert.deepEqual([tiles[index], attrs[index]], expected[row][mx],
            `Sector ${sector + 1}, camera ${cam}, world row ${row}, column ${mx}: incomplete scenery transfer`);
        }
      }
      checked++;
    }
    try {
      run(120);
      if (collection) {
        launcherTap('start'); launcherTap('left');
        assert.equal(get('menu_index'), 11, 'Dotwing selected in collection');
        launcherTap('A');
        let limit = 1000;
        while ((get('dw_state') !== 0 || get('dw_fade') !== 8 || word('dw_clock') < 2) && --limit) run(1);
        assert.ok(limit, 'Dotwing title reached through launcher');
      }
      tap('A'); tap('A');
      if (sector) { put('dw_state', 5); put('dw_sector', sector); }
      tap('start');
      for (let frame = 0; frame < 1300; frame++) {
        put('dw_invincible', 120);
        const right = (frame >> 6) & 1;
        m._set_joyp_right(e, right); m._set_joyp_left(e, !right);
        run(1); verify();
        if (frame === 650) {
          m._set_joyp_right(e, 0); m._set_joyp_left(e, 0);
          tap('start'); assert.equal(get('dw_state'), 4);
          run(30); tap('start'); assert.equal(get('dw_state'), 3); verify();
        }
      }
      console.log(`PASS browser video: sector ${sector + 1}, full scenery rows through repeated map wraps and pause/resume`);
      // Arrive at the boss using the real warning and arena setup code.
      m._set_joyp_right(e, 0); m._set_joyp_left(e, 0);
      m._emulator_write_mem(e, symbols._dw_cam, 2400 & 255);
      m._emulator_write_mem(e, symbols._dw_cam + 1, 2400 >> 8);
      let limit = 700;
      while (get('dw_phase') !== 4 && --limit) {
        for (let foe = 0; foe < 8; foe++) m._emulator_write_mem(e, 0xD1A0 + foe * 16, 0);
        put('dw_invincible', 120); run(1);
      }
      assert.ok(limit, 'boss arena reached');
      assert.equal(word('dw_boss_x'), 32, 'boss enters centered on the 160-pixel screen');
      const bossHeader = fs.readFileSync(game + '/src/boss_data.h', 'utf8');
      const bossMap = bossHeader.match(new RegExp(`b${sector}_map\\[\\] = \\{([\\s\\S]*?)\\};`))[1].match(/\d+/g).map(Number);
      const base = table(sector, 'base');
      const bossCenters = [];
      for (let frame = 0; frame < 128; frame++) {
        put('dw_invincible', 120); run(1);
        while ((read(0xFF41) & 3) !== 1) {
          const target = m._emulator_get_ticks_f64(e) + 128;
          let event; do { event = m._emulator_run_until_f64(e, target); } while (!(event & 4));
        }
        const left = word('dw_boss_x');
        assert.ok(left >= 0 && left + 96 <= 160, 'whole boss stays inside the screen during sway');
        assert.equal(get('dw_scx'), (-left) & 255, 'boss scroll target follows its collision and attack origin');
        // SCX is applied after the VBlank interrupt, so this sample can
        // still show the preceding frame's scroll while its update waits.
        const visibleLeft = (-read(0xFF43)) & 255;
        assert.ok(visibleLeft + 96 <= 160, 'rendered boss stays inside the screen');
        bossCenters.push(visibleLeft + 48);
        const bank = read(0xFF4F) & 1;
        for (let vbank = 0; vbank < 2; vbank++) {
          m._emulator_write_mem(e, 0xFF4F, vbank);
          for (let row = 0; row < 32; row++) for (let col = 0; col < 32; col++) {
            const value = row < 7 && col < 12 ? bossMap[(row * 12 + col) * 2 + vbank] : base[vbank];
            assert.equal(read(0x9800 + row * 32 + col), value,
              `Sector ${sector + 1}, boss arena row ${row}, column ${col}, bank ${vbank}`);
          }
        }
        m._emulator_write_mem(e, 0xFF4F, bank);
      }
      const center = bossCenters.reduce((sum, x) => sum + x, 0) / bossCenters.length;
      assert.ok(Math.abs(center - 80) <= 1, 'boss sways around the middle of the screen');
      assert.ok(Math.min(...bossCenters) < 80 && Math.max(...bossCenters) > 80, 'boss moves to both sides of center');
      console.log(`PASS browser video: sector ${sector + 1}, complete boss arena, centered sway and aligned core`);
    } finally { m._emulator_delete(e); m._joypad_delete(joy); m._free(rp); }
  }
  console.log(`PASS ${collection ? 'collection' : 'standalone'}: ${checked} rendered frames checked against source scenery in both VRAM banks`);
})().catch(error => { console.error(error); process.exitCode = 1; });
