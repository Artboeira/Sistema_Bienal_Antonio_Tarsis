# Sistema Bienal Antonio Tarsis

ESP32-based controlled elevation system with precise stepper motor control for an artistic installation featuring synchronized lifting and controlled descent movements.

## Project Overview

This project implements a control system using an ESP32 microcontroller to manage an artistic installation with a continuous cycle of controlled movements:

1. **Lifting Phase**: Controlled stepper motor ascent for 10 seconds
2. **Top Hold Phase**: Maintains the object suspended with motor locked for 5 seconds  
3. **Descent Phase**: Controlled stepper motor descent for 12 seconds (slower for safety)
4. **Bottom Hold Phase**: Maintains the object at bottom position for 2 seconds
5. **Cycle Restart**: Automatically repeats the entire sequence

The system uses direct stepper motor control with TB6600 driver, implementing different speeds for ascent and descent phases to ensure smooth and safe operation.

## Hardware Requirements

- ESP32 Development Board
- NEMA 23 Stepper Motor
- TB6600 Stepper Motor Driver
- 24V Power Supply for motor driver
- 5V/3.3V Power Supply for ESP32
- Jumper wires and breadboard/PCB
- Pulley/lifting mechanism system

## Pin Configuration

### Stepper Motor Control (TB6600 Driver)
- Pin 25: Step Pin (STEP/PUL+) - Pulse generation
- Pin 26: Direction Pin (DIR+) - Motor direction control  
- Pin 27: Enable Pin (ENA+) - Motor enable/disable

**Important**: Connect TB6600 common pins (PUL-, DIR-, ENA-) to ESP32 GND.

## Motor Configuration

### Speed Settings
- **Ascent Speed**: 4000 microseconds between pulses
- **Descent Speed**: 6000 microseconds between pulses (slower for safety)
- **Initial Speed**: 4000 microseconds (smooth start)

### Timing Configuration
- **Ascent Time**: 10 seconds
- **Top Pause**: 5 seconds  
- **Descent Time**: 12 seconds
- **Bottom Pause**: 2 seconds

### Direction Settings
- **UP**: HIGH (configurable in code)
- **DOWN**: LOW (opposite direction)

## Features

- **Continuous Cycle Operation**: Automatic sequence repetition
- **Controlled Motor Movement**: Direct pulse generation for precise control
- **Different Ascent/Descent Speeds**: Safety-optimized speed profiles
- **Motor Locking**: Motor remains energized during pauses to maintain position
- **Serial Monitoring**: Real-time status feedback and control
- **Emergency Commands**: Safety stop and status monitoring via Serial
- **State Machine Control**: Robust state management for reliable operation

## Operating States

1. **SUBINDO** (Ascending): Motor runs upward with controlled speed
2. **PARADO_NO_TOPO** (Stopped at Top): Motor locked in position, object suspended
3. **DESCENDO** (Descending): Motor runs downward with reduced speed for safety
4. **PARADO_EM_BAIXO** (Stopped at Bottom): Motor locked, brief stabilization pause
5. **REINICIANDO** (Restarting): System preparation for new cycle

## Serial Commands

The system accepts the following commands via Serial Monitor (115200 baud):

- **`STOP`**: Emergency stop - immediately stops and locks motor (requires reset to resume)
- **`STATUS`**: Display current system status, motor state, and timing information
- **`VELOCIDADES`**: Show configured speed settings for ascent and descent

## Usage

1. Upload the code to your ESP32
2. Connect hardware according to pin configuration
3. Open Serial Monitor (115200 baud) to monitor system status
4. System automatically begins continuous cycle operation
5. Use serial commands for monitoring and emergency control

## TB6600 Driver Setup

Configure the TB6600 driver switches for optimal performance:
- Set appropriate microstep resolution
- Configure current limit according to your NEMA 23 motor specifications
- Ensure proper power supply voltage and current rating

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

## Safety Features

- **Controlled Descent Speed**: Reduced speed during descent phase for safety
- **Motor Locking**: Motor remains energized during pauses to prevent uncontrolled movement
- **Emergency Stop**: Immediate halt command via serial interface
- **State Monitoring**: Real-time system status feedback
- **Stabilization Pauses**: Brief pauses for mechanical system stabilization

## Configuration

Key parameters can be modified in the code:

```cpp
// Speed settings (microseconds between pulses)
const int VELOCIDADE_SUBIDA = 4000;      // Ascent speed
const int VELOCIDADE_DESCIDA = 6000;     // Descent speed (slower)

// Timing settings (milliseconds)  
const unsigned long TEMPO_SUBIDA = 10000;     // 10s ascending
const unsigned long TEMPO_PAUSA = 5000;       // 5s top pause
const unsigned long TEMPO_DESCIDA = 12000;    // 12s descending
```

## Dependencies

No external libraries required - uses native ESP32 Arduino framework functions for direct motor control.

## Troubleshooting

- **Motor not moving**: Check TB6600 wiring and power supply
- **Erratic movement**: Verify pulse timing and driver configuration
- **System hanging**: Use `STOP` command and reset ESP32
- **Speed issues**: Adjust `VELOCIDADE_SUBIDA` and `VELOCIDADE_DESCIDA` values

## License

This project is open source and available under the MIT License.

## Author

Antonio Tarsis - Bienal Project