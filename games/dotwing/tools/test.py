#!/usr/bin/env python3
"""Exercise the actual cartridge in PyBoy; all saves live in a temp directory.

Ordinary play uses buttons alone. Controlled scenarios arrange WRAM actors or
SRAM records, then let the unmodified ROM process collisions, input, saving
and loading. The evidence manifest keeps those two kinds of validation apart.
"""
from pathlib import Path
import hashlib
import importlib.metadata
import json
import re
import shutil
import tempfile
import time

from pyboy import PyBoy

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
ROM = BUILD / "dotwing.gbc"
SYM = {name: int(value, 16) for name, value in re.findall(
    r"DEF (_\w+) 0x([\da-fA-F]+)", (BUILD / "dotwing.noi").read_text())}
IDS = {name: int(value) for name, value in re.findall(
    r"#define (DW_\w+) (\d+)", (ROOT / "src/art_ids.h").read_text())}
TITLE, BUILDER, HANGAR, FLIGHT, PAUSE, SHOP, RESULTS, HELP, TAKEOFF = range(9)
P_INTRO, P_WAVES, P_WARN, P_BOSS_IN, P_BOSS, P_BOSS_DIE, P_CLEAR, P_DEAD = range(8)
D_COIN, D_POWER, D_REPAIR, D_BOLT, D_ALLY = range(5)
K_DRONE, K_GUNNER, K_HEAVY = range(3)
M_DIVE = 0
BIAS = 32
FOE, SHOT, DROP = 16, 10, 8            # record strides
FOES, SHOTS, BULLETS, DROPS = 8, 12, 24, 6
BOSS_HP = (320, 400, 480, 560)
PRICES = (40, 80, 140)
checks, ordinary, controlled = [], [], []
evidence = {"emulator": "PyBoy " + importlib.metadata.version("pyboy"),
            "ordinary_input_checks": ordinary, "controlled_scenarios": controlled,
            "screenshots": [], "passed": checks, "status": "running"}
p = None
held = set()
video_frames = 0
lcd_off_frames = 0
display_off_calls = 0
phase = "ordinary input"


def check(condition, label, group=ordinary):
    assert condition, label
    checks.append(label)
    group.append(label)
    print("PASS:", label, flush=True)


def count_display_off(_context):
    global display_off_calls
    display_off_calls += 1


def boot(path, raw=None):
    global p, held, display_off_calls
    p = PyBoy(str(path), window="null", sound_emulated=False)
    p.set_emulation_speed(0)
    p.hook_register(0, SYM["_display_off"], count_display_off, None)
    held = set()
    display_off_calls = 0
    if raw is not None:
        p.memory[0x0000] = 0x0A
        p.memory[0x4000] = 0
        for i, byte in enumerate(raw):
            p.memory[0xA200 + i] = byte
        p.memory[0x0000] = 0
    p.tick(60)
    until(lambda: get("dw_state") == TITLE and get("dw_fade") == 8
          and word("dw_clock") > 2, watch=False)
    # The C runtime and the game's own setup each switch the LCD off once
    # while the screen is still blank; nothing may do so afterwards.
    assert display_off_calls == 2, ("boot display_off calls", display_off_calls)
    display_off_calls = 0


def run(frames=1, keys=(), watch=True):
    """Advance whole video frames; every frame must keep the LCD on."""
    global held, video_frames, lcd_off_frames
    keys = set(keys)
    for key in held - keys:
        p.button_release(key)
    for key in keys - held:
        p.button_press(key)
    held = keys
    for _ in range(frames):
        p.tick()
        if watch and not p.memory[0xFF40] & 0x80:
            lcd_off_frames += 1
    video_frames += frames


def until(predicate, limit=900, keys=(), watch=True):
    for _ in range(limit):
        if predicate():
            return
        run(1, keys, watch)
    raise AssertionError("condition timed out: state=%d phase=%d" % (get("dw_state"), get("dw_phase")))


BITS = {"right": 1, "left": 2, "up": 4, "down": 8,
        "a": 16, "b": 32, "select": 64, "start": 128}


