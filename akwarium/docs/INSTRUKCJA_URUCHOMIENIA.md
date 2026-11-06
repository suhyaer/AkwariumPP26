# Akwarium ESP32: kompletna instrukcja pierwszego uruchomienia

Montaż, łączenie czujników, Wi-Fi, kalibracja i test urządzenia. Wersja 1.0, październik 2026.

Idź po kolei. Każdy etap kończy się punktem **Oczekiwany wynik**. Jeśli wynik się nie zgadza, nie przechodź dalej, tylko zajrzyj do rozdziału 9.
Do etapu 7 grzałka **nie jest** podłączona do sieci 230 V. Środowiska PlatformIO: **bringup** (diagnostyka, bez sterowania grzałką, etapy 0–5)
oraz **esp32dev** (finalny firmware, etapy 6–7).

## 1. Bezpieczeństwo

- Obwód grzałki (230 V) może zabić. Przy okablowaniu wtyczka jest **wyjęta z gniazdka**
- Programowe zabezpieczenie temperatury nie wystarcza. Użyj **grzałki z własnym termostatem** (ustawionym na maksimum) oraz **bezpiecznika** po stronie 230 V (I = P / 230 V plus ok. 50% zapasu).
- Przed podłączeniem grzałki wykonaj **test wyłączania przekaźnika** (etap 3). To najważniejszy test bezpieczeństwa w projekcie.

## 2. Części i narzędzia

| Element | Uwagi |
|---|---|
| ESP32 DevKitC (WROOM-32, 38 pin) | nie wersje S2/S3/C3 (inne piny); warto mieć zapasową |
| DS18B20, sonda w stalowej obudowie | + rezystor 4,7 kΩ |
| DFRobot SEN0244 (TDS) | z sondą |
| SEN0189 (zmętnienie) | + 2 rezystory 10 kΩ i kondensator 100 nF |
| MPS20N0040D + moduł HX710B | moduł cyfrowy (piny OUT i SCK) |
| BH1750 (GY-302) | |
| OLED 0,96″ SSD1306 I2C | 4 piny |
| Moduł przekaźnika 5 V, 10 A, z optoizolacją | najlepiej opisany jako zgodny z logiką 3,3 V |
| Zasilacz 5 V / 2 A | |
| Wąż silikonowy, trójnik, zawór zwrotny | układ powietrza (5.5) |
| Przewody, płytka uniwersalna lub stykowa, zaciski śrubowe | |
| Bezpiecznik z oprawką, kabel z wtyczką i uziemieniem, obudowa | strona 230 V |
| Multimetr, termometr referencyjny, wiadro | testy |
| Miernik TDS lub roztwór wzorcowy | kalibracja TDS |
| Kabel USB **z liniami danych** | część kabli tylko ładuje |

## 3. Komputer i oprogramowanie

1. Zainstaluj **Visual Studio Code** i rozszerzenie **PlatformIO IDE**.
2. Jeśli płytka nie jest widoczna jako port COM, zainstaluj sterownik (**CP210x** lub **CH340**, zależnie od układu przy gnieździe USB).
3. Otwórz folder projektu `akwarium` (File → Open Folder).
4. Skopiuj `src/secrets.example.h` jako `src/secrets.h` i wpisz nazwę oraz hasło sieci Wi-Fi.
5. Środowisko wybierasz na dolnym pasku VS Code. Build to ✓, Upload to →, a Serial Monitor (115200) to ikona wtyczki.
6. Gdy Upload kończy się na „Connecting......”, przytrzymaj na płytce przycisk **BOOT**.
7. W Monitorze portu możesz wpisywać komendy i zatwierdzać Enterem.

## 4. Jak działa Wi-Fi i strona

Tej części nie da się przetestować w symulatorze. Pierwszy prawdziwy test strony jest dopiero na sprzęcie.

### 4.1 Zasada działania

ESP32 łączy się z routerem jako zwykłe urządzenie sieciowe i dostaje adres IP. Jednocześnie działa w nim mały serwer WWW (port 80).
Przeglądarka pobiera stronę, a ta co 2 s pyta `/api/status` o dane (JSON). Zmiana temperatury zadanej to zapytanie `POST /api/target`.
**Sterowanie grzałką działa niezależnie od sieci.**

### 4.2 Wymagania sieci

- Sieć **2,4 GHz** (ESP32 nie połączy się z siecią wyłącznie 5 GHz).
- Telefon lub komputer w **tej samej sieci** co urządzenie. Sieć gości często izoluje urządzenia, więc jej nie używaj.

