# Akwarium ESP32 - telemetria i sterowanie temperaturą wody

Urządzenie IoT na ściankę akwarium: mierzy temperaturę, TDS, zmętnienie, światło i ciśnienie napowietrzania,
utrzymuje zadaną temperaturę (regulator dwustawny z histerezą), pokazuje dane na OLED i na stronie WWW.

## Struktura projektu

| Ścieżka | Zawartość |
| `src/main.cpp` | pętla główna|
| `src/TemperatureManager.*` | DS18B20, histereza, zabezpieczenia, zapis Tzad w NVS |
| `src/TdsSensor.*`, `TurbiditySensor.*`, `PressureSensor.*`, `LightSensor.*` | czujniki |
| `src/SensorMath.h` | funckje przeliczeniowe |
| `src/AlarmManager.*`, `Debounce.h` | alarmy z opóźnieniami |
| `src/DisplayManager.*` | OLED, zmiana stron |
| `src/WifiManager.*`, `WebInterface.*`, `WebApi.h`, `WebPage.h` | Wi-Fi i serwer WWW |
| `src/Watchdog.h` | watchdog i powód restartu |
| `src/Bringup.*`, `BringupParser.h` | firmware do pierwszego uruchomienia |
| `src/config.h` | **wszystkie piny, progi i stałe kalibracyjne** |
| `src/secrets.example.h` | wzór pliku z danymi Wi-Fi |
| `diagram.json`, `wokwi.toml` | symulacja Wokwi |
| `docs/Akwarium_dokumentacja.docx` | dokumentacja projektowa |
| `docs/INSTRUKCJA_URUCHOMIENIA.md` | kompletna instrukcja montażu i pierwszego uruchomienia |
| `docs/schemat_polaczen.*` | schemat połączeń (generowany przez `docs/make_schematic.py`) |
| `tools/ui_preview.html` | podgląd strony WWW bez urządzenia |

## Środowiska PlatformIO

| Środowisko | Do czego | Kiedy |
|---|---|---|
| `sim` | do symulacji na pc (potencjometry zamiast części czujników) | codziennie, bez sprzętu |
| `selftest` | testy logiki na (wirtualnym) ESP32, wynik w terminalu| po każdej zmianie logiki |
| `bringup_sim` | firmware diagnostyczny w symulatorze | działanie symulacyjnie |
| `bringup` | **firmware diagnostyczny na prawdziwym ESP32** | pierwsze uruchomienie sprzętu |
| `esp32dev` | **finalny firmware na prawdziwy ESP32** | po kalibracji, NAJWAZNIEJSZY!!!! |

Symulacja Wokwi: w `wokwi.toml` ścieżki `firmware`/`elf` muszą wskazywać na zbudowane środowisko
(`.pio/build/<środowisko>/firmware.bin`). Po zmianie zawsze zapisz plik i zrestartuj symulację.

## Konfiguracja przed wgraniem na sprzęt

1. Skopiuj `src/secrets.example.h` jako `src/secrets.h` i wpisz nazwę oraz hasło sieci Wi-Fi (tylko **2,4 GHz**).
   Plik jest w `.gitignore`. Bez niego środowisko `esp32dev` celowo się nie kompiluje.
2. Wartości w `config.h` wypełnić później. 

## Podłączenia (wersja sprzętowa)

Schemat: `docs/schemat_polaczen.png`. Wszystkie masy (GND) połączone razem.

| Sygnał | Element | Pin ESP32 | Uwagi |
|---|---|---|---|
| 1-Wire | DS18B20 (DQ) | GPIO4 | rezystor 4,7 kΩ DQ↔3,3 V, zasilanie 3,3 V |
| I2C SDA | OLED SSD1306, BH1750 | GPIO21 | oba na 3,3 V |
| I2C SCL | OLED SSD1306, BH1750 | GPIO22 | |
| ADC1 | TDS SEN0244 | GPIO32 | zasilanie 3,3 V, wyjście 0–2,3 V |
| ADC1 | zmętnienie SEN0189 | GPIO33 | zasilanie 5 V, **dzielnik 10k/10k + 100 nF** |
| cyfrowy | HX710B DOUT (ciśnienie) | GPIO34 | moduł zasilany **3,3 V** |
| cyfrowy | HX710B SCK | GPIO26 | |
| cyfrowy | przekaźnik IN | GPIO25 | przekaźnik VCC z 5 V |

Zasilanie: zasilacz 5 V / 2 A do pinu 5V/VIN ESP32; przekaźnik i czujnik zmętnienia z szyny 5 V; reszta z 3V3.

## Procedura pierwszego uruchomienia 

NIE PODLACZAC GRZALKI WCZESNIEJ