def tap(key):
    # Screen changes fade over several frames. Hold each press until the ROM
    # has sampled it, then release and wait for any fade to settle.
    run(1)
    until(lambda: get("dw_keys") == 0)
    run(1, [key])
    until(lambda: bool(get("dw_keys") & BITS[key]), keys=[key])
    run(2, [key])
    run(1)
    until(lambda: get("dw_keys") == 0)
    settle()


def settle():
    until(lambda: get("dw_fade") in (0, 8))
    run(2)


def get(name, offset=0):
    return p.memory[SYM["_" + name] + offset]


def put(name, value, offset=0):
    p.memory[SYM["_" + name] + offset] = value & 255


def word(name, offset=0):
    return get(name, offset) | (get(name, offset + 1) << 8)


def sword(name, offset=0):
    value = word(name, offset)
    return value - 65536 if value > 32767 else value


def putword(name, value, offset=0):
    put(name, value, offset)
    put(name, value >> 8, offset + 1)


def pos(pixel):
    return ((pixel + BIAS) << 8) | 0x80


def profile():
    """shape, color, face, gear, configured, weapon, shield, reactor,
    cleared, tokens, ally"""
    return ([get("dw_profile", i) for i in range(9)]
            + [word("dw_profile", 9), get("dw_profile", 11)])


def screenshot(name):
    until(lambda: bool(p.memory[0xFF40] & 0x80) and get("dw_fade") == 8)
    run(2, held)
    target = BUILD / (name + ".png")
    assert p.screen.image.size == (160, 144)
    p.screen.image.save(target)
    evidence["screenshots"].append({"file": target.name, "size": [160, 144],
        "state": get("dw_state"), "sector": get("dw_sector"),
        "logic_frame": word("dw_frame"), "provenance": phase})


def slots():
    p.memory[0x0000] = 0x0A
    p.memory[0x4000] = 0
    data = bytes(p.memory[0xA200:0xA240])
    p.memory[0x0000] = 0
    return data


def checksum(record):
    value = 0xD071
    for byte in record[:28]:
        value = (((value << 1) | (value >> 15)) & 65535) ^ byte
    return value


def valid(record):
    return (record[:5] == b"DOTW\x01" and record[31] == 0xA5
            and record[28] | record[29] << 8 == checksum(record))


def record(generation, shape, tokens, ally=0, fields=None):
    data = bytearray(32)
    data[:5] = b"DOTW\x01"
    data[5] = generation
    data[6:15] = bytes(fields or [shape, 1, 1, 1, 1, 1, 1, 1, 4])
    data[15:20] = bytes([tokens & 255, tokens >> 8, 123, 0, ally])
    total = checksum(data)
    data[28:30] = bytes([total & 255, total >> 8])
    data[31] = 0xA5
    return data


def live(name, count, stride):
    return [i for i in range(count) if get(name, i * stride)]


def clear_actors():
    for name, count, stride in (("dw_foes", FOES, FOE), ("dw_shots", SHOTS, SHOT),
                                ("dw_bullets", BULLETS, SHOT), ("dw_drops", DROPS, DROP)):
        for i in range(count):
            put(name, 0, i * stride)


def prepare(hp=3):
    """Hold the sortie in its wave phase, clear the sky and park the plane."""
    assert get("dw_state") == FLIGHT
    putword("dw_cam", 200)
    clear_actors()
    putword("dw_px", 72)
    putword("dw_py", 94)
    for name, value in (("dw_hp", hp), ("dw_invincible", 120),
                        ("dw_burst", 0), ("dw_energy", 100)):
        put(name, value)
    run(2)
    clear_actors()


def projectile(name, index, x, y, vx=0, vy=0, tile=None):
    """x, y are the projectile's centre in screen pixels."""
    o = index * SHOT
    putword(name, pos(x), o + 1)
    putword(name, vx, o + 3)
    putword(name, pos(y), o + 5)
    putword(name, vy, o + 7)
    put(name, IDS["DW_S_SHOT"] if tile is None else tile, o + 9)
    put(name, 1, o)


