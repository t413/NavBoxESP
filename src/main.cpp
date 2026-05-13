#include <Arduino.h>
#include <M5Unified.h>

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);
}

void loop() {
    M5.update();
}