**Etap 0: przygotowanie.** `secrets.h` utworzony. Zbuduj i wgraj środowisko `bringup` (strzałka → Upload).
Otwórz Monitor portu (115200). Oczekiwane: nagłówek „FIRMWARE DIAGNOSTYCZNY”, lista komend, skan 

**Etap 1: temperatura.** Podłącz DS18B20 z rezystorem 4,7 kΩ. Wpisz `scan`: powinien być 1 czujnik i adres ROM.
Odczyt `T = ...` ma być zbliżony do temperatury pokojowej i rosnąć po ogrzaniu sondy palcami.
MOZLIWY BLAD: Brak czujnika: sprawdź rezystor, przewody i zasilanie.

**Etap 2: I2C.** Podłącz OLED i BH1750, wpisz `scan`: oczekiwane 0x3C (OLED) i 0x23 (BH1750).
Jeśli OLED ma 0x3D lub BH1750 0x5C, popraw `OLED_I2C_ADDR`/`BH1750_I2C_ADDR` w `config.h`.

**Etap 3: przekaźnik (BEZ 230 V).** Podłącz VCC (5 V), GND i IN (GPIO25). Wpisz `relay on` i `relay off`.
- Sprawdź, czy `relay on` załącza (klik + dioda).
Jeśli załącza się przy `relay off`, ustaw `RELAY_ACTIVE_HIGH = false` w `config.h`.
- **KRYTYCZNE:** po `relay off` przekaźnik musi być **wyraźnie wyłączony**. Jeśli zostaje włączony (klik się nie cofa, dioda świeci), moduł nie nadaje się do sterowania 3,3 V. **Wymień moduł**, bo grzałka mogłaby grzać bez końca.
- Naciśnij przycisk EN kilka razy i obserwuj, czy przekaźnik nie „klika” przy starcie. Jeśli tak, dodaj rezystor 10 kΩ podciągający IN do poziomu „wyłączony”.

**Etap 4: czujniki analogowe.** Podłącz TDS, zmętnienie (z dzielnikiem!) i ciśnienie (HX710B, 3,3 V). W `stream` obserwuj odczyty.
Zanurz TDS w wodzie destylowanej (bliskie 0 ppm) i w kranówce (zwykle kilkaset ppm). Ruch przy pompce zmienia `raw` ciśnienia.

**Etap 5: kalibracja** (wartości wpisujesz w `config.h`, potem przebuduj).
- **TDS:** zanurz sondę w roztworze wzorcowym o znanym TDS (albo w wodzie zmierzonej wiarygodnym miernikiem), wpisz `cal tds <ppm>`. Przepisz `TDS_K_VALUE`.
- **Zmętnienie:** czujnik w czystej wodzie, `cal clear`. Przepisz `TURBIDITY_V_CLEAR`.
- **Ciśnienie zero:** pompka wyłączona, wąż otwarty, `cal zero`. Przepisz `PRESSURE_RAW_ZERO` (sekcja `#else`, czyli sprzętowa).
- **Ciśnienie czułość:** zanurz końcówkę węża na znaną głębokość (30 cm ≈ 2,94 kPa), `cal press 2.94`. Przelicz zgodnie z komunikatem i wpisz `PRESSURE_RAW_PER_KPA`.
- **Progi napowietrzania:** uruchom pompkę, odczytaj typowe kPa i ustaw `AERATION_MIN_KPA` / `AERATION_MAX_KPA`.

**Etap 6: finalny firmware.** Wgraj środowisko `esp32dev`. Adres IP widać w logu oraz na stronie „WI-FI” na OLED (zob. też `http://akwarium.local`). Otwórz stronę w przeglądarce w tej samej sieci.
Sprawdź zmianę temperatury zadanej z przeglądarki oraz to, że po restarcie wartość jest zachowana (NVS).

**Etap 7: grzałka.** Najpierw woda w wiadrze, osobny termometr do porównania, grzałka z własnym termostatem ustawionym na maksimum.
Sprawdź załączanie i wyłączanie wg histerezy.


## Testy

Zob. `selftest` (logika) oraz scenariusze w symulatorze (temperatura, alarmy, watchdog). Podgląd strony: `tools/ui_preview.html`.

## Znane ograniczenia

- Brak uwierzytelniania na stronie WWW: każdy w sieci lokalnej może zmienić temperaturę zadaną (15–32 °C). Nie wystawiaj urządzenia do internetu.
- Progi alarmów to wartości domyślne DO USTAWIENIA. Zakładamy akwarium słodkowodne i napowietrzanie działające bez przerwy.
- Czcionka OLED nie ma polskich znaków.
- Zabezpieczenie temperaturowe jest programowe. 
