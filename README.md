# Smart Intersection Guardian

> Hackathon MVP — inteligentne skrzyżowanie z wykrywaniem wypadków.
> Hackathon MVP — smart intersection with accident detection.

---

## 🇵🇱 PL

### Co to jest

Inteligentne skrzyżowanie w dwóch trybach:

- **Tryb normalny** — światła przełączają się adaptacyjnie, zielone trwa dłużej po bardziej obciążonej stronie.
- **Tryb awaryjny** — po wykryciu wypadku (wstrząs / przycisk / symulacja) system blokuje wszystkie kierunki na czerwono, włącza alarm i wysyła komunikat do drugiej płytki / dashboardu.

Backend w czystym C, bez zależności sprzętowych. Całe piny / LEDy / czujniki siedzą za warstwą HAL — dzięki temu logikę można pisać, testować i debugować na laptopie zanim sprzęt w ogóle działa.

### Struktura projektu

```
include/              — nagłówki publiczne modułów
  soft_timer.h        — programowe timery (wiele slotów na jednym HW timerze)
  intersection_fsm.h  — maszyna stanów skrzyżowania
  traffic_logic.h     — adaptacyjny czas trwania zielonego
  event_logger.h      — ring buffer logów zdarzeń
  protocol.h          — serializacja pakietów między nRF54L15 ↔ nRF7002
  hal.h               — interfejs warstwy sprzętowej
  hal_stub.h          — setery fake-inputów (tylko PC / testy)

src/
  soft_timer.c, intersection_fsm.c, traffic_logic.c,
  event_logger.c, protocol.c         — implementacje modułów
  hal_stub.c                         — HAL backend dla PC (printy zamiast LEDów)
  hal_arduino.c.template             — szkielet HAL pod Arduino (pin mapa)
  main.c                             — demo pokazujące pełny scenariusz

test/                 — testy jednostkowe (własny mini-framework w test_common.h)
CMakeLists.txt        — buduje bibliotekę + demo + testy
```

### Kolejność zależności

1. `soft_timer` — reszta z niego korzysta
2. `intersection_fsm` — serce systemu, używa timerów
3. `traffic_logic` — generuje `EVT_HIGH_TRAFFIC` do FSM
4. `event_logger` — loguje zmiany stanów
5. `protocol` — pakuje statusy do wysyłki

### Zasady projektu (świadome ograniczenia)

- **Zero `#include` hardware'u** w modułach core — żadnego GPIO, żadnych nagłówków Zephyr.
- **Zero `malloc`** — wszystko statyczne, bezpieczne na mikrokontrolerze.
- **Zero zmiennych globalnych** — cały stan w strukturach przekazywanych przez wskaźnik.
- Każdy moduł kompiluje się samodzielnie na PC.
- Sprzęt wpinamy przez HAL — core nie wie i nie musi wiedzieć, że istnieje Arduino.

### Jak uruchomić

#### Z CMake (CLion / terminal)

```bash
cmake -S . -B build
cmake --build build
./build/guardian_demo      # demo
ctest --test-dir build     # wszystkie testy
```

#### Bez CMake (szybki build)

```bash
cc -std=c11 -Iinclude src/*.c -o guardian_demo
./guardian_demo
```

### Jak testować

Są testy per moduł — uruchom wszystkie naraz:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Albo pojedynczo:

```bash
./build/test_fsm
./build/test_soft_timer
./build/test_traffic_logic
./build/test_event_logger
./build/test_protocol
./build/test_hal_stub
```

Testy używają własnego mini-frameworka w [test/test_common.h](test/test_common.h) — `ASSERT_EQ`, `ASSERT_TRUE`, `RUN_TEST`. Nowy test to po prostu dopisanie funkcji + wpis w `RUN_TEST`, a w `CMakeLists.txt` jedna linijka `add_guardian_test(...)`.

### Jak podłączyć Arduino

