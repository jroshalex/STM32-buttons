# Interrupt-Based Button & HID System

This project implements a **button-controlled USB HID keyboard** with LED feedback and an idle loading animation. The firmware uses **STM32 HAL**, handling button presses via **external interrupts (EXTI)** for efficient, low-latency input.

---

## System Overview

The firmware has **two main worlds**:

1. **Interrupt (ISR) world** – triggered when a button is pressed.
2. **Main loop world** – processes HID reports, controls LEDs, and runs an idle animation.

---

## Interrupt Handler (Real-Time Processing)

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t now = HAL_GetTick();

    // Debounce: ignore presses under 120 ms apart
    if (now - lastInterruptTime < 120)
        return;

    // Map GPIO pin to button index
    if (GPIO_Pin == GPIO_PIN_2)      btnPressed = 0;
    else if (GPIO_Pin == GPIO_PIN_3) btnPressed = 1;
    else if (GPIO_Pin == GPIO_PIN_4) btnPressed = 2;
    else if (GPIO_Pin == GPIO_PIN_5) btnPressed = 3;
    else if (GPIO_Pin == GPIO_PIN_6) btnPressed = 4;

    // Record timestamp for debounce
    lastInterruptTime = now;
}
```

### Explanation

* The EXTI ISR is called **whenever a configured button pin detects a falling edge**.
* **Debouncing:** ignores interrupts occurring within 120 ms of the last valid press.
* The ISR maps the pin to a **button index** and stores it in `btnPressed`.
* Minimal processing ensures the ISR is **fast and non-blocking**.

---

## Main Loop (Processing & Animation)

```c
while (1)
{
    uint32_t now = HAL_GetTick();

    // ===== Process Button Press =====
    if (btnPressed != 255)
    {
        uint8_t id = btnPressed;      // Copy button ID
        btnPressed = 255;             // Clear flag
        lastActivityTime = now;       // Reset idle timer

        // Turn LED on
        HAL_GPIO_WritePin(GPIOA, leds[id], GPIO_PIN_SET);

        // Send HID keypress
        USBD_HID_SendReport(&hUsbDeviceFS, (uint8_t*)&shortcuts[id], sizeof(shortcuts[id]));

        HAL_Delay(40); // Hold key long enough for USB frame

        // Send HID release
        USBD_HID_SendReport(&hUsbDeviceFS, (uint8_t*)&release, sizeof(release));

        // Turn LED off
        HAL_GPIO_WritePin(GPIOA, leds[id], GPIO_PIN_RESET);
    }

    // ===== Idle Loading Animation =====
    if (now - lastActivityTime > 2000)
    {
        HAL_GPIO_WritePin(GPIOA, leds[index], GPIO_PIN_SET);
        HAL_Delay(120);
        HAL_GPIO_WritePin(GPIOA, leds[index], GPIO_PIN_RESET);

        index += direction;
        if (index == 4) direction = -1;
        if (index == 0) direction = 1;
    }
}
```

### Explanation

#### Button Press Processing

1. The main loop **checks `btnPressed`**, set by the ISR.
2. Copies the button ID to a local variable and clears the flag immediately.
3. Updates `lastActivityTime` to **pause the idle animation**.
4. Turns on the corresponding LED for feedback.
5. Sends a **USB HID keypress report** to the PC.
6. Waits 40 ms to ensure the PC detects the key press.
7. Sends a **HID release report** to indicate the key is no longer pressed.
8. Turns off the LED.

#### Idle Animation

* Runs only when **no button is pressed for 2 seconds**.
* Lights up LEDs in a “bouncing” pattern:

```
0 → 1 → 2 → 3 → 4 → 3 → 2 → 1 → 0 → repeat
```

* Uses `index` and `direction` variables to track LED position and sweep direction.
* `HAL_Delay(120)` ensures visible timing.

---

## How It Works Together

### Button Press Flow

1. Hardware detects a falling edge → EXTI ISR triggers.

2. ISR:
   * Debounces input
   * Writes `btnPressed`

3. Main loop:
   * Lights LED
   * Sends HID keypress
   * Waits 40 ms
   * Sends HID release
   * Turns LED off

4. Resets idle animation timer (`lastActivityTime`)

### Idle Animation Flow

* If **no button is pressed for 2 seconds**, LEDs sweep in a bouncing pattern.
* Animation stops when a button press occurs.
