#pragma once
// Firmware diagnostyczny do pierwszego uruchomienia sprzetu (srodowiska "bringup" i "bringup_sim").
// NIE steruje grzalka - sluzy do sprawdzenia okablowania i kalibracji czujnikow.
void bringupSetup();
void bringupLoop();
