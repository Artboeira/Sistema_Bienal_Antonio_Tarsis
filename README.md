# Sistema Bienal Antonio Tarsis

ESP32-based control system for an artistic installation featuring a synchronized sequence of lifting and releasing movements.

## Project Overview

This project implements a control system using an ESP32 microcontroller to manage an artistic installation with a precise sequence of movements:

1. **Lifting Phase**: Activates a lifting mechanism via relay for 30 seconds
2. **Suspension Phase**: Maintains the object suspended for 4 seconds
3. **Release Phase**: Executes a precise stepper motor movement to release the object
4. **Lowering Phase**: Maintains the object in lowered position for 8 seconds
5. **Cycle Repetition**: Automatically repeats the entire sequence

The system uses non-blocking control techniques to ensure precise timing and smooth motor operation.

## Hardware Requirements

- ESP32 Development Board
- Stepper Motor with driver (TB6600)
- Relay Module 
- DC motor + reduction Lifting mechanism 
- 12v Power supply suitable for all components
- Jumper wires and breadboard/PCB

## Pin Configuration

### Stepper Motor (Driver)
- Pin 25: Step Pin (PUL+)
- Pin 26: Direction Pin (DIR+)
- Pin 27: Enable Pin (ENA+)

### Relay Module (ACTIVE LOW)
- Pin 21: Control for lifting mechanism
- Pin 22: Reserved for expansion

### Built-in Components
- Pin 2: Built-in LED (status indicator)

## Features

- Automated sequence execution with precise timing
- Non-blocking stepper motor control using AccelStepper library
- Precise position control with homing calibration
- Automatic cycle repetition
- Serial monitoring for status feedback
- Configurable timing parameters
- Built-in LED status indicator

## Sequence Details

1. **ICANDO**: Activates lifting mechanism for 30 seconds
2. **RETENDO_ALTO**: Maintains suspension for 4 seconds
3. **LIBERANDO**: 
   - Moves stepper motor -200° (clockwise)
   - Waits 1 second at position
   - Returns stepper motor to 0° (counter-clockwise)
4. **RETENDO_BAIXO**: Maintains lowered position for 8 seconds
5. **Cycle Repeat**: Automatically restarts the sequence

## Usage

1. Upload the code to your ESP32
2. Connect hardware according to pin configuration
3. Open Serial Monitor (115200 baud) to view status
4. System automatically begins sequence execution

## Development

This project uses VSCode + PlatformIO for development. To get started:

```bash
# Clone the repository
git clone https://github.com/Artboeira/Sistema_Bienal_Antonio_Tarsis.git

# Open in PlatformIO
cd Sistema_Bienal_Antonio_Tarsis
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio run --target monitor
```

## Dependencies

- AccelStepper library

## License

This project is open source and available under the MIT License.

## Author

Antonio Tarsis - Bienal Project