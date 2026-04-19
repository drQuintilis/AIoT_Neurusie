# AIoT Neurusie — Smart Intersection Guardian

> Hackathon MVP: inteligentne skrzyżowanie z wykrywaniem wypadków, adaptacyjnym sterowaniem świateł i modelem TinyML trenowanym pod nRF54L15.

Repo zbiera trzy kawałki, które składają się w jeden system:

1. **Rdzeń C** (`include/`, `src/`, `test/`) — logika skrzyżowania: FSM, software timery, adaptacyjne fazy, logger zdarzeń, serializacja pakietów. Wszystko za warstwą HAL, więc kompiluje się i testuje na laptopie bez żadnego sprzętu.
2. **Gateway `nRF7002/`** — szkielet Zephyra, docelowo odbiornik `PKT_STATUS` / `PKT_ALERT` z nRF54L15 i dashboard.
3. **`Hacknarok/`** — piaskownica TinyML: środowisko sterowania skrzyżowaniem, trening tabelarycznej polityki Q + mała liniowa polityka eksportowana jako nagłówek C do flashowania na nRF54L15, plus zabawy z akustyczną detekcją wypadków (`cnn8rnn-audioset-sed`).

---

## Tryby działania

- **Normalny** — światła przełączają się adaptacyjnie, zielone trwa dłużej po bardziej obciążonej stronie (czujnik HC-SR04 lub tripwire z polityki RL).
- **Awaryjny** — po wykryciu wypadku (wstrząs / przycisk / detekcja akustyczna / ręczna symulacja) wszystkie kierunki lecą na czerwono, alarm, pakiet `PKT_ALERT` do gatewaya.

---

## Struktura repo

```
include/              nagłówki rdzenia C
  soft_timer.h        programowe timery (wiele slotów na jednym HW timerze)
  intersection_fsm.h  maszyna stanów skrzyżowania
  traffic_logic.h     adaptacyjny czas trwania zielonego
  event_logger.h      ring buffer logów zdarzeń
  protocol.h          serializacja pakietów nRF54L15 ↔ nRF7002
  hal.h               interfejs warstwy sprzętowej
  hal_stub.h          setery fake-inputów (PC / testy)

src/                  implementacje modułów + main.c (demo) + HAL stub + szablon HAL Arduino
test/                 testy jednostkowe (własny mini-framework w test_common.h)
CMakeLists.txt        buduje bibliotekę + demo + testy

nRF7002/              Zephyrowy projekt gatewaya (na razie pusty main.c)
  CMakeLists.txt
  prj.conf
  src/main.c

Hacknarok/            piaskownica TinyML (Python + eksport do C)
  tinyml_intersection.py   trener polityki sterowania skrzyżowaniem
  out/
    nrf54l15_intersection_policy.h    wyeksportowana polityka do flashowania
    nrf54l15_intersection_model.*     pełny model + metadane
    training_history.png
  backup/                  starsza wersja (agent.py, env.py, learn.py, train.py,
                           Q-tabela jako q_table_data.h, traffic generator,
                           testy akustyczne i serialu)
  cnn8rnn-audioset-sed/    model detekcji zdarzeń akustycznych (AudioSet SED)
  venv/                    środowisko Pythona do treningu
```

---

## Rdzeń C — zasady projektu

- **Zero `#include` sprzętu** w modułach core — brak GPIO, brak nagłówków Zephyra.
- **Zero `malloc`** — wszystko statyczne, bezpieczne na mikrokontrolerze.
- **Zero globali** — stan w strukturach, przekazywany przez wskaźnik.
- Każdy moduł kompiluje się samodzielnie na PC.
- Sprzęt wpinamy przez HAL — core nie wie, że istnieje Arduino / nRF.

Kolejność zależności: `soft_timer` → `intersection_fsm` → `traffic_logic` → `event_logger` → `protocol`.

---

## Build i uruchomienie

### Rdzeń C (CMake)

```bash
cmake -S . -B build
cmake --build build
./build/guardian_demo          # demo pełnego scenariusza
ctest --test-dir build --output-on-failure   # wszystkie testy
```

Albo bez CMake:

```bash
cc -std=c11 -Iinclude src/*.c -o guardian_demo && ./guardian_demo
```

Pojedyncze testy: `./build/test_fsm`, `./build/test_soft_timer`, `./build/test_traffic_logic`, `./build/test_event_logger`, `./build/test_protocol`, `./build/test_hal_stub`.

Nowy test = funkcja + `RUN_TEST` w pliku + jedna linia `add_guardian_test(...)` w `CMakeLists.txt`.

### Gateway nRF7002

Projekt Zephyra w `nRF7002/` — build standardowo przez `west build -b nrf7002dk_nrf5340_cpuapp` (docelowo: odbiór `PKT_STATUS` / `PKT_ALERT` z rdzenia przez UART/SPI/BLE). `main.c` póki co to szkielet.

