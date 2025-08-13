# Sistema Bienal Antonio Tarsis

ESP32-based motor control system for managing DC and stepper motors in synchronized loop sequences.

## Project Overview

This project implements a motor control system using an ESP32 microcontroller to manage:
- **DC Motor**: For continuous rotation operations
- **Stepper Motor**: For precise positioning and controlled movements

The system operates both motors in a coordinated loop sequence with specific steps and timing.

## Hardware Requirements

- ESP32 Development Board
- DC Motor with H-Bridge driver (L298N or similar)
- Stepper Motor with driver (A4988, DRV8825, or similar)
- Power supply suitable for both motors
- Jumper wires and breadboard/PCB

## Pin Configuration

### DC Motor (H-Bridge)
- Pin 2: Motor Direction 1
- Pin 3: Motor Direction 2  
- Pin 4: Motor Enable (PWM)

### Stepper Motor
- Pin 5: Step Pin
- Pin 6: Direction Pin
- Pin 7: Enable Pin

## Features

- Dual motor control (DC + Stepper)
- Loop sequence execution
- Serial command interface
- Configurable motor parameters
- Emergency stop functionality
- Real-time motor status monitoring

## Usage

1. Upload the code to your ESP32
2. Connect motors according to pin configuration
3. Open Serial Monitor (115200 baud)
4. Send commands:
   - `START`: Begin motor sequence
   - `STOP`: Stop all motors

## Development

This project uses PlatformIO for development. To get started:

```bash
# Clone the repository
git clone https://github.com/yourusername/Sistema_Bienal_Antonio_Tarsis.git

# Open in PlatformIO
cd Sistema_Bienal_Antonio_Tarsis
pio run

# Upload to ESP32
pio run --target upload
```

## License

This project is open source and available under the MIT License.

## Author

Antonio Tarsis - Bienal Project