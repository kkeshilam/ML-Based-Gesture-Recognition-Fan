/* Edge Impulse ingestion SDK
 * Copyright (c) 2023 EdgeImpulse Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <Gesture_Predictor_inferencing.h>
#include <LSM6DS3.h>
#include <Wire.h>


// Create IMU instance (I2C mode, address 0x6A)
LSM6DS3 myIMU(I2C_MODE, 0x6A);

// Sampling rate and buffer setup (match your Edge Impulse model)
#define EI_CLASSIFIER_FREQUENCY_HZ   62.5
#define EI_CLASSIFIER_INTERVAL_MS    (1000 / EI_CLASSIFIER_FREQUENCY_HZ)
#define TOTAL_SAMPLES                (EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE / EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME)

// Raw feature buffer
static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

// Forward declarations for IMU reading
static int read_accelerometer(float &x, float &y, float &z);
static int read_gyroscope(float &x, float &y, float &z);

void setup() {
    Serial.begin(115200);
    while (!Serial); // wait for serial monitor

    Serial.println("Edge Impulse IMU Classifier - Seeed XIAO nRF52840 Sense");

    Wire.begin();

    if (myIMU.begin() != 0) {
        Serial.println("Failed to initialize LSM6DS3!");
        while (1);
    }
    else {
        Serial.println("LSM6DS3 initialized.");
    }
}

void loop() {
    // Fill feature buffer
    for (int i = 0; i < TOTAL_SAMPLES; i++) {
        float ax, ay, az;
        float gx, gy, gz;

        read_accelerometer(ax, ay, az);
        read_gyroscope(gx, gy, gz);

        // Place in feature array (order must match EI model input)
        features[i * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME + 0] = ax;
        features[i * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME + 1] = ay;
        features[i * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME + 2] = az;
        features[i * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME + 3] = gx;
        features[i * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME + 4] = gy;
        features[i * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME + 5] = gz;

        delay(EI_CLASSIFIER_INTERVAL_MS);
    }

    // Create signal from features
    signal_t signal;
    int err = numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
    if (err != 0) {
        Serial.println("Failed to create signal from buffer");
        return;
    }

    // Run classifier
    ei_impulse_result_t result = {0};
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
    if (res != EI_IMPULSE_OK) {
        Serial.print("ERR: Failed to run classifier (");
        Serial.print(res);
        Serial.println(")");
        return;
    }

    // Print results
    Serial.println("Predictions:");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        Serial.print(result.classification[ix].label);
        Serial.print(": ");
        Serial.println(result.classification[ix].value, 5);
    }
    Serial.println();
    delay(2000);
}

// Accelerometer read
static int read_accelerometer(float &x, float &y, float &z) {
    x = myIMU.readFloatAccelX();
    y = myIMU.readFloatAccelY();
    z = myIMU.readFloatAccelZ();
    return 0;
}

// Gyroscope read
static int read_gyroscope(float &x, float &y, float &z) {
    x = myIMU.readFloatGyroX();
    y = myIMU.readFloatGyroY();
    z = myIMU.readFloatGyroZ();
    return 0;
}