### 4.3 Pierwsze połączenie

1. W `secrets.h` wpisz poprawną nazwę i hasło sieci (wielkość liter ma znaczenie).
2. Wgraj środowisko `esp32dev` i włącz Monitor portu.
3. W logu zobaczysz: `Wi-Fi: laczenie z siecia ...`, potem `Wi-Fi: polaczono, adres IP: 192.168.x.x` i `Strona urzadzenia: http://...`.
4. Adres IP zobaczysz też na OLED, na stronie **WI-FI** (strony zmieniają się co 3 s).
5. Na telefonie lub komputerze w tej samej sieci otwórz `http://` plus ten adres IP.
6. Powinna pojawić się strona: temperatura, grzałka, TDS, przejrzystość, światło, napowietrzanie i formularz temperatury zadanej.
7. Urządzenie ogłasza się też jako `http://akwarium.local` (działa na Windows 10/11 i urządzeniach Apple, na Androidzie bywa zawodne).

Adres IP można też znaleźć w panelu routera na liście urządzeń (nazwa zawiera zwykle `espressif`).

### 4.4 Komunikaty w logu

| Komunikat | Znaczenie |
|---|---|
| `Wi-Fi: laczenie z siecia ...` | start łączenia (do kilkunastu sekund) |
| `Wi-Fi: polaczono, adres IP: ...` | jest sieć, strona działa |
| `Wi-Fi: utracono polaczenie (sterowanie grzalka dziala dalej)` | router zniknął lub słaby sygnał |
| `Wi-Fi: ponawiam laczenie...` | po 15 s bez sieci próba ponowna |
| `Serwer WWW uruchomiony (port 80)` | serwer startuje razem z urządzeniem |

Urządzenie nie podaje przyczyny nieudanego łączenia. Jeśli przez minutę nie ma `polaczono`, sprawdź hasło, wielkość liter i pasmo 2,4 GHz.

### 4.5 Stały adres IP

Ustaw w routerze **rezerwację adresu DHCP** dla urządzenia (po adresie MAC z listy urządzeń). Wtedy adres się nie zmienia po restarcie routera.

### 4.6 Utrata Wi-Fi

Brak sieci nie zatrzymuje pracy: grzałka jest sterowana, OLED działa, alarmy działają. Strona przestaje odpowiadać („Brak połączenia z urządzeniem”).
Po powrocie sieci urządzenie łączy się samo, a strona ożywa bez ręcznej interwencji.

### 4.7 Test bez strony

Wpisz w przeglądarce `http://adres/api/status`. Zobaczysz surowe dane (JSON), więc wiadomo, że serwer żyje.

### 4.8 Bezpieczeństwo

Strona nie ma hasła: każdy w sieci domowej może zmienić temperaturę zadaną (zawsze w zakresie 15–32 °C, a limit 35 °C działa niezależnie).
**Nie przekierowuj portu 80 z internetu do urządzenia.**

## 5. Podłączenie czujników

### 5.0 Zasady ogólne

- Wszystkie połączenia wykonuj przy **odłączonym zasilaniu**.
- **Wszystkie masy (GND) połącz razem**: ESP32, czujniki, przekaźnik, zasilacz.
- **Sprawdzaj opisy na płytkach** (VCC, GND, SDA, SCL, OUT). Kolejność pinów różni się między producentami, a zamiana VCC z GND niszczy elektronikę. Kolory przewodów są typowe, ale opis na module ma pierwszeństwo.
- Szyna **3,3 V** to pin 3V3 płytki, szyna **5 V** to pin 5V/VIN.
- Schemat całości: `docs/schemat_polaczen.png`.

### 5.1 DS18B20 (GPIO4)

| Przewód sondy | Dokąd |
|---|---|
| czerwony (VCC) | 3,3 V |
| czarny (GND) | GND |
| żółty lub biały (DQ) | GPIO4 |

Między DQ a 3,3 V wstaw **rezystor 4,7 kΩ**. **Sprawdź:** `scan` pokazuje 1 czujnik, a odczyt jest zbliżony do temperatury pokojowej i rośnie po ogrzaniu sondy.

### 5.2 OLED i BH1750 (I2C)

Oba urządzenia podłączasz **równolegle** do tych samych dwóch linii.

| Sygnał | Pin ESP32 | OLED | BH1750 |
|---|---|---|---|
| zasilanie | 3V3 | VCC | VCC |
| masa | GND | GND | GND |
| dane | GPIO21 | SDA | SDA |
| zegar | GPIO22 | SCL | SCL |

