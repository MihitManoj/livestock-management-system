# Smart Livestock Health Monitoring System

An ESP32-based IoT prototype for monitoring livestock health indicators
and barn environmental conditions, with automated temperature-based
cooling.

## Overview

The system collects sensor data, processes it locally on an ESP32, and
publishes selected readings and status information to an **Adafruit IO**
dashboard over Wi-Fi.

It monitors:

-   **Cow body temperature** using an LM35 temperature sensor.
-   **Cow activity** using an MPU6050 accelerometer/gyroscope module.
-   **Milk yield** using a load cell and HX711 amplifier.
-   **Barn temperature** using a DHT11 sensor.
-   **Cooling status** using an ESP32-controlled relay connected to a
    Peltier module.

The milk-yield measurement is used locally as an additional indicator
and is **not uploaded to Adafruit IO**.

## System Architecture

``` text
LM35 (cow temperature) ─────┐
MPU6050 (activity) ─────────┤
DHT11 (barn temperature) ───┤
Load cell + HX711 (milk) ───┤
                            ▼
                          ESP32
                    ┌───────┴────────┐
                    │                │
              Local processing   Wi-Fi connection
                    │                │
             Relay → Peltier     Adafruit IO
                                     │
                                  Dashboard
```

The ESP32 performs sensor processing and local control. Adafruit IO is
used to view selected readings remotely; the cloud dashboard does not
make the cooling decision.

## Hardware Components

  Component                 Purpose
  ------------------------- -------------------------------------------------
  ESP32 development board   Main controller and Wi-Fi connectivity
  LM35                      Cow body-temperature measurement
  MPU6050                   Acceleration-based activity monitoring
  DHT11                     Barn temperature measurement
  Load cell                 Milk-yield/weight measurement for the prototype
  HX711                     Load-cell signal amplification and conversion
  5 V relay module          Switching control for the Peltier supply
  Peltier module            Demonstration of localized cooling
  External power supply     Supplies the Peltier module

## Pin Connections

  Component            ESP32 connection
  -------------------- ------------------
  LM35 analog output   GPIO 34
  MPU6050 SDA          GPIO 25
  MPU6050 SCL          GPIO 26
  DHT11 data           GPIO 13
  Relay input          GPIO 27
  HX711 DT/DOUT        GPIO 21
  HX711 SCK/CLK        GPIO 22

Use a common ground where required by the circuit. The Peltier module
must be powered from a suitable external supply; do not power it
directly from an ESP32 GPIO or 5 V board pin. Use an appropriate heat
sink and fan on the hot side of the Peltier module.

**Relay logic:** The prototype uses an active-HIGH relay configuration:
GPIO HIGH turns the relay on, and GPIO LOW turns it off. Verify the
logic of your particular relay module before powering the load.

## How It Works

1.  The ESP32 reads the LM35 analog output and converts it into a
    cow-temperature value.
2.  The MPU6050 provides acceleration readings. The current prototype
    uses threshold-based logic to classify activity as **Resting**,
    **Walking**, or **Active**.
3.  The DHT11 measures barn temperature independently of the
    cow-temperature sensor.
4.  The ESP32 evaluates the sensor readings and generates a prototype
    health status, such as **Normal**, **Fever Alert**, **Low
    Temperature**, or **Low Activity**.
5.  If the barn temperature crosses the configured threshold, the ESP32
    switches the relay and turns the Peltier cooling load on. It
    switches the relay off when the temperature falls below the
    threshold.
6.  The load cell and HX711 provide a local milk-yield/weight estimate.
    This value is not sent to Adafruit IO.

These rules are prototype decision logic, not a veterinary diagnosis.
Sensor placement, calibration, filtering, and validated animal-specific
thresholds are required before real-world use.

## IoT Monitoring with Adafruit IO

The ESP32 connects to a Wi-Fi network and uses the Adafruit IO Arduino
library to publish data to separate feeds.

  Adafruit IO feed    Data
  ------------------- ---------------------------------
  `cowtemperature`    Cow temperature
  `barntemperature`   Barn temperature
  `activity`          Resting, Walking, or Active
  `healthstatus`      Locally generated health status
  `coolingstatus`     Cooling ON or Cooling OFF

The program uses `io.connect()` to establish the Adafruit IO connection,
`io.run()` to service the connection, and each feed's `save()` method to
publish values.

To reduce unnecessary cloud requests and avoid feed throttling, numeric
temperature values can be published periodically, while activity,
health, and cooling status are published only when their values change.
The milk-yield value is kept local and has no Adafruit IO feed.

## Live IoT Dashboard

The system provides remote livestock health monitoring through an Adafruit IO dashboard.

[View Smart Livestock Monitoring Dashboard](https://io.adafruit.com/mihit_manoj/dashboards/livestock-health-monitor?kiosk=true)

The dashboard displays:
- Cow body temperature
- Barn temperature
- Cow activity
- Health status
- Peltier cooling status

The dashboard enables remote monitoring of livestock health indicators and barn environmental conditions.

## Software and Libraries

Developed for the Arduino IDE using the ESP32 board package.

Libraries used:

-   `WiFi.h`
-   `AdafruitIO_WiFi.h`
-   `Wire.h`
-   `Adafruit_MPU6050.h`
-   `Adafruit_Sensor.h`
-   `DHT.h`
-   `HX711.h`

Install the corresponding libraries through the Arduino IDE Library
Manager.

## Configuration

1.  Install the ESP32 board support package in Arduino IDE.
2.  Install the libraries listed above.
3.  Create an Adafruit IO account.
4.  Create the five feeds listed in the IoT Monitoring section.
5.  Add your Wi-Fi and Adafruit IO credentials to the program.
6.  Select the correct ESP32 board and COM port.
7.  Upload the sketch and open Serial Monitor to check sensor readings
    and connection status.
8.  Configure the Adafruit IO dashboard with blocks linked to the five
    feeds.

### Protect your credentials

Do **not** commit your Wi-Fi password or Adafruit IO key to a public Git
repository. Keep credentials in a separate, untracked secrets file or
use placeholders in the example code. If credentials have already been
committed publicly, change the exposed passwords/keys and remove them
from the repository history where appropriate.

## Calibration and Limitations

-   Calibrate the LM35 reading against a reliable thermometer and verify
    the sensor's placement and electrical range.
-   Tare and calibrate the load cell with known weights. Secure its
    mounting mechanically to reduce unstable readings.
-   The current MPU6050 activity classification is threshold-based; it
    is not an implemented machine-learning model.
-   The health-status rules are basic prototype alerts and must not be
    treated as clinical diagnosis.
-   DHT11 readings and the cooling threshold should be checked against
    the actual barn environment and application requirements.
-   A Peltier module transfers heat; effective cooling requires suitable
    heat dissipation on its hot side and an appropriately rated power
    supply.

## Future Improvements

-   Collect labelled MPU6050 data and train an activity-classification
    model.
-   Improve sensor filtering, calibration, and fault detection.
-   Add data logging and historical trend analysis.
-   Add alerts for abnormal readings and connectivity failures.
-   Validate health rules and thresholds with appropriate veterinary
    guidance.
-   Improve power management for long-term deployment.

## Project Status

This repository describes a working prototype with sensor monitoring,
local decision logic, relay-based cooling demonstration, and Adafruit IO
dashboard communication. Machine-learning activity classification and
further field validation are potential future improvements, not claims
of completed functionality.

## Disclaimer

This project is an academic prototype intended for learning and
demonstration. It is not a certified veterinary diagnostic or
animal-care system.
