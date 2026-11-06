#pragma once
#include <stdint.h>

//  PRZYPISANIE PINÓW (Hardware Mapping)
// Magistrala 1-Wire
constexpr int PIN_ONE_WIRE = 4;        // Czujnik temperatury DS18B20

// Magistrala I2C
constexpr int PIN_I2C_SDA = 21;        // Wyświetlacz OLED i czujnik światła BH1750
constexpr int PIN_I2C_SCL = 22;        

// Piny analogowe ADC1 
constexpr int PIN_ADC_TDS = 32;        // Czujnik TDS
constexpr int PIN_ADC_TURBIDITY = 33;  // Czujnik zmętnienia
// Czujnik ciśnienia MPS20N0040D przez moduł HX710B (cyfrowy, 2 piny):
constexpr int PIN_PRESSURE_DATA = 34;  // DOUT z HX710B (wejście; GPIO34 jest input-only). W symulacji: potencjometr (ADC)
constexpr int PIN_HX_SCK = 26;         // SCK do HX710B (wyjście; nieużywane w symulacji)

// Wyjścia cyfrowe
constexpr int PIN_RELAY_HEATER = 25;   // Sterowanie przekaźnikiem grzałki (10A)

// PARAMETRY ALGORYTMÓW 
constexpr int FILTER_SAMPLES = 10;     // Rozmiar bufora dla średniej kroczącej TUUU MOZNA ZMIENIAC

constexpr float DEFAULT_TEMP_SETPOINT = 25.0; // Domyślna temperatura w °C TU TEZ MOZNA ZMIENIAC
constexpr float TEMP_HYSTERESIS = 0.5; // Histereza tak chyba okej ale TU TEZ MOZNA


// true  = stan HIGH na pinie WŁĄCZA grzałkę
// false = stan LOW na pinie WŁĄCZA grzałkę (większość tanich modułów przekaźnikowych)
constexpr bool RELAY_ACTIVE_HIGH = true;


//     PARAMETRY CZUJNIKÓW (WARTOŚCI STARTOWE - DO KALIBRACJI NA SPRZĘCIE) 
// TDS (SEN0244): współczynnik korekcji po kalibracji roztworem wzorcowym (1.0 = brak korekty)
constexpr float TDS_K_VALUE = 1.0f;

// Zmętnienie (SEN0189): zasilany 5 V, wyjście do ~4.5 V -> dzielnik 2x10k (1:1) przed ADC
constexpr float TURBIDITY_DIVIDER = 2.0f;   // napięcie czujnika = napięcie na ADC * 2
constexpr float TURBIDITY_V_CLEAR = 4.1f;   // napięcie czujnika w CZYSTEJ wodzie [V] (do kalibracji)

// HX710B: liczba dodatkowych impulsów SCK po 24 bitach danych
// 1 = wejście różnicowe 10 Hz (HX711 kanał A, gain 128 też = 1)
constexpr int HX_EXTRA_PULSES = 1;

// Ciśnienie: przeliczenie surowego odczytu na kPa:  kPa = (raw - ZERO) / PER_KPA
#ifdef SIMULATION
constexpr float PRESSURE_RAW_ZERO    = 0.0f;
constexpr float PRESSURE_RAW_PER_KPA = 100.0f;   // potencjometr 0..4095 -> 0..~41 kPa
#else
constexpr float PRESSURE_RAW_ZERO    = 0.0f;     // DO KALIBRACJI (odczyt przy pompce wyłączonej)
constexpr float PRESSURE_RAW_PER_KPA = 1.0f;     // DO KALIBRACJI (wymaga manometru lub słupa wody)
#endif


// --- I2C: WYŚWIETLACZ I CZUJNIK ŚWIATŁA ---
constexpr uint8_t  OLED_I2C_ADDR   = 0x3C;     // typowy adres SSD1306 (czasem 0x3D - sprawdzić na module)
constexpr uint8_t  BH1750_I2C_ADDR = 0x23;     // ADDR do GND = 0x23, do VCC = 0x5C
constexpr uint32_t I2C_CLOCK_HZ    = 400000;   // MOZNA ZMIENIC 400 kHz skraca odświeżanie OLED (mniej blokowania pętli)
constexpr uint32_t DISPLAY_PAGE_MS = 3000;     // MOZNA ZMIENIC jak długo widoczny jest jeden parametr 

// Wokwi nie ma czujnika BH1750, więc światło zastępuje potencjometr na ADC1
// (GPIO35, input-only). Na sprzęcie ten pin nie jest używany - światło mierzy BH1750 po I2C.
constexpr int PIN_ADC_LIGHT_SIM = 35;

//  ALARMY WARTOŚCI DOMYŚLNE - DO DOSTOSOWANIA
constexpr uint32_t ALARM_CLEAR_DELAY_MS = 5000;     // MOZNA ZMIENICalarm gaśnie dopiero, gdy przyczyna znika na tyle długo

// Temperatura względem zadanej
constexpr float TEMP_ALARM_HIGH_DELTA = 2.0f;       // alarm, gdy T > Tzad + 2 °C
constexpr float TEMP_ALARM_LOW_DELTA  = 2.0f;       // alarm, gdy T < Tzad - 2 °C

// Napowietrzanie (ciśnienie w wężyku, w kPa po kalibracji)
constexpr float AERATION_MIN_KPA = 1.0f;            // poniżej: pompka nie pracuje / wąż odłączony
constexpr float AERATION_MAX_KPA = 25.0f;           // powyżej: zatkany kamień / zagięty wąż

// Woda (czujnik SEN0244 mierzy 0-1000 ppm)
constexpr float TDS_ALARM_PPM = 600.0f;             // MOZNA ZMIENICdostosować do gatunków ryb/roślin
constexpr float TURBIDITY_ALARM_CLARITY_PCT = 60.0f;// MOZNA ZMIENICprzejrzystość poniżej = woda mętna

// Opóźnienia zadziałania alarmów [ms] (odporność na chwilowe zakłócenia)
constexpr uint32_t ALARM_TEMP_SENSOR_DELAY_MS = 5000;
constexpr uint32_t ALARM_AERATION_DELAY_MS    = 10000;
constexpr uint32_t ALARM_PRESSURE_SENSOR_DELAY_MS = 10000;
constexpr uint32_t ALARM_LIGHT_SENSOR_DELAY_MS    = 30000;
#ifdef SIMULATION
// W symulatorze opóźnienia są krótsze, żeby dało się je przetestować w minutę
constexpr uint32_t ALARM_TEMP_HIGH_DELAY_MS = 10000;
constexpr uint32_t ALARM_TEMP_LOW_DELAY_MS  = 30000;
constexpr uint32_t ALARM_TURBID_DELAY_MS    = 10000;
constexpr uint32_t ALARM_TDS_DELAY_MS       = 10000;
#else
constexpr uint32_t ALARM_TEMP_HIGH_DELAY_MS = 60UL * 1000UL;          // 1 min
constexpr uint32_t ALARM_TEMP_LOW_DELAY_MS  = 60UL * 60UL * 1000UL;   // 60 min nagrzewanie trwa
constexpr uint32_t ALARM_TURBID_DELAY_MS    = 60UL * 1000UL;
constexpr uint32_t ALARM_TDS_DELAY_MS       = 60UL * 1000UL;
#endif

// --- WATCHDOG ---
constexpr uint32_t WATCHDOG_TIMEOUT_S = 10;         // zawieszenie pętli > 10 s = restart ESP32

// --- SIEĆ ---
constexpr const char* MDNS_HOSTNAME = "akwarium";   // strona dostępna też jako http://akwarium.local (nie zawsze działa na Androidzie)