BH1750: ADDR wolny lub do GND (adres 0x23). Zamontuj go za przezroczystą osłoną, zwrócony ku oświetleniu, z dala od rozbryzgów.
**Sprawdź:** `scan` pokazuje 0x3C (OLED) i 0x23 (BH1750). Gdy OLED ma 0x3D, zmień `OLED_I2C_ADDR` w `config.h`.

### 5.3 TDS SEN0244 (GPIO32)

Moduł ma trzy przewody (typowo czerwony – VCC, czarny – GND, niebieski – sygnał). VCC do **3,3 V**, GND do GND, sygnał do **GPIO32**.
Sondę wpina się do modułu dwupinowym złączem. Zanurzaj tylko sondę, nie moduł. Sonda i DS18B20 mają być w tej samej wodzie (korekta temperaturowa).

### 5.4 Zmętnienie SEN0189 (GPIO33), z dzielnikiem napięcia

Czujnik jest zasilany 5 V, a jego wyjście sięga ok. 4,5 V, czyli **więcej niż ESP32 znosi (3,3 V)**. Sygnał **musi** przejść przez dzielnik:

1. Zasilanie czujnika: VCC do **5 V**, GND do GND.
2. Wyjście analogowe czujnika → rezystor 10 kΩ → **punkt środkowy** → GPIO33.
3. Z punktu środkowego rezystor 10 kΩ do GND.
4. Z punktu środkowego kondensator 100 nF do GND (blisko pinu ESP32).

Używaj wyjścia **analogowego**. Nie zanurzaj płytki adaptera ani złącza, tylko głowicę czujnika.
**Sprawdź:** w `stream` napięcie czujnika w czystej wodzie jest rzędu kilku woltów i nie przekracza ok. 4,5 V.

### 5.5 Ciśnienie MPS20N0040D + HX710B (GPIO34 i GPIO26)

| Pin modułu | Dokąd |
|---|---|
| VCC | **3,3 V** (nie 5 V: sygnał OUT przekroczyłby 3,3 V) |
| GND | GND |
| OUT (DT) | GPIO34 |
| SCK | GPIO26 |

**Układ powietrza.** Czujnik mierzy do 40 kPa i **nie wolno dopuścić, by wpłynęła do niego woda**, ani dmuchać w niego ustami.
Zalecany układ: pompka → trójnik → (odnoga 1: czujnik ciśnienia) i (odnoga 2: zawór zwrotny → kamień napowietrzający).
Zawór zwrotny chroni czujnik przed cofnięciem wody po wyłączeniu pompki. Połączenia na wężyku muszą być szczelne.
W etapie 5 sprawdź, że przy wyłączonej pompce ciśnienie spada do ok. zera.

### 5.6 Przekaźnik (GPIO25)

| Pin modułu | Dokąd |
|---|---|
| VCC | **5 V** |
| GND | GND |
| IN | GPIO25 |

Zworkę VCC/JD-VCC (jeśli moduł ją ma) zostaw w ustawieniu fabrycznym. Strona sieciowa modułu (COM, NO, NC) na razie **zostaje niepodłączona**.

### 5.7 Zasilanie

- **Etapy 0–5 (bringup):** wszystko z USB komputera (pin 5V płytki zasila przekaźnik i czujnik zmętnienia).
- **Finalnie:** zasilacz 5 V / 2 A na pin 5V/VIN.
- **Nie podłączaj naraz zasilacza i kabla USB**, chyba że dokumentacja płytki to dopuszcza.
- Restarty z komunikatem „brownout” w logu oznaczają za słabe zasilanie. Zasil układ zasilaczem 5 V i odłącz USB albo użyj zasilanego huba.

### 5.8 Obwód 230 V (dopiero w etapie 7)

Schemat jest w ramce na rysunku. Faza (L) przez **bezpiecznik** do zacisku **COM** przekaźnika, zacisk **NO** do grzałki, przewód **N** prosto do grzałki,
**PE** do grzałki i metalowej obudowy. Zaciski zamknij w obudowie z odciążeniem kabli. Nie przecinaj przewodu grzałki bez wiedzy
(lepsze są gniazdo i wtyczka w obudowie). W razie wątpliwości zleć to osobie z uprawnieniami.

## 6. Etapy uruchomienia