def foe(index, x=64, y=30, hp=1, kind=K_GUNNER):
    """A stationary foe whose top-left corner is (x, y) in screen pixels."""
    o = index * FOE
    putword("dw_foes", pos(x), o + 1)
    putword("dw_foes", 0, o + 3)
    putword("dw_foes", pos(y), o + 5)
    putword("dw_foes", 0, o + 7)
    for field, value in ((9, kind), (10, 0), (11, M_DIVE), (12, 0), (13, 0), (14, 255), (15, 0)):
        put("dw_foes", value, o + field)
    put("dw_foes", hp, o)


def pickup(kind):
    """Drop an arranged pickup on the plane; the ROM collects it."""
    putword("dw_drops", pos(sword("dw_px") + 8), 1)
    putword("dw_drops", pos(sword("dw_py") + 8), 3)
    put("dw_drops", kind, 5)
    put("dw_drops", 0, 6)
    put("dw_drops", 1, 0)
    until(lambda: not get("dw_drops"), limit=20)
    run(2)


def launch():
    tap("start")
    assert get("dw_state") == TAKEOFF, get("dw_state")
    until(lambda: get("dw_state") == FLIGHT, limit=400)
    run(2)


def end_sortie():
    tap("start")
    assert get("dw_state") == PAUSE
    tap("b")
    assert get("dw_state") == RESULTS
    tap("a")
    assert get("dw_state") == HANGAR


def bot_keys():
    """Pick buttons from what is on screen: chase the nearest foe or the boss
    core, sidestep incoming bullets and burst when one is about to hit."""
    px, py = sword("dw_px"), sword("dw_py")
    target, best = 72, 999
    for i in live("dw_foes", FOES, FOE):
        fx = get("dw_foes", i * FOE + 2) - BIAS
        if abs(fx - px) < best:
            best, target = abs(fx - px), fx
    if get("dw_phase") in (P_BOSS_IN, P_BOSS):
        target = sword("dw_boss_x") + 40
    keys, danger = [], None
    for i in live("dw_bullets", BULLETS, SHOT):
        bx = get("dw_bullets", i * SHOT + 2) - BIAS
        by = get("dw_bullets", i * SHOT + 6) - BIAS
        if 0 < by - py + 24 < 44 and abs(bx - (px + 8)) < 14:
            danger = bx
    if danger is not None:
        keys.append("left" if danger > px + 8 else "right")
        if get("dw_energy") >= 50 and abs(danger - (px + 8)) < 5:
            keys.append("b")
    elif abs(target - px) > 3:
        keys.append("right" if target > px else "left")
    if py < 96:
        keys.append("down")
    return keys