### TinyML (Hacknarok)

```bash
cd Hacknarok
./venv/bin/python3 tinyml_intersection.py --arms 4 --episodes 80 --ticks 1800
# opcjonalnie wykres:
./venv/bin/python3 tinyml_intersection.py --arms 4 --episodes 80 --ticks 1800 \
    --visualize-training --visualization-output out/training_history.png
```

Skrypt zapisuje wytrenowaną politykę do `Hacknarok/out/nrf54l15_intersection_policy.h` — ten plik trafia bezpośrednio do firmware nRF54L15. Polityka jest mała liniowa Q z dwiema akcjami: `0` = trzymaj fazę, `1` = zmień fazę. Trening karze przełączenia (żeby nie migało), nagradza przepustowość, lekko mocniej karze korek — żeby agent nie unikał przełączeń za wszelką cenę.

Model akustyczny (`cnn8rnn-audioset-sed/`) to gotowy SED z AudioSet — używany do detekcji dźwięków kolizji/klaksonów jako dodatkowy trigger trybu awaryjnego.

---

## Podłączanie Arduino (HAL)

1. Skopiuj `src/hal_arduino.c.template` → `hal_arduino.c`.
2. Przełącz `#if 0` na `#if 1` (albo usuń guard).
3. Zweryfikuj pin mapę (LED_A_RED, HC-SR04 TRIG/ECHO, TEMT6000, …) — obecne są domyślne.
4. W buildzie Arduino wymień `hal_stub.c` na `hal_arduino.c`. Prototypy identyczne — core bez zmian.
5. W `main.c` zamień `printf`-y na wywołania HAL (`hal_set_light(HAL_DIR_A, HAL_LIGHT_GREEN)`).

### Sprzęt → HAL

| Element                         | HAL                                                |
| ------------------------------- | -------------------------------------------------- |
| LED 3/5 mm (światła)            | `hal_set_light(dir, color)`                        |
| HC-SR04 (odległość)             | `hal_ultrasonic_distance_cm()` → detekcja pojazdu  |
| TEMT6000 (natężenie światła)    | `hal_light_sensor()` → tryb dzień/noc              |
| Głośnik YD58                    | `hal_set_alarm(true)`                              |
| Wyświetlacz 4-cyfrowy (HC595)   | `hal_display_number(ms / 1000)` → countdown fazy   |
| Joystick + przycisk             | `hal_button_pressed()` → emergency                 |
| BME280                          | (opcjonalnie: kontekst pogodowy w logach)          |

---

## Ściąga modułów

| Moduł                | Kluczowe typy                                            | Kluczowe funkcje                                                |
| -------------------- | -------------------------------------------------------- | --------------------------------------------------------------- |
| `soft_timer`         | `timer_mgr_t`, `soft_timer_t`                            | `timer_start` / `_stop` / `_tick` / `_remaining`                |
| `intersection_fsm`   | `intersection_ctx_t`, `intersection_state_t`, `_event_t` | `intersection_init` / `intersection_update`                     |
| `traffic_logic`      | `traffic_config_t`, `traffic_phase_t`                    | `traffic_default_config` / `traffic_compute_phase`              |
| `event_logger`       | `event_logger_t`, `log_entry_t`                          | `logger_init` / `_write` / `_read_last` / `_clear`              |
| `protocol`           | `status_payload_t`, `alert_payload_t`, `packet_type_t`   | `protocol_serialize_status` / `_alert`, `_deserialize`, `_checksum` |
| `hal` (stub/arduino) | `hal_light_t`, `hal_direction_t`                         | `hal_init` / `_set_light` / `_button_pressed` / `_ultrasonic_distance_cm` / … |

---

## Co jest do zrobienia

- **Rdzeń C:** prawdziwa pętla w `main.c` z `timer_tick()` co ~10 ms i HAL jako źródłem eventów; `NORMAL → HIGH_TRAFFIC → NORMAL` z timera fazy zamiast ręcznego `EVT_TIMEOUT`; debounce przycisku awaryjnego; test end-to-end (stub HAL → krok w czasie → sprawdź FSM + wysłany pakiet).
- **nRF7002:** implementacja dashboardu czytającego `PKT_STATUS` / `PKT_ALERT`.
- **TinyML:** spięcie `nrf54l15_intersection_policy.h` z FSM (polityka decyduje o `EVT_HIGH_TRAFFIC` zamiast progu z `traffic_logic`); wpięcie detektora akustycznego jako triggera emergency.
- **Porządki w `Hacknarok/`:** `backup/` i `cnn8rnn-audioset-sed-bak/` do wywalenia albo opisania, `venv/` i `Hacknarok.zip` do `.gitignore`.