Wgraj środowisko **bringup**. W Monitorze zobaczysz nagłówek „FIRMWARE DIAGNOSTYCZNY”, listę komend, skan i odczyty co 1 s.
Komendy: `help`, `scan`, `stream on|off`, `relay on|off`, `cal zero`, `cal press <kPa>`, `cal tds <ppm>`, `cal clear`.

**Etap 0: sama płytka.** Tylko ESP32 przez USB. *Oczekiwany wynik:* nagłówek, lista komend, skan nic nie znajduje.

**Etap 1: temperatura.** Dołącz DS18B20 (5.1), `scan`. *Oczekiwany wynik:* 1 czujnik i adres ROM, a `T = ...` zbliżone do temperatury pokojowej.

**Etap 2: I2C.** Dołącz OLED i BH1750 (5.2), `scan`. *Oczekiwany wynik:* 0x3C i 0x23. Inne adresy popraw w `config.h`.

**Etap 3: przekaźnik (bez 230 V).** Dołącz przekaźnik (5.6).
1. `relay on`: przekaźnik ma kliknąć i zaświecić diodę. Jeśli klika przy `relay off`, a nie przy `relay on`, ustaw `RELAY_ACTIVE_HIGH = false` w `config.h`.
2. **Test krytyczny:** `relay off`. Przekaźnik musi się **wyraźnie wyłączyć** (klik, dioda gaśnie). Jeśli zostaje włączony, moduł nie nadaje się do sterowania 3,3 V. **Wymień go**, bo grzałka mogłaby grzać bez końca.
3. Multimetrem (ciągłość) sprawdź COM–NO: przy `relay on` ciągłość, przy `relay off` brak.
4. Naciśnij kilka razy przycisk **EN** i obserwuj, czy przekaźnik nie klika przy starcie. Jeśli klika, dodaj rezystor 10 kΩ podciągający IN do poziomu „wyłączony” (do 3,3 V przy module aktywnym stanem LOW, do GND przy aktywnym stanem HIGH).

**Etap 4: czujniki analogowe i ciśnienie.** Dołącz TDS, zmętnienie z dzielnikiem i HX710B (5.3–5.5), obserwuj `stream`.
*Oczekiwany wynik:* TDS w wodzie destylowanej bliski 0 ppm, w kranówce zwykle kilkaset ppm; stabilne napięcie zmętnienia; ciśnienie zmienia się po uruchomieniu pompki.
Przykładowa linia: `T = 22.50 C | TDS: 1230 mV -> 412 ppm (k=1) | zmetnienie: 3.57 V czujnika, przejrzystosc 87% | cisnienie: raw 12345 -> 5.20 kPa | swiatlo: 520 lx`.

**Etap 5: kalibracja.** Rozdział 7.

**Etap 6: finalny firmware.** Upewnij się, że `secrets.h` istnieje, wybierz **esp32dev** i wgraj.
*Oczekiwany wynik:* adres IP w logu i na OLED, działająca strona (rozdz. 4), poprawne odczyty.
Ustaw z przeglądarki temperaturę zadaną (np. 26,5), zrestartuj urządzenie (EN) i sprawdź, że wartość została zapamiętana.

**Etap 7: grzałka.** Rozdział 8.

## 7. Kalibracja

Wykonaj w firmware **bringup**. Wartości wpisujesz w `src/config.h`, potem budujesz i wgrywasz ponownie.

| Co | Jak | Co wpisać |
|---|---|---|
| TDS | sonda w roztworze wzorcowym o znanym TDS (lub w wodzie zmierzonej wiarygodnym miernikiem), po ustabilizowaniu `cal tds <ppm>` | `TDS_K_VALUE` |
| Zmętnienie | głowica w czystej wodzie bez pęcherzyków, `cal clear` | `TURBIDITY_V_CLEAR` (100% = ta woda) |
| Ciśnienie, zero | pompka **wyłączona**, wąż otwarty, `cal zero` | `PRESSURE_RAW_ZERO` (sekcja `#else`, sprzętowa) |
| Ciśnienie, czułość | końcówka węża na znanej głębokości (30 cm ≈ 2,94 kPa), `cal press 2.94`, przelicz wg komunikatu | `PRESSURE_RAW_PER_KPA` |
| Progi napowietrzania | uruchom pompkę, odczytaj typowe kPa | `AERATION_MIN_KPA`, `AERATION_MAX_KPA` |

Każda komenda `cal` zbiera próbki przez 3 s. Kalibracja ciśnienia słupem wody jest przybliżona (kilkanaście procent), co wystarcza do progów alarmu.
Progi alarmów TDS, zmętnienia i temperatury (`config.h`) to wartości domyślne do dostosowania do Twoich ryb i roślin.