1. Skopiuj `src/hal_arduino.c.template` → `hal_arduino.c`.
2. Ustaw `#if 0` na `#if 1` (albo usuń makro).
3. Zweryfikuj pin mapę w pliku (LED_A_RED, HC-SR04 TRIG/ECHO, TEMT6000 itd.) — teraz są wartości domyślne.
4. W buildzie dla Arduino wywal `hal_stub.c`, wrzuć `hal_arduino.c`. Prototypy są identyczne, więc core się nie zmienia.
5. Z `main.c` zamień `printf`-y na wywołania HAL (`hal_set_light(HAL_DIR_A, HAL_LIGHT_GREEN)`). Logika FSM zostaje ta sama.

### Sprzęt, który mamy

| Element                         | Gdzie w HAL                         |
| ------------------------------- | ----------------------------------- |
| LED 3mm/5mm (światła)           | `hal_set_light(dir, color)`         |
| HC-SR04 (odległość)             | `hal_ultrasonic_distance_cm()` → wykrywanie pojazdu |
| TEMT6000 (natężenie światła)    | `hal_light_sensor()` → tryb dzień/noc |
| Głośnik YD58                    | `hal_set_alarm(true)`               |
| Wyświetlacz 4-cyfrowy (HC595)   | `hal_display_number(ms / 1000)` → countdown fazy |
| Joystick + przycisk             | `hal_button_pressed()` → emergency  |
| BME280                          | (opcjonalnie: kontekst pogodowy w logach) |

### Co dodać w następnym kroku

- Właściwa pętla w `main.c` wywołująca `timer_tick()` co ~10 ms i czytająca HAL jako źródło eventów.
- `NORMAL → HIGH_TRAFFIC → NORMAL` sterowane przez timer fazy, nie ręczne `EVT_TIMEOUT`.
- Debounce przycisku awaryjnego w HAL albo przez `soft_timer`.
- Na gatewayu (nRF7002) implementacja dashboardu czytającego `PKT_STATUS` i `PKT_ALERT`.
- Test end-to-end: stub HAL → krok w czasie → sprawdź stan FSM + wysłany pakiet.

---

## 🇬🇧 EN

### What this is

A smart road intersection in two modes:

- **Normal mode** — traffic lights switch adaptively, green stays longer on the heavier direction.
- **Emergency mode** — when a crash is detected (shock / button / simulation) the system locks all directions to red, fires the alarm, and sends an alert to the second board / dashboard.

Backend is pure C with no hardware dependencies. All pins / LEDs / sensors sit behind a HAL layer — so logic can be written, tested, and debugged on a laptop before the hardware is even working.

### Project layout

```
include/              — public module headers
  soft_timer.h        — software timers (many logical slots on one HW timer)
  intersection_fsm.h  — intersection state machine
  traffic_logic.h     — adaptive green-light duration
  event_logger.h      — ring buffer for events
  protocol.h          — packet (de)serialization between nRF54L15 ↔ nRF7002
  hal.h               — hardware abstraction interface
  hal_stub.h          — fake-input setters (PC / tests only)

src/
  soft_timer.c, intersection_fsm.c, traffic_logic.c,
  event_logger.c, protocol.c         — module implementations
  hal_stub.c                         — HAL backend for PC (printf instead of LEDs)
  hal_arduino.c.template             — Arduino HAL skeleton (pin map)
  main.c                             — demo walking through the full scenario

test/                 — unit tests (tiny in-house framework in test_common.h)
CMakeLists.txt        — builds library + demo + tests
```

### Dependency order

1. `soft_timer` — everyone else uses it
2. `intersection_fsm` — system core, uses timers
3. `traffic_logic` — feeds `EVT_HIGH_TRAFFIC` into the FSM
4. `event_logger` — records state changes
5. `protocol` — packages statuses for transmission

### Project rules (deliberate constraints)

- **Zero hardware `#include`** in the core — no GPIO, no Zephyr headers.
- **Zero `malloc`** — everything static, microcontroller-safe.
- **Zero globals** — all state lives in structs passed by pointer.
- Every module compiles standalone on PC.
- Hardware plugs in via the HAL — the core doesn't need to know Arduino exists.

