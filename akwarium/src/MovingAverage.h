#pragma once

class MovingAverage { // Klasa do obliczania średniej kroczącej z ostatnich 'windowSize' próbek. Używa bufora cyklicznego.
private:
    int windowSize;
    float* buffer;
    int index;
    int count;
    float sum;

public:
    MovingAverage(int size);
    ~MovingAverage();
    
    // Dodaje nową próbkę i natychmiast zwraca wyliczoną średnią
    float addSample(float newSample);
    
    // Resetuje bufor (przydatne np. po wykryciu skrajnego błędu odczytu)
    void reset();
};