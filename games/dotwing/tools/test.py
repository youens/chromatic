#!/usr/bin/env python3
"""Exercise the actual cartridge in PyBoy; all saves live in a temp directory.

Ordinary play uses buttons alone. Controlled scenarios arrange WRAM actors or
SRAM records, then let the unmodified ROM process collisions/input/save/load.
The evidence manifest explicitly distinguishes those two kinds of validation.
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
TITLE, BUILDER, HANGAR, FLIGHT, PAUSE, SHOP, RESULTS, HELP, TAKEOFF = range(9)
checks, ordinary, controlled = [], [], []
evidence = {"emulator": "PyBoy " + importlib.metadata.version("pyboy"),
            "ordinary_input_checks": ordinary, "controlled_scenarios": controlled,
            "screenshots": [], "passed": checks, "status": "running"}
p = None
held = set()
video_frames = 0
phase = "ordinary input"


def check(condition, label, group=ordinary):
    assert condition, label
    checks.append(label)
    group.append(label)
    print("PASS:", label, flush=True)


def boot(path, raw=None):
    global p, held
    p = PyBoy(str(path), window="null", sound_emulated=False)
    p.set_emulation_speed(0)
    held = set()
    if raw is not None:
        p.memory[0x0000] = 0x0A
        p.memory[0x4000] = 0
        for i, byte in enumerate(raw):
            p.memory[0xA200 + i] = byte
        p.memory[0x0000] = 0
    run(180)
    assert get("dw_state") == TITLE, ("boot", get("dw_state"))


def run(frames=1, keys=()):
    global held, video_frames
    keys = set(keys)
    for key in held - keys:
        p.button_release(key)
    for key in keys - held:
        p.button_press(key)
    held = keys
    p.tick(frames)
    video_frames += frames


def tap(key):
    # Full menu redraws can span several video frames. Hold each transition
    # until the ROM has sampled it, then wait for its release to be sampled.
    bits = {"right": 1, "left": 2, "up": 4, "down": 8,
            "a": 16, "b": 32, "select": 64, "start": 128}
    run(1)
    until(lambda: get("dw_keys") == 0)
    run(1, [key])
    until(lambda: bool(get("dw_keys") & bits[key]), keys=[key])
    run(2, [key])
    run(1)
    until(lambda: get("dw_keys") == 0)
    run(2)


def get(name, offset=0):
    return p.memory[SYM["_" + name] + offset]


def put(name, value, offset=0):
    p.memory[SYM["_" + name] + offset] = value & 255


def word(name, offset=0):
    return get(name, offset) | (get(name, offset + 1) << 8)


def putword(name, value, offset=0):
    put(name, value, offset)
    put(name, value >> 8, offset + 1)


def profile():
    return [get("dw_profile", i) for i in range(9)] + [word("dw_profile", 9)]


def until(predicate, limit=600, keys=()):
    for _ in range(limit):
        if predicate():
            return
        run(1, keys)
    raise AssertionError("condition timed out: state=" + str(get("dw_state")))


def screenshot(name):
    # The ROM sets the next state before redrawing the LCD. Wait for the real
    # display to return and render two complete frames instead of capturing
    # the white LCD-off frame in the middle of a menu transition.
    until(lambda: bool(p.memory[0xFF40] & 0x80))
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


def record(generation, shape, tokens):
    data = bytearray(32)
    data[:5] = b"DOTW\x01"
    data[5] = generation
    data[6:15] = bytes([shape, 1, 1, 1, 1, 1, 1, 1, 4])
    data[15:19] = bytes([tokens & 255, tokens >> 8, 123, 0])
    total = checksum(data)
    data[28:30] = bytes([total & 255, total >> 8])
    data[31] = 0xA5
    return data


def clear_actors():
    for name, count, stride, active in (("dw_enemies", 6, 10, 9),
            ("dw_shots", 8, 7, 6), ("dw_bullets", 8, 7, 6),
            ("dw_drops", 4, 6, 5)):
        for i in range(count):
            put(name, 0, i * stride + active)


def prepare():
    assert get("dw_state") == FLIGHT
    clear_actors()
    putword("dw_px", 72)
    putword("dw_py", 94)
    putword("dw_stage_frame", 100)
    for name, value in (("dw_hp", 3), ("dw_invincible", 120),
            ("dw_burst", 0), ("dw_boss_active", 0), ("dw_energy", 100)):
        put(name, value)


def bullet(name, index, x, y, vx=0, vy=0):
    offset = index * 7
    putword(name, x, offset)
    putword(name, y, offset + 2)
    put(name, vx, offset + 4)
    put(name, vy, offset + 5)
    put(name, 1, offset + 6)


def enemy(index, x=72, y=44, hp=1):
    offset = index * 10
    putword("dw_enemies", x, offset)
    putword("dw_enemies", y, offset + 2)
    for field, value in ((4, 0), (5, 0), (6, 0), (7, hp), (8, 0), (9, 1)):
        put("dw_enemies", value, offset + field)


def pickup(kind):
    putword("dw_drops", word("dw_px") + 4)
    putword("dw_drops", word("dw_py"), 2)
    put("dw_drops", kind, 4)
    put("dw_drops", 1, 5)
    until(lambda: not get("dw_drops", 5), limit=20)
    run(2)


def launch():
    tap("start")
    assert get("dw_state") == TAKEOFF
    until(lambda: get("dw_state") == FLIGHT)
    run(2)


def end_sortie():
    tap("start")
    assert get("dw_state") == PAUSE
    tap("b")
    assert get("dw_state") == RESULTS
    tap("a")
    assert get("dw_state") == HANGAR


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
        screenshot("title")
        check(profile() == [0] * 10, "Fresh battery RAM initializes the profile")
        tap("up")
        check(get("dw_state") == HELP, "Flight guide opens with ordinary input")
        screenshot("guide")
        tap("b")
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
        screenshot("takeoff")
        until(lambda: get("dw_state") == FLIGHT)
        check(word("dw_stage_frame") <= 2, "Takeoff reaches flight after 72 logic updates")
        run(3)
        x, f = word("dw_px"), word("dw_frame")
        run(10, ["right"])
        fast = (word("dw_px") - x, (word("dw_frame") - f) & 65535)
        run(2)
        x, f = word("dw_px"), word("dw_frame")
        run(10, ["right", "a"])
        slow = (word("dw_px") - x, (word("dw_frame") - f) & 65535)
        run(2)
        check(fast[0] == fast[1] * 2 and slow[0] == slow[1] and slow[1] > 0,
              "Steering moves two pixels and A focus moves one per logic update")
        check(any(get("dw_shots", i * 7 + 6) for i in range(8)),
              "The plane auto-fires without holding a fire button")
        tap("start")
        f = word("dw_frame")
        run(120)
        check(get("dw_state") == PAUSE and word("dw_frame") == f,
              "Pause freezes gameplay for 120 emulator frames")
        screenshot("pause")
        tap("start")
        check(get("dw_state") == FLIGHT, "Start resumes the paused sortie")
        f = word("dw_frame")
        started = time.monotonic()
        run(120)
        elapsed = time.monotonic() - started
        delta = (word("dw_frame") - f) & 65535
        evidence["performance"] = {"emulator_frames": 120, "logic_updates": delta,
             "logic_hz_at_59_7275_video_hz": round(delta / 120 * 59.7275, 2),
             "unthrottled_emulation_seconds": round(elapsed, 4)}
        check(delta >= 110, "Normal gameplay sustains approximately 60 logic Hz")
        # A complete ordinary campaign attempt: observation chooses buttons,
        # never changes RAM. Buy upgrades with currency actually earned in play.
        session_frames = 0
        session_start = video_frames
        captures = 0
        ordinary_bosses, ordinary_clears, purchases = set(), [], []
        while get("dw_state") in (FLIGHT, SHOP) and video_frames - session_start < 16000:
            if get("dw_state") == SHOP:
                ordinary_clears.append(get("dw_sector"))
                screenshot("ordinary-shop-" + str(get("dw_sector")))
                for row in range(3):
                    while get("dw_menu_row") != row:
                        tap("down")
                    while get("dw_profile", 5 + row) < 3:
                        level = get("dw_profile", 5 + row)
                        price = (40, 80, 140)[level]
                        if word("dw_profile", 9) < price:
                            break
                        tap("a")
                        assert get("dw_profile", 5 + row) == level + 1
                        purchases.append({"sector": get("dw_sector"),
                                          "upgrade_row": row, "level": level + 1,
                                          "price": price})
                launch()
                continue
            x = word("dw_px")
            targets = [word("dw_enemies", i * 10) for i in range(6)
                       if get("dw_enemies", i * 10 + 9)]
            target = min(targets, key=lambda value: abs(value - x)) if targets else 72
            if get("dw_boss_active") == 1:
                target = word("dw_boss_x") + 8
                sector = get("dw_sector")
                if word("dw_boss_y") == 20 and sector not in ordinary_bosses:
                    screenshot("ordinary-boss-" + str(sector))
                    ordinary_bosses.add(sector)
            keys = []
            if abs(target - x) > 3:
                keys.append("right" if target > x else "left")
            close = any(get("dw_bullets", i * 7 + 6)
                        and abs(word("dw_bullets", i * 7) - x) < 18
                        and 68 < word("dw_bullets", i * 7 + 2) < 112
                        for i in range(8))
            if close and get("dw_energy") >= 50:
                keys.append("b")
            if len(targets) >= 2 and not captures:
                screenshot("gameplay")
                captures += 1
            run(5, keys)
            session_frames += 5
        if not captures:
            screenshot("gameplay")
        if get("dw_victory"):
            ordinary_clears.append(4)
        evidence["ordinary_sortie"] = {"additional_emulator_frames": video_frames - session_start,
             "flight_bot_frames": session_frames,
             "score": word("dw_score"), "kills": word("dw_kills"),
             "run_tokens": word("dw_run_tokens"), "sector": get("dw_sector"),
             "ending_state": get("dw_state"), "victory": bool(get("dw_victory")),
             "cleared_sectors": ordinary_clears, "upgrade_purchases": purchases,
             "method": "Button-only enemy/boss tracking, defensive burst and shop purchases; no WRAM writes."}
        check(word("dw_kills") > 0, "Ordinary button-only flight defeats enemies")
        if purchases:
            check(True, "Ordinary campaign buys upgrades with earned tokens")
        if get("dw_victory"):
            check(True, "Ordinary button-only campaign defeats all four bosses")
        if get("dw_state") == FLIGHT:
            end_sortie()
        elif get("dw_state") == RESULTS:
            screenshot("ordinary-results")
            tap("a")
        elif get("dw_state") == SHOP:
            launch()
            end_sortie()
        assert get("dw_state") == HANGAR
        # Shop currency has already been credited. A new, empty sortie must
        # not repeat that credit when ended through the actual pause menu.
        bank = word("dw_profile", 9)
        launch()
        end_sortie()
        check(word("dw_profile", 9) == bank,
              "Ending an empty sortie after the shop cannot duplicate prior credits")
        # Controlled scenarios begin here. Return upgrades to baseline so the
        # collision and purchase tests do not depend on the bot's shop choices.
        phase = "controlled WRAM/SRAM scenario"
        for offset in (5, 6, 7):
            put("dw_profile", 0, offset)
        launch()
        prepare()
        initial_tokens, initial_kills = word("dw_run_tokens"), word("dw_kills")
        enemy(0)
        bullet("dw_shots", 0, 76, 44)
        until(lambda: word("dw_kills") > initial_kills, limit=30)
        run(2)
        check(word("dw_kills") == initial_kills + 1
              and word("dw_run_tokens") == initial_tokens + 1,
              "A real projectile collision defeats an arranged enemy and awards tokens", controlled)
        clear_actors()
        tokens, score = word("dw_run_tokens"), word("dw_score")
        pickup(0)
        check(word("dw_run_tokens") == tokens + 6 and word("dw_score") == score + 50,
              "Token pickup awards six tokens and fifty score", controlled)
        put("dw_power", 0)
        pickup(1)
        pickup(1)
        pickup(1)
        check(get("dw_power") == 2, "Power pickups strengthen the weapon and clamp at level two", controlled)
        put("dw_hp", 2)
        pickup(2)
        check(get("dw_hp") == 3, "Shield pickup restores one hull point", controlled)
        put("dw_energy", 0)
        tokens = word("dw_run_tokens")
        pickup(3)
        check(get("dw_energy") == 100 and word("dw_run_tokens") == tokens + 4,
              "Token boost fills burst energy and awards four tokens", controlled)
        clear_actors()
        put("dw_invincible", 0)
        put("dw_combo", 5)
        bullet("dw_bullets", 0, word("dw_px") + 4, word("dw_py"))
        until(lambda: get("dw_hp") == 2, limit=20)
        run(2)
        check(get("dw_power") == 1 and get("dw_combo") == 0
              and get("dw_invincible") > 90, "A hostile projectile damages hull, reduces power and grants invincibility", controlled)
        bullet("dw_bullets", 0, word("dw_px") + 4, word("dw_py"))
        run(5)
        check(get("dw_hp") == 2, "Invincibility prevents an immediate second hit", controlled)
        prepare()
        for i in range(3):
            enemy(i, 20 + i * 30)
            bullet("dw_bullets", i, 10 + i * 20, 60)
        put("dw_boss_active", 1)
        put("dw_boss_hp", 40)
        put("dw_boss_max", 40)
        putword("dw_boss_x", 64)
        putword("dw_boss_y", 20)
        tokens, kills = word("dw_run_tokens"), word("dw_kills")
        tap("b")
        check(48 <= get("dw_energy") <= 53 and get("dw_burst") > 0
              and not any(get("dw_bullets", i * 7 + 6) for i in range(8))
              and word("dw_kills") == kills + 3 and word("dw_run_tokens") == tokens + 9
              and get("dw_boss_hp") <= 28,
              "B burst spends fifty energy, clears bullets, destroys enemies and damages the boss", controlled)
        screenshot("burst")
        prepare()
        put("dw_energy", 0)
        tap("b")
        check(get("dw_burst") == 0, "Insufficient energy prevents a burst", controlled)
        # End this scenario sortie before the four-sector scenario starts.
        end_sortie()
        launch()
        for sector in range(1, 5):
            prepare()
            assert get("dw_sector") == sector
            putword("dw_stage_frame", 1799)
            until(lambda: get("dw_boss_active") == 1, limit=20)
            check(get("dw_boss_hp") == 54 + sector * 14,
                  f"Sector {sector} spawns its boss at the real stage timer", controlled)
            until(lambda: word("dw_boss_y") == 20, limit=100)
            screenshot(f"boss-{sector}")
            clear_actors()
            put("dw_boss_hp", 1)
            previous_cleared = get("dw_profile", 8)
            bullet("dw_shots", 0, word("dw_boss_x") + 12, word("dw_boss_y") + 8)
            until(lambda: get("dw_state") != FLIGHT, limit=30)
            run(5)
            check(get("dw_profile", 8) == max(previous_cleared, sector)
                  and get("dw_state") == (RESULTS if sector == 4 else SHOP),
                  f"An arranged final hit completes sector {sector} through the real transition", controlled)
            bank = word("dw_profile", 9)
            run(60)
            check(word("dw_profile", 9) == bank,
                  f"Sector {sector} completion credits cannot be farmed while waiting", controlled)
            if sector != 4:
                old_tokens = word("dw_run_tokens")
                screenshot("shop" if sector == 1 else f"shop-{sector}")
                launch()
                check(word("dw_run_tokens") == old_tokens
                      and word("dw_profile", 9) == bank,
                      f"Shop Next preserves the run without re-crediting sector {sector}", controlled)
        check(get("dw_victory") == 1, "Four controlled boss defeats reach AI Supremacy results", controlled)
        screenshot("victory")
        tap("a")
        bank = word("dw_profile", 9)
        launch()
        check(word("dw_run_tokens") == 0 and word("dw_score") == 0,
              "Launching a fresh sortie resets run rewards", controlled)
        prepare()
        # Earn purchase funding using real pickup updates at arranged positions.
        for _ in range(max(0, (120 - bank + 5) // 6)):
            pickup(0)
        end_sortie()
        before = word("dw_profile", 9)
        for row in range(3):
            tap("a")
            if row != 2:
                tap("down")
        check(profile()[5:8] == [1, 1, 1]
              and word("dw_profile", 9) == before - 120,
              "All three permanent upgrades buy through normal menu input", controlled)
        launch()
        check(get("dw_hp") == 4, "Permanent shield upgrade increases starting hull", controlled)
        prepare()
        kills = word("dw_kills")
        enemy(0, hp=2)
        bullet("dw_shots", 0, 76, 44)
        until(lambda: word("dw_kills") > kills, limit=20)
        check(word("dw_kills") == kills + 1,
              "Permanent wing upgrade lets one projectile defeat a two-hull enemy", controlled)
        regeneration = []
        for level in (0, 1):
            prepare()
            put("dw_profile", level, 7)
            put("dw_energy", 0)
            putword("dw_frame", 96)
            run(60)
            regeneration.append(get("dw_energy"))
        put("dw_profile", 1, 7)
        check(regeneration[1] > regeneration[0] * 1.6,
              "Permanent reactor upgrade increases burst-energy regeneration", controlled)
        evidence["reactor_regeneration"] = {"video_frames_each": 60,
             "baseline_energy": regeneration[0], "level_one_energy": regeneration[1]}
        end_sortie()
        saved = profile()
        battery = slots()
        check(valid(battery[:32]) and valid(battery[32:]),
              "Repeated saves alternate two valid records at A200 and A220", controlled)
        p.stop(save=True)
        boot(game)
        check(profile() == saved and word("dw_best") > 0,
              "Dot, tokens, upgrades, cleared sectors and best score survive power cycling", controlled)
        tap("a")
        check(get("dw_state") == HANGAR, "Configured dot skips the builder after a power cycle", controlled)
        screenshot("saved-hangar")
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
            candidate = temp / ("case-" + str(len(checks)) + ".gbc")
            shutil.copyfile(ROM, candidate)
            boot(candidate, damaged)
            check(profile()[:9] == list(expected[6:15])
                  and profile()[9] == expected[15] | expected[16] << 8,
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
        candidate = temp / "invalid.gbc"
        shutil.copyfile(ROM, candidate)
        boot(candidate, bytes(64))
        check(profile() == [0] * 10 and word("dw_best") == 0,
              "Two invalid battery records safely restore defaults", controlled)
        p.stop(save=False)
    evidence["campaign_completion_method"] = (
        "Controlled stage timer/final-hit WRAM setup plus actual ROM collision, "
        "shop and results code is validated separately from the ordinary button-only "
        "campaign attempt recorded in ordinary_sortie.")
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