### How to build & run

#### With CMake (CLion / terminal)

```bash
cmake -S . -B build
cmake --build build
./build/guardian_demo      # demo binary
ctest --test-dir build     # all tests
```

#### Without CMake (quick build)

```bash
cc -std=c11 -Iinclude src/*.c -o guardian_demo
./guardian_demo
```

### How to test

Tests are per-module. Run everything at once:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Or run individual binaries:

```bash
./build/test_fsm
./build/test_soft_timer
./build/test_traffic_logic
./build/test_event_logger
./build/test_protocol
./build/test_hal_stub
```

Tests use a tiny in-house framework in [test/test_common.h](test/test_common.h) — `ASSERT_EQ`, `ASSERT_TRUE`, `RUN_TEST`. Adding a new test = write a function, add it to `RUN_TEST`, register the file in `CMakeLists.txt` with one `add_guardian_test(...)` line.

### Wiring in Arduino 

1. Copy `src/hal_arduino.c.template` → `hal_arduino.c`.
2. Flip `#if 0` to `#if 1` (or drop the guard).
3. Check the pin map in the file (LED_A_RED, HC-SR04 TRIG/ECHO, TEMT6000, …) — current values are placeholders.
4. In the Arduino build, drop `hal_stub.c`, include `hal_arduino.c`. Prototypes are identical — the core doesn't change.
5. In `main.c`, replace `printf`s with HAL calls (`hal_set_light(HAL_DIR_A, HAL_LIGHT_GREEN)`). FSM logic stays the same.

### Hardware we have

| Part                           | HAL surface                                 |
| ------------------------------ | ------------------------------------------- |
| LEDs 3mm/5mm (traffic lights)  | `hal_set_light(dir, color)`                 |
| HC-SR04 (distance)             | `hal_ultrasonic_distance_cm()` → vehicle detection |
| TEMT6000 (light level)         | `hal_light_sensor()` → day/night mode       |
| YD58 speaker                   | `hal_set_alarm(true)`                       |
| 4-digit display (HC595)        | `hal_display_number(ms / 1000)` → phase countdown |
| Joystick + button              | `hal_button_pressed()` → emergency          |
| BME280                         | (optional: weather context in logs)         |

### What to add next

- A proper main loop calling `timer_tick()` every ~10 ms and reading HAL as the event source.
- `NORMAL → HIGH_TRAFFIC → NORMAL` driven by phase timer, not manual `EVT_TIMEOUT`.
- Emergency-button debounce in HAL or via `soft_timer`.
- Gateway side (nRF7002): dashboard consuming `PKT_STATUS` + `PKT_ALERT`.
- End-to-end test: stub HAL → advance time → assert FSM state + emitted packet.

---

### Module cheat-sheet / Ściąga modułów

| Module               | Key types                                   | Key functions                                       |
| -------------------- | ------------------------------------------- | --------------------------------------------------- |
| `soft_timer`         | `timer_mgr_t`, `soft_timer_t`               | `timer_start / timer_stop / timer_tick / timer_remaining` |
| `intersection_fsm`   | `intersection_ctx_t`, `intersection_state_t`, `intersection_event_t` | `intersection_init / intersection_update`           |
| `traffic_logic`      | `traffic_config_t`, `traffic_phase_t`       | `traffic_default_config / traffic_compute_phase`    |
| `event_logger`       | `event_logger_t`, `log_entry_t`             | `logger_init / logger_write / logger_read_last / logger_clear` |
| `protocol`           | `status_payload_t`, `alert_payload_t`, `packet_type_t` | `protocol_serialize_status / _alert`, `protocol_deserialize`, `protocol_checksum` |
| `hal` (stub/arduino) | `hal_light_t`, `hal_direction_t`            | `hal_init / hal_set_light / hal_button_pressed / hal_ultrasonic_distance_cm / …` |

**Powodzenia na hackathonie! / Good luck at the hackathon!** 🚦
