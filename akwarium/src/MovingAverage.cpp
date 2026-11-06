#include "MovingAverage.h"

MovingAverage::MovingAverage(int size) : windowSize(size < 1 ? 1 : size), index(0), count(0), sum(0.0) {
    buffer = new float[windowSize];
    for (int i = 0; i < windowSize; i++) {
        buffer[i] = 0.0;
    }
}

MovingAverage::~MovingAverage() { //zwolnienie pamieci
    delete[] buffer;
}

float MovingAverage::addSample(float newSample) {
    // Odejmujemy najstarszą wartość z sumy (jeśli bufor jest pełny)
    sum -= buffer[index];
    
    // Dodajemy nową wartość do bufora i do sumy
    buffer[index] = newSample;
    sum += newSample;
    
    // Przesuwamy indeks (cyklicznie)
    index = (index + 1) % windowSize;
    
    // Zwiększamy licznik aż do zapełnienia bufora
    if (count < windowSize) {
        count++;
    }
    
    // Zwracamy średnią
    return sum / count;
}

void MovingAverage::reset() { // Resetuje bufor i sumę
    index = 0;
    count = 0;
    sum = 0.0;
    for (int i = 0; i < windowSize; i++) {
        buffer[i] = 0.0;
    }
}