def main():
    global phase
    data = ROM.read_bytes()
    header = {"bytes": len(data), "cgb_flag": data[0x143],
              "mapper": data[0x147], "rom_size_code": data[0x148],
              "ram_size_code": data[0x149], "header_checksum": data[0x14D],
              "global_checksum": int.from_bytes(data[0x14E:0x150], "big")}
    assert len(data) == 128 * 1024
    assert data[0x143] == 0xC0 and data[0x147] == 0x1B
    assert data[0x148] == 2 and data[0x149] == 2
    assert data[0x14D] == (-sum(data[0x134:0x14D]) - 25) & 255
    assert header["global_checksum"] == (sum(data) - sum(data[0x14E:0x150])) & 65535
    evidence.update(sha256=hashlib.sha256(data).hexdigest(), header=header,
                    save_isolation="Temporary copied ROMs; no repository SRAM touched.")
    check(True, "CGB-only 128 KiB MBC5 battery RAM header and both checksums", [])
    with tempfile.TemporaryDirectory(prefix="dotwing-pyboy-") as directory:
        temp = Path(directory)
        game = temp / "dotwing.gbc"
        shutil.copyfile(ROM, game)
        boot(game)
        check(p.memory[0xFF4D] & 0x80, "The cartridge runs the CGB CPU in double-speed mode")
        screenshot("title")
        check(profile() == [0] * 11, "Fresh battery RAM initializes the profile")
        tap("up")
        check(get("dw_state") == HELP, "Flight guide opens with ordinary input")
        screenshot("guide")
        tap("b")
        check(get("dw_state") == TITLE, "B returns from the guide to the title")
        tap("a")
        check(get("dw_state") == BUILDER, "First play opens the dot builder")
        for row in range(4):
            tap("right")
            if row != 3:
                tap("down")
        screenshot("builder")
        check(profile()[:5] == [0] * 5, "Builder preview does not commit before Save")
        tap("a")
        check(profile()[:5] == [1, 1, 1, 1, 1] and get("dw_state") == HANGAR,
              "All four dot choices commit with ordinary input")
        raw = slots()
        check(valid(raw[:32]) and raw[5] == 0 and raw[31] == 0xA5,
              "Initial Save commits checksummed generation zero to A200")
        tap("a")
        check(get("dw_profile", 5) == 0 and word("dw_profile", 9) == 0,
              "An unaffordable upgrade leaves the profile unchanged")
        screenshot("hangar")
        tap("start")
        check(get("dw_state") == TAKEOFF, "Launch enters the animated takeoff")
        run(30)
        screenshot("takeoff")
        until(lambda: get("dw_state") == FLIGHT, limit=400)
        check(get("dw_phase") == P_WAVES and get("dw_hp") == 3,
              "Takeoff hands over to the wave phase with a full hull")
        # Measure while the buttons are already held, past input latency.
        run(3, ["right"])
        x, f = sword("dw_px"), word("dw_frame")
        run(10, ["right"])
        fast = (sword("dw_px") - x, (word("dw_frame") - f) & 65535)
        run(3, ["left", "a"])
        x, f = sword("dw_px"), word("dw_frame")
        run(10, ["left", "a"])
        slow = (x - sword("dw_px"), (word("dw_frame") - f) & 65535)
        run(2)
        check(fast[0] == fast[1] * 2 and slow[0] == slow[1] and slow[1] > 0,
              "Steering moves two pixels and A focus moves one per logic update")
        until(lambda: live("dw_shots", SHOTS, SHOT), limit=60)
        check(True, "The plane auto-fires without holding a fire button")
        tap("start")
        f, cam = word("dw_frame"), word("dw_cam")
        run(120)
        check(get("dw_state") == PAUSE and word("dw_frame") == f and word("dw_cam") == cam
              and get("dw_dim") == 1,
              "Pause dims the sky and freezes gameplay for 120 emulator frames")
        screenshot("pause")
        tap("start")
        check(get("dw_state") == FLIGHT and get("dw_dim") == 0,
              "Start resumes the paused sortie at full brightness")
        f, missed = word("dw_frame"), word("dw_missed")
        started = time.monotonic()
        run(240)
        elapsed = time.monotonic() - started
        delta = (word("dw_frame") - f) & 65535
        evidence["performance"] = {"emulator_frames": 240, "logic_updates": delta,
             "missed_frames": (word("dw_missed") - missed) & 65535,
             "logic_hz_at_59_7275_video_hz": round(delta / 240 * 59.7275, 2),
             "unthrottled_emulation_seconds": round(elapsed, 4)}
        check(delta >= 232, "Normal gameplay sustains approximately 60 logic Hz")

        # A complete ordinary campaign attempt: observation chooses buttons,
        # never changes RAM. Upgrades are bought with tokens earned in play.
        session_start = video_frames
        missed_start = word("dw_missed")
        flight_frames = 0
        captured = set()
        bosses, clears, purchases = set(), [], []
        while get("dw_state") in (FLIGHT, SHOP, TAKEOFF) and video_frames - session_start < 60 * 60 * 12:
            state = get("dw_state")
            if state == SHOP:
                sector = get("dw_sector")
                clears.append(sector)
                if "shop" not in captured:
                    screenshot("shop")
                    captured.add("shop")
                for row in range(4):
                    while get("dw_menu_row") != row:
                        tap("down")
                    level = (get("dw_profile", 5 + row) if row < 3 else get("dw_profile", 11))
                    if level < 3 and word("dw_profile", 9) >= PRICES[level]:
                        tap("a")
                        purchases.append({"after_sector": sector, "upgrade_row": row,
                                          "level": level + 1, "price": PRICES[level]})
                launch()
                continue
            if state == TAKEOFF:
                run(1)
                continue
            ph = get("dw_phase")
            if ph == P_BOSS and get("dw_sector") not in bosses and word("dw_phase_t") > 90:
                bosses.add(get("dw_sector"))
                screenshot("ordinary-boss-%d" % get("dw_sector"))
            if "gameplay" not in captured and len(live("dw_foes", FOES, FOE)) >= 3 \
                    and len(live("dw_bullets", BULLETS, SHOT)) >= 2:
                screenshot("gameplay")
                captured.add("gameplay")
            run(2, bot_keys())
            flight_frames += 2
        missed = (word("dw_missed") - missed_start) & 65535
        if get("dw_victory"):
            clears.append(4)
        evidence["ordinary_campaign"] = {
             "emulator_frames": video_frames - session_start,
             "flight_bot_frames": flight_frames, "missed_frames": missed,
             "missed_ratio": round(missed / max(1, flight_frames), 4),
             "score": word("dw_score"), "kills": word("dw_kills"),
             "run_tokens": word("dw_run_tokens"), "sector": get("dw_sector"),
             "ending_state": get("dw_state"), "victory": bool(get("dw_victory")),
             "cleared_sectors": clears, "bosses_seen": sorted(bosses),
             "upgrade_purchases": purchases,
             "method": "Button-only foe/boss tracking, bullet dodging, defensive "
                       "bursts and shop purchases; no WRAM writes."}
        print("ordinary campaign:", json.dumps(evidence["ordinary_campaign"]), flush=True)
        check(word("dw_kills") >= 20, "Ordinary button-only flight defeats rival squadrons")
        check(missed <= flight_frames * 0.04,
              "Busy flight keeps at least 96 percent of display frames on time")
        if clears:
            check(True, "Ordinary button-only play defeats a sector boss and reaches the shop")
        if purchases:
            check(True, "Ordinary campaign buys upgrades with earned tokens")
        if get("dw_victory"):
            check(True, "Ordinary button-only campaign defeats all four rival labs")
        state = get("dw_state")
        if state in (FLIGHT, TAKEOFF):
            until(lambda: get("dw_state") == FLIGHT)
            end_sortie()
        else:
            assert state == RESULTS, state
            screenshot("ordinary-results")
            tap("a")
        assert get("dw_state") == HANGAR
        # Shop currency has already been credited. A new, empty sortie must
        # not repeat that credit when ended through the actual pause menu.
        bank = word("dw_profile", 9)
        launch()
        end_sortie()
        check(word("dw_profile", 9) == bank,
              "Ending an empty sortie after the shop cannot duplicate prior credits")
        launch()
        check(p.memory[0xFF4A] == 128 and p.memory[0xFF40] & 0x20,
              "A sortie after one ended from the pause menu keeps the HUD at the bottom")
        end_sortie()

        # Controlled scenarios: return upgrades to baseline so the collision
        # and purchase checks do not depend on the bot's shop choices.
        phase = "controlled WRAM/SRAM scenario"
        for offset in (5, 6, 7, 11):
            put("dw_profile", 0, offset)
        launch()
        prepare()
        tokens, kills = word("dw_run_tokens"), word("dw_kills")
        foe(0, 64, 30)
        projectile("dw_shots", 0, 72, 46, vy=-1600)
        until(lambda: word("dw_kills") > kills, limit=30)
        check(word("dw_kills") == kills + 1 and not get("dw_foes"),
              "A real projectile collision defeats an arranged rival", controlled)
        prepare()
        foe(0, 64, 30, hp=5)
        projectile("dw_shots", 0, 72, 46, vy=-1600)
        until(lambda: get("dw_foes") != 5, limit=30)
        check(get("dw_foes") == 3 and get("dw_foes", 13) > 0,
              "A twin shot deals two damage and flashes a sturdier rival", controlled)
        prepare()
        tokens, score = word("dw_run_tokens"), word("dw_score")
        pickup(D_COIN)
        check(word("dw_run_tokens") == tokens + 1 and word("dw_score") == score + 25,
              "A coin pickup awards one token and 25 points", controlled)
        put("dw_power", 0)
        for _ in range(3):
            pickup(D_POWER)
        check(get("dw_power") == 2, "Power pickups strengthen the guns and clamp at level two", controlled)
        put("dw_hp", 2)
        pickup(D_REPAIR)
        hp = get("dw_hp")
        pickup(D_REPAIR)
        check(hp == 3 and get("dw_hp") == 3, "Repair restores one heart without exceeding the hull", controlled)
        put("dw_energy", 0)
        pickup(D_BOLT)
        check(get("dw_energy") == 100, "A bolt pickup fills burst energy", controlled)
        allies = [get("dw_allies")]
        pickup(D_ALLY)
        allies.append(get("dw_allies"))
        pickup(D_ALLY)
        pickup(D_ALLY)
        allies.append(get("dw_allies"))
        check(allies == [0, 1, 2], "Ally pickups add wing dots up to two", controlled)
        run(30)
        check(any(get("dw_shots", i * SHOT + 9) == IDS["DW_S_SHOT_SOLO"]
                  for i in live("dw_shots", SHOTS, SHOT)),
              "Wing dots fire their own shots", controlled)
        prepare()
        put("dw_invincible", 0)
        put("dw_power", 2)
        projectile("dw_bullets", 0, sword("dw_px") + 8, sword("dw_py") + 7, tile=IDS["DW_S_BULLET"])
        until(lambda: get("dw_hp") == 2, limit=20)
        check(get("dw_power") == 1 and get("dw_invincible") > 90,
              "A hostile bullet costs a heart and a power level and grants invincibility", controlled)
        projectile("dw_bullets", 0, sword("dw_px") + 8, sword("dw_py") + 7, tile=IDS["DW_S_BULLET"])
        run(5)
        check(get("dw_hp") == 2, "Invincibility prevents an immediate second hit", controlled)
        prepare()
        put("dw_invincible", 0)
        kills = word("dw_kills")
        foe(0, sword("dw_px"), sword("dw_py"))
        until(lambda: get("dw_hp") == 2, limit=10)
        check(word("dw_kills") == kills + 1, "Ramming a rival craft destroys it and damages the plane", controlled)
        prepare()
        for i in range(3):
            foe(i, 20 + i * 40, 20, hp=6)
            projectile("dw_bullets", i, 10 + i * 20, 60, tile=IDS["DW_S_BULLET"])
        tokens, kills = word("dw_run_tokens"), word("dw_kills")
        tap("b")
        check(48 <= get("dw_energy") <= 53 and get("dw_burst") > 0
              and not live("dw_bullets", BULLETS, SHOT)
              and word("dw_kills") == kills + 3 and word("dw_run_tokens") >= tokens + 6,
              "B burst spends fifty energy, clears bullets and destroys nearby rivals", controlled)
        prepare()
        put("dw_energy", 20)
        tap("b")
        check(get("dw_burst") == 0 and get("dw_energy") >= 20,
              "Insufficient energy prevents a burst", controlled)
        prepare()
        put("dw_hp", 1)
        put("dw_invincible", 0)
        projectile("dw_bullets", 0, sword("dw_px") + 8, sword("dw_py") + 7, tile=IDS["DW_S_BULLET"])
        until(lambda: get("dw_state") == RESULTS, limit=300)
        check(get("dw_victory") == 0, "Losing the last heart ends the sortie in the results screen", controlled)
        screenshot("results")
        tap("a")

        # Each sector's boss, reached by advancing the camera to the end of
        # the level; the ROM then runs its real warning and boss phases.
        launch()
        for sector in range(1, 5):
            assert get("dw_sector") == sector
            prepare()
            putword("dw_cam", IDS["DW_LEVEL_ROWS"] * 8)
            seen = set()
            for _ in range(400):
                seen.add(get("dw_phase"))
                if get("dw_phase") == P_BOSS:
                    break
                if get("dw_phase") == P_WAVES:
                    clear_actors()
                put("dw_invincible", 120)
                run(1)
            evidence.setdefault("boss_arrivals", []).append({"sector": sector,
                "hull": word("dw_boss_hp"), "max": word("dw_boss_max"), "phases": sorted(seen)})
            check({P_WARN, P_BOSS_IN, P_BOSS} <= seen and word("dw_boss_max") == BOSS_HP[sector - 1]
                  and word("dw_boss_hp") >= BOSS_HP[sector - 1] - 8,
                  f"Sector {sector} warns, then brings in its boss at full strength", controlled)
            hp = word("dw_boss_hp")
            for _ in range(120):
                put("dw_invincible", 120)
                run(1, bot_keys())
            screenshot(f"boss-{sector}")
            check(word("dw_boss_hp") < hp, f"Sector {sector} boss takes damage from the plane's guns", controlled)
            previous = get("dw_profile", 8)
            putword("dw_boss_hp", 1)
            for _ in range(600):
                if get("dw_state") != FLIGHT:
                    break
                put("dw_invincible", 120)
                run(1, bot_keys() if get("dw_phase") == P_BOSS else [])
            settle()
            check(get("dw_profile", 8) == max(previous, sector)
                  and get("dw_state") == (RESULTS if sector == 4 else SHOP),
                  f"Defeating boss {sector} plays the clear sequence and banks the sector", controlled)
            bank = word("dw_profile", 9)
            run(60)
            check(word("dw_profile", 9) == bank,
                  f"Sector {sector} completion credits cannot be farmed while waiting", controlled)
            if sector != 4:
                tokens = word("dw_run_tokens")
                hp = get("dw_hp")
                launch()
                check(word("dw_run_tokens") == tokens and word("dw_profile", 9) == bank
                      and get("dw_hp") == min(3, hp + 1),
                      f"Shop Next keeps the run, repairs a heart and does not re-credit sector {sector}",
                      controlled)
        check(get("dw_victory") == 1, "Four boss defeats reach the victory results", controlled)
        screenshot("victory")
        tap("a")
        launch()
        check(word("dw_run_tokens") == 0 and word("dw_score") == 0 and get("dw_sector") == 1,
              "Launching a fresh sortie resets run rewards", controlled)
        end_sortie()

        # Purchases with an arranged balance, through the real hangar menu.
        putword("dw_profile", 160, 9)
        tap("down")
        tap("up")
        for row in range(4):
            tap("a")
            if row != 3:
                tap("down")
        check(profile()[5:8] == [1, 1, 1] and profile()[10] == 1 and word("dw_profile", 9) == 0,
              "All four permanent upgrades buy through normal menu input", controlled)
        launch()
        check(get("dw_hp") == 4 and get("dw_allies") == 1,
              "Hull and ally upgrades add a heart and a wing dot at launch", controlled)
        prepare(hp=4)
        put("dw_power", 1)
        run(40)
        side = [get("dw_shots", i * SHOT + 9) for i in live("dw_shots", SHOTS, SHOT)]
        check(IDS["DW_S_SHOT_L"] in side and IDS["DW_S_SHOT_R"] in side,
              "The wing gun upgrade adds angled side shots", controlled)
        regeneration = []
        for level in (0, 1):
            prepare(hp=4)
            put("dw_profile", level, 7)
            put("dw_energy", 0)
            run(64)
            regeneration.append(get("dw_energy"))
        check(regeneration[1] >= regeneration[0] * 2 - 1 and regeneration[0] > 0,
              "The reactor upgrade speeds up burst-energy regeneration", controlled)
        evidence["reactor_regeneration"] = {"video_frames_each": 64,
             "baseline_energy": regeneration[0], "level_one_energy": regeneration[1]}
        end_sortie()
        saved = profile()
        battery = slots()
        check(valid(battery[:32]) and valid(battery[32:]),
              "Repeated saves alternate two valid records at A200 and A220", controlled)
        check(lcd_off_frames == 0 and display_off_calls == 0,
              "The LCD never switches off after boot, so screens never flash white")
        evidence["lcd"] = {"display_off_calls_after_boot": display_off_calls,
                           "lcd_off_frames_after_boot": lcd_off_frames,
                           "watched_frames": video_frames}
        p.stop(save=True)
        boot(game)
        check(profile() == saved and word("dw_best") > 0,
              "Dot, tokens, upgrades, cleared sectors and best score survive power cycling", controlled)
        tap("a")
        check(get("dw_state") == HANGAR, "Configured dot skips the builder after a power cycle", controlled)
        p.stop(save=False)
        # Independent malformed battery images, never the user's save.
        recent = 1 if ((battery[37] - battery[5]) & 255) < 128 else 0
        older = 1 - recent
        expected = battery[older * 32:(older + 1) * 32]
        for label, mutate in (("Corrupted latest checksum falls back to the prior record",
                               lambda b, o: b.__setitem__(o + 28, b[o + 28] ^ 1)),
                              ("Torn latest commit falls back to the prior record",
                               lambda b, o: b.__setitem__(o + 31, 0))):
            damaged = bytearray(battery)
            mutate(damaged, recent * 32)
            candidate = temp / ("case-%d.gbc" % len(checks))
            shutil.copyfile(ROM, candidate)
            boot(candidate, damaged)
            check(profile()[:9] == list(expected[6:15])
                  and profile()[9] == expected[15] | expected[16] << 8
                  and profile()[10] == expected[19],
                  label, controlled)
            p.stop(save=False)
        candidate = temp / "wrap.gbc"
        shutil.copyfile(ROM, candidate)
        boot(candidate, record(255, 2, 500) + record(0, 3, 501))
        check(get("dw_profile") == 3 and word("dw_profile", 9) == 501,
              "Generation zero is newer than generation 255 after wrap", controlled)
        tap("b")
        tap("right")
        tap("a")
        wrapped = slots()
        check(valid(wrapped[:32]) and wrapped[5] == 1 and wrapped[6] == 4,
              "Saving after generation wrap alternates slots and advances to generation one", controlled)
        p.stop(save=False)
        candidate = temp / "v1.gbc"
        shutil.copyfile(ROM, candidate)
        boot(candidate, record(7, 4, 321) + bytes(32))
        check(get("dw_profile") == 4 and word("dw_profile", 9) == 321 and get("dw_profile", 11) == 0,
              "A save from the first release loads with no ally upgrade", controlled)
        p.stop(save=False)
        candidate = temp / "range.gbc"
        shutil.copyfile(ROM, candidate)
        boot(candidate, record(3, 0, 50, fields=[9, 1, 1, 1, 1, 1, 1, 1, 4]) + bytes(32))
        check(profile() == [0] * 11, "A checksummed record with an impossible shape is rejected", controlled)
        p.stop(save=False)
        candidate = temp / "invalid.gbc"
        shutil.copyfile(ROM, candidate)
        boot(candidate, bytes(64))
        check(profile() == [0] * 11 and word("dw_best") == 0,
              "Two invalid battery records safely restore defaults", controlled)
        p.stop(save=False)
    evidence["campaign_completion_method"] = (
        "Controlled camera advance and boss-hull setup, then the ROM's real "
        "warning, boss, clear, shop and results code; validated separately "
        "from the ordinary button-only campaign in ordinary_campaign.")
    evidence["status"] = "passed"
    (BUILD / "validation.json").write_text(json.dumps(evidence, indent=2) + "\n")
    print("PASS:", len(checks), "checks; evidence:", BUILD / "validation.json")


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        evidence.update(status="failed", failure=type(error).__name__ + ": " + str(error))
        (BUILD / "validation.json").write_text(json.dumps(evidence, indent=2) + "\n")
        raise
    finally:
        if p is not None:
            p.stop(save=False)