## 8. Test końcowy i grzałka

1. Woda w wiadrze, obok niezależny termometr. Grzałka z własnym termostatem ustawionym na maksimum.
2. Wgrany firmware **esp32dev**, temperatura zadana np. 25 °C.
3. Obwód 230 V wykonany zgodnie z 5.8. Włącz zasilanie.
4. *Oczekiwany wynik:* grzałka włącza się poniżej 24,5 °C i wyłącza powyżej 25,5 °C (histereza ±0,5 °C). Odczyt różni się od termometru referencyjnego o ułamki stopnia.
5. Alarmy: wyciągnij wąż z pompki, po ok. 10 s pojawia się alarm braku napowietrzania (OLED, strona, log).
6. Odporność: wyłącz router. Grzałka nadal pracuje, a po włączeniu routera urządzenie łączy się samo.
7. Odłącz czujnik temperatury: grzałka musi się natychmiast wyłączyć i pojawia się alarm.
8. Zostaw układ na kilka godzin (najlepiej dobę) i sprawdź stabilność temperatury i brak restartów.

## 9. Rozwiązywanie problemów

| Objaw | Prawdopodobna przyczyna | Co zrobić |
|---|---|---|
| Upload: „Connecting......” | brak trybu programowania, kabel tylko ładujący, brak sterownika | przytrzymaj BOOT, kabel z danymi, sterownik |
| `T: BRAK ODCZYTU (-127)` | brak rezystora 4,7 kΩ, zły DQ, zasilanie | rezystor DQ–3,3 V i wszystkie 3 przewody |
| Odczyt 85,0 °C | czujnik po resecie lub zakłócenia | firmware go odrzuca; sprawdź zasilanie i przewody |
| `scan` nic nie znajduje (I2C) | zamienione SDA/SCL, brak 3,3 V | piny 21/22 i VCC/GND |
| OLED pusty | zły adres (0x3D), zamienione VCC/GND | `scan`, `OLED_I2C_ADDR` |
| Zmętnienie stale na maksimum | brak dzielnika | dzielnik 10k/10k (5.4) |
| TDS = 0 lub stale maksimum | sonda nie w wodzie, zły pin lub przewód | GPIO32 i zanurzenie sondy |
| Ciśnienie `[BRAK ODCZYTU]` | zamienione OUT/SCK, moduł na 5 V, brak zasilania | OUT→GPIO34, SCK→GPIO26, VCC 3,3 V |
| Ciśnienie przy wyłączonej pompce nie spada do zera | nieszczelność lub zawór zwrotny trzyma ciśnienie | popraw układ powietrza, powtórz `cal zero` |
| Restarty ESP32, „brownout” | za słabe zasilanie | zasilacz 5 V / 2 A, grubsze przewody, hub zasilany |
| Brak `polaczono` (Wi-Fi) | złe hasło, sieć 5 GHz, ukryta sieć | `secrets.h`, sieć 2,4 GHz |
| Strona się nie otwiera | inna sieć, izolacja gości, https | ta sama sieć, `http://`, spróbuj `http://adres/api/status` |
| Adres IP się zmienił | router przydzielił nowy | rezerwacja DHCP (4.5) |
| Przekaźnik nie wyłącza się | moduł niezgodny z logiką 3,3 V | wymień moduł |

## 10. Lista kontrolna

- [ ] Części zamówione i sprawdzone względem listy (rozdz. 2)
- [ ] `secrets.h` utworzony, Wi-Fi 2,4 GHz
- [ ] Etap 0: płytka działa, log widoczny
- [ ] Etap 1: DS18B20 widoczny, odczyt sensowny
- [ ] Etap 2: I2C: 0x3C i 0x23
- [ ] Etap 3: przekaźnik włącza i **wyraźnie wyłącza**, polaryzacja ustawiona, brak kliknięć przy resecie
- [ ] Etap 4: TDS, zmętnienie (z dzielnikiem), ciśnienie dają sensowne odczyty
- [ ] Etap 5: kalibracja wykonana i wpisana w `config.h`
- [ ] Etap 6: finalny firmware, strona działa, temperatura zadana zapamiętana po restarcie
- [ ] Rezerwacja adresu IP w routerze (zalecana)
- [ ] Etap 7: bezpiecznik, termostat w grzałce, obudowa, test w wiadrze, test kilkugodzinny
