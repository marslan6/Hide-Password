# HIDE YOUR PASSWORD

![ISA](https://img.shields.io/badge/ISA-ARM_Cortex--M4-blue)
![Language](https://img.shields.io/badge/Language-C-green)
![Platform](https://img.shields.io/badge/Platform-XMC4500-orange)
![Toolchain](https://img.shields.io/badge/Toolchain-ARM_GCC-yellow)
![License](https://img.shields.io/badge/License-Apache_2.0-brightgreen)

A hardware-based timing side-channel attack implementation for password extraction using the XMC4500 ARM Cortex-M4 microcontroller. This personal project demonstrates how keystroke timing variations in password validation systems can be exploited to extract credentials character-by-character.

The device emulates a USB HID keyboard, systematically tests each possible character, measures host response times, and identifies correct password characters based on processing latency differences.

---

## Table of Contents

- [Overview](#overview)
- [Side-Channel Attack Theory](#side-channel-attack-theory)
- [System Architecture](#system-architecture)
- [State Machine & Flow Diagrams](#state-machine--flow-diagrams)
- [Hardware Requirements](#hardware-requirements)
- [Software Dependencies](#software-dependencies)
- [Project Structure](#project-structure)
- [Build & Flash](#build--flash)
- [Testing](#testing)
- [Configuration](#configuration)
- [Technical Details](#technical-details)

---

## Overview

### What It Does

1. **Enumerates as a USB keyboard** on the target host system
2. **Sends test characters** (84 possible: a-z, A-Z, 0-9, special characters)
3. **Measures response time** for each character using Num Lock LED feedback
4. **Identifies the correct character** based on timing analysis (longest response = match)
5. **Extracts the full password** by repeating the process for each position
6. **Outputs results** via keyboard emulation

### Attack Principle

Password validation systems often exhibit timing variations:
- **Correct characters**: Additional processing (hash comparison, database lookup, history check)
- **Incorrect characters**: Early exit / fast rejection

These microsecond-to-millisecond differences are measurable via USB HID LED feedback timing, revealing password information without authentication.

---

## Side-Channel Attack Theory

### Timing Side-Channel Vulnerability

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         TIMING SIDE-CHANNEL ATTACK                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   Traditional Password Validation (Vulnerable):                             │
│                                                                             │
│   for (i = 0; i < password_length; i++) {                                   │
│       if (input[i] != stored[i]) {                                          │
│           return INVALID;  ←── Early exit leaks timing information          │
│       }                                                                     │
│   }                                                                         │
│   return VALID;                                                             │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   Timing Difference:                                                        │
│                                                                             │
│   Wrong char: ████░░░░░░░░░░░░░░░░░░░░░░  (Fast - ~2ms)                     │
│   Right char: ████████████████████████░░  (Slow - ~15ms)                    │
│                                                                             │
│   This difference reveals which character is correct!                       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Information Leakage Impact

| Attack Type | Brute Force Complexity | After Attack |
|-------------|------------------------|--------------|
| 8-char password (84 charset) | 84^8 = 2.48 × 10^15 | 84 × 8 = 672 attempts |
| 12-char password | 84^12 = 3.5 × 10^23 | 84 × 12 = 1,008 attempts |
| 20-char password | 84^20 = 1.5 × 10^38 | 84 × 20 = 1,680 attempts |

The attack reduces exponential complexity to linear.

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           SYSTEM ARCHITECTURE                               │
└─────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────┐         USB HID          ┌──────────────────────────┐
│                      │        Keyboard          │                          │
│   XMC4500 Device     │◄─────────────────────────►│     Target Host          │
│   (Attacker)         │                          │     (Victim)             │
│                      │                          │                          │
│  ┌────────────────┐  │    Keystroke Reports     │  ┌────────────────────┐  │
│  │ main.c         │  │ ─────────────────────────►│ │ Password Manager/  │  │
│  │ State Machine  │  │                          │  │ Login System       │  │
│  └───────┬────────┘  │                          │  └─────────┬──────────┘  │
│          │           │                          │            │             │
│  ┌───────▼────────┐  │     LED Status           │  ┌─────────▼──────────┐  │
│  │ Handlers.c     │  │ ◄────────────────────────│  │ Keyboard Driver    │  │
│  │ Key Events     │  │   (Num Lock Toggle)      │  │ LED Feedback       │  │
│  └───────┬────────┘  │                          │  └────────────────────┘  │
│          │           │                          │                          │
│  ┌───────▼────────┐  │                          │                          │
│  │ KeyboardHID.c  │  │                          │                          │
│  │ USB Interface  │  │                          │                          │
│  └───────┬────────┘  │                          │                          │
│          │           │                          │                          │
│  ┌───────▼────────┐  │                          │                          │
│  │ Timing Array   │  │                          │                          │
│  │ [84 entries]   │  │                          │                          │
│  └────────────────┘  │                          │                          │
│                      │                          │                          │
│  LED1 ● Num Lock     │                          │                          │
│  LED2 ● Caps Lock    │                          │                          │
│                      │                          │                          │
└──────────────────────┘                          └──────────────────────────┘
```

---

## State Machine & Flow Diagrams

### Main Execution Flow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          MAIN EXECUTION FLOW                                │
└─────────────────────────────────────────────────────────────────────────────┘

                              ┌─────────────────┐
                              │   System Init   │
                              │  - USB Setup    │
                              │  - GPIO Config  │
                              │  - Clock Setup  │
                              └────────┬────────┘
                                       │
                                       ▼
                              ┌─────────────────┐
                              │   Main Loop     │
                              │  USB_USBTask()  │
                              │  HID_Task()     │◄─────────────────────────┐
                              └────────┬────────┘                          │
                                       │                                   │
                                       ▼                                   │
                         ┌─────────────────────────────┐                   │
                         │  passwordExtractionComplete │                   │
                         │         == false ?          │                   │
                         └─────────────┬───────────────┘                   │
                                       │                                   │
                        ┌──────────────┴──────────────┐                    │
                        │ YES                         │ NO                 │
                        ▼                             ▼                    │
               ┌─────────────────┐          ┌─────────────────┐            │
               │  PHASE 1:       │          │ capsLockPhase   │            │
               │  Password       │          │ Finished?       │            │
               │  Extraction     │          └────────┬────────┘            │
               └────────┬────────┘                   │                     │
                        │                 ┌──────────┴──────────┐          │
                        │                 │ NO                  │ YES      │
                        │                 ▼                     ▼          │
                        │        ┌─────────────────┐   ┌─────────────────┐ │
                        │        │  PHASE 2:       │   │  PHASE 3:       │ │
                        │        │  Toggle         │   │  Name Output    │ │
                        │        │  Caps Lock      │   │  (ID String)    │ │
                        │        └─────────────────┘   └─────────────────┘ │
                        │                                                  │
                        └──────────────────────────────────────────────────┘
```

### Phase 1: Password Extraction State Machine

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    PHASE 1: PASSWORD EXTRACTION STATE MACHINE               │
└─────────────────────────────────────────────────────────────────────────────┘

                                    ┌──────────────┐
                                    │    START     │
                                    │ charIndex=0  │
                                    └──────┬───────┘
                                           │
                                           ▼
     ┌─────────────────────────────────────────────────────────────────────┐
     │                                                                     │
     │  ┌──────────────────────┐                                           │
     │  │   STATE: IDLE        │◄──────────────────────────────────────┐   │
     │  │   Waiting for        │                                       │   │
     │  │   readyForNextChar   │                                       │   │
     │  └──────────┬───────────┘                                       │   │
     │             │                                                   │   │
     │             │ readyForNextCharacter == true                     │   │
     │             ▼                                                   │   │
     │  ┌──────────────────────┐                                       │   │
     │  │ STATE: SEND_CHAR     │                                       │   │
     │  │                      │                                       │   │
     │  │ 1. Record start_time │                                       │   │
     │  │ 2. Send char[index]  │                                       │   │
     │  │ 3. awaitingRelease=1 │                                       │   │
     │  └──────────┬───────────┘                                       │   │
     │             │                                                   │   │
     │             ▼                                                   │   │
     │  ┌──────────────────────┐                                       │   │
     │  │ STATE: RELEASE_KEY   │                                       │   │
     │  │                      │                                       │   │
     │  │ Send empty report    │                                       │   │
     │  │ (key release)        │                                       │   │
     │  └──────────┬───────────┘                                       │   │
     │             │                                                   │   │
     │             ▼                                                   │   │
     │  ┌──────────────────────┐                                       │   │
     │  │ STATE: SEND_ENTER    │                                       │   │
     │  │                      │                                       │   │
     │  │ Send Enter key       │                                       │   │
     │  │ (trigger validation) │                                       │   │
     │  └──────────┬───────────┘                                       │   │
     │             │                                                   │   │
     │             ▼                                                   │   │
     │  ┌──────────────────────┐                                       │   │
     │  │ STATE: WAIT_RESPONSE │                                       │   │
     │  │                      │                                       │   │
     │  │ Wait for Num Lock    │                                       │   │
     │  │ LED toggle from host │                                       │   │
     │  └──────────┬───────────┘                                       │   │
     │             │                                                   │   │
     │             │ Num Lock toggled (response received)              │   │
     │             ▼                                                   │   │
     │  ┌──────────────────────┐                                       │   │
     │  │ STATE: RECORD_TIME   │                                       │   │
     │  │                      │                                       │   │
     │  │ elapsed = now -      │                                       │   │
     │  │          start_time  │                                       │   │
     │  │                      │                                       │   │
     │  │ responseTimes[index] │                                       │   │
     │  │    = elapsed         │                                       │   │
     │  │                      │                                       │   │
     │  │ index++              │                                       │   │
     │  └──────────┬───────────┘                                       │   │
     │             │                                                   │   │
     │             ▼                                                   │   │
     │  ┌──────────────────────┐                                       │   │
     │  │   index < 84 ?       │───── YES ─────────────────────────────┘   │
     │  └──────────┬───────────┘                                           │
     │             │ NO (all 84 chars tested)                              │
     │             ▼                                                       │
     │  ┌──────────────────────┐                                           │
     │  │ STATE: ANALYZE       │                                           │
     │  │                      │                                           │
     │  │ findTheChar():       │                                           │
     │  │ Find max response    │                                           │
     │  │ time index           │                                           │
     │  │                      │                                           │
     │  │ Store char in        │                                           │
     │  │ passwordBuffer       │                                           │
     │  └──────────┬───────────┘                                           │
     │             │                                                       │
     │             ▼                                                       │
     │  ┌──────────────────────┐                                           │
     │  │ Reset index = 0      │                                           │
     │  │ Move to next         │                                           │
     │  │ password position    │                                           │
     │  └──────────────────────┘                                           │
     │                                                                     │
     └─────────────────────────────────────────────────────────────────────┘
```

### Character Test Timing Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     CHARACTER TIMING MEASUREMENT                            │
└─────────────────────────────────────────────────────────────────────────────┘

    XMC4500 Device                                          Host System
         │                                                       │
         │  ──────── Test Character 'a' ────────                 │
         │                                                       │
    t0 ──┼── Send HID Report: 'a' ──────────────────────────────►│
         │                                                       │
         │                                     ┌─────────────────┤
         │                                     │ Password Check  │
         │                                     │ 'a' != 'p'     │
         │                                     │ REJECT (fast)   │
         │                                     └─────────────────┤
         │                                                       │
    t1 ──┼◄────────────── Num Lock Toggle ───────────────────────┤
         │                                                       │
         │  Δt1 = t1 - t0 = 3ms (short)                          │
         │                                                       │
         │  ──────── Test Character 'p' ────────                 │
         │                                                       │
    t2 ──┼── Send HID Report: 'p' ──────────────────────────────►│
         │                                                       │
         │                                     ┌─────────────────┤
         │                                     │ Password Check  │
         │                                     │ 'p' == 'p' ✓   │
         │                                     │ Continue check  │
         │                                     │ Hash comparison │
         │                                     │ DB lookup...    │
         │                                     └─────────────────┤
         │                                                       │
    t3 ──┼◄────────────── Num Lock Toggle ───────────────────────┤
         │                                                       │
         │  Δt2 = t3 - t2 = 12ms (LONG!) ◄── This is the         │
         │                                    correct character! │
         │                                                       │
         ▼                                                       ▼

    Analysis: max(Δt1, Δt2, ..., Δt84) → identifies correct char
```

### Complete System State Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    COMPLETE SYSTEM STATE DIAGRAM                            │
└─────────────────────────────────────────────────────────────────────────────┘

                                 ┌─────────┐
                                 │  BOOT   │
                                 └────┬────┘
                                      │
                                      ▼
                         ┌────────────────────────┐
                         │     USB_ENUMERATION    │
                         │  Wait for host to      │
                         │  recognize device      │
                         └───────────┬────────────┘
                                     │
                                     ▼
    ┌────────────────────────────────────────────────────────────────────┐
    │                                                                    │
    │                         PHASE 1: EXTRACTION                        │
    │                                                                    │
    │   ┌───────────┐    ┌───────────┐    ┌───────────┐    ┌──────────┐ │
    │   │   SEND    │───►│  RELEASE  │───►│   ENTER   │───►│  WAIT    │ │
    │   │   CHAR    │    │    KEY    │    │    KEY    │    │ RESPONSE │ │
    │   └───────────┘    └───────────┘    └───────────┘    └────┬─────┘ │
    │         ▲                                                  │       │
    │         │                                                  │       │
    │         │          ┌───────────┐                           │       │
    │         └──────────┤  RECORD   │◄──────────────────────────┘       │
    │        (idx < 84)  │   TIME    │                                   │
    │                    └─────┬─────┘                                   │
    │                          │                                         │
    │                          │ (idx == 84)                             │
    │                          ▼                                         │
    │                    ┌───────────┐                                   │
    │                    │  ANALYZE  │                                   │
    │                    │ Find max  │                                   │
    │                    │  Store    │                                   │
    │                    └─────┬─────┘                                   │
    │                          │                                         │
    │                          │ (password complete)                     │
    └──────────────────────────┼─────────────────────────────────────────┘
                               │
                               ▼
    ┌────────────────────────────────────────────────────────────────────┐
    │                                                                    │
    │                      PHASE 2: CAPS LOCK TOGGLE                     │
    │                                                                    │
    │   ┌─────────────┐         ┌─────────────┐         ┌─────────────┐ │
    │   │  CAPS_PRESS │────────►│ CAPS_RELEASE│────────►│  CAPS_DONE  │ │
    │   └─────────────┘         └─────────────┘         └──────┬──────┘ │
    │                                                          │        │
    └──────────────────────────────────────────────────────────┼────────┘
                                                               │
                                                               ▼
    ┌────────────────────────────────────────────────────────────────────┐
    │                                                                    │
    │                        PHASE 3: NAME OUTPUT                        │
    │                                                                    │
    │   ┌─────────────┐         ┌─────────────┐         ┌─────────────┐ │
    │   │  NAME_CHAR  │────────►│ NAME_RELEASE│────────►│  NAME_NEXT  │ │
    │   │   SEND      │         │             │         │   CHAR      │ │
    │   └─────────────┘         └─────────────┘         └──────┬──────┘ │
    │         ▲                                                │        │
    │         │                                                │        │
    │         └────────────────────────────────────────────────┘        │
    │                        (more chars)                               │
    │                                                                   │
    │                              │ (string complete)                  │
    │                              ▼                                    │
    │                        ┌───────────┐                              │
    │                        │   DONE    │                              │
    │                        └───────────┘                              │
    │                                                                   │
    └───────────────────────────────────────────────────────────────────┘
```

### Character Set Mapping

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         CHARACTER INDEX MAPPING                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Index Range    Characters                          Count                   │
│  ───────────    ──────────────────────────────────  ─────                   │
│   0  - 25       a b c d e f g h i j k l m n o p     26 (lowercase)          │
│                 q r s t u v w x y z                                         │
│                                                                             │
│  26  - 51       A B C D E F G H I J K L M N O P     26 (uppercase)          │
│                 Q R S T U V W X Y Z                                         │
│                                                                             │
│  52  - 61       0 1 2 3 4 5 6 7 8 9                 10 (digits)             │
│                                                                             │
│  62  - 85       ! ( ) _ + = - ~ ; : , . < > [ ]    24 (special)            │
│                 { } / ? & $ " (space)                                       │
│                                                                             │
│  ─────────────────────────────────────────────────────────────              │
│  TOTAL:         84 unique characters                                        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Hardware Requirements

### Microcontroller

| Component | Specification |
|-----------|---------------|
| **MCU** | Infineon XMC4500 |
| **Core** | ARM Cortex-M4 (32-bit) |
| **Clock** | 120 MHz |
| **Flash** | 1024 KB |
| **RAM** | 160 KB |
| **USB** | Full-speed device (12 Mbps) |
| **Debug** | SWD via J-Link |

### Pin Configuration

```
XMC4500 Microcontroller
        ┌────────────────────┐
        │                    │
   P1_1 │ ●──────► LED1      │  (Num Lock indicator)
        │                    │
   P1_0 │ ●──────► LED2      │  (Caps Lock indicator)
        │                    │
   USB  │ ◄═════► USB Port   │  (HID Keyboard interface)
        │                    │
   SWD  │ ◄═════► J-Link     │  (Debug/Programming)
        │                    │
        └────────────────────┘
```

### Required Equipment

- XMC4500 development board (or compatible)
- Segger J-Link debugger
- USB cable (device connection)
- Target host system with password authentication

---

## Software Dependencies

### Toolchain

| Tool | Version | Purpose |
|------|---------|---------|
| arm-none-eabi-gcc | 7.0+ | ARM cross-compiler |
| arm-none-eabi-gdb | 7.0+ | Debugger |
| make | 4.0+ | Build system |
| JLinkExe | 6.0+ | Flash programming |
| JLinkGDBServer | 6.0+ | GDB debug server |

### Libraries

| Library | Version | Purpose |
|---------|---------|---------|
| XMC Peripheral Library | 2.1.16 | Low-level drivers |
| LUFA | Included | USB HID stack |
| CMSIS | 4.5+ | ARM Cortex interface |
| Newlib | System | Standard C library |

### Installation (Ubuntu/Debian)

```bash
# Install ARM toolchain
sudo apt-get install gcc-arm-none-eabi gdb-arm-none-eabi

# Install J-Link tools (download from Segger website)
# https://www.segger.com/downloads/jlink/

# Set XMC library path
export XMC_LIBDIR=/opt/XMClib/XMC_Peripheral_Library_v2.1.16
```

---

## Project Structure

```
HIDE_PASSWORD/
├── main.c                  # Main program, state machine, timing logic
├── Handlers.c              # Keystroke event handlers
├── Handlers.h              # Handler function declarations
├── KeyboardHID.c           # USB HID initialization and events
├── KeyboardHID.h           # USB function declarations
├── Descriptors.c           # USB device descriptors
├── Descriptors.h           # Descriptor structure definitions
├── ReportUtils.c           # Keyboard report construction
├── ReportUtils.h           # Report utility declarations
├── IndexToChar.c           # Character index mapping (0-85)
├── IndexToChar.h           # Index mapping declarations
├── CharCodeGerman.c        # German keyboard layout conversion
├── CharCodeGerman.h        # German layout declarations
├── german_keyboardCodes.h  # German HID scan codes
├── Makefile                # Build configuration
├── student.mk              # Source file configuration
├── LICENSE.txt             # Apache 2.0 License
├── README.md               # This documentation
├── mock/                   # Test code directory
│   ├── main.c              # Mock main.c for host-based testing
│   ├── test_attack.c       # Attack simulation test harness
│   ├── test_logic.c        # Logic verification test
│   ├── KeyboardHID.h       # Mock USB HID interface
│   └── USB.h               # Mock USB definitions
├── build/                  # Compiled output
│   ├── main.elf            # Executable
│   ├── main.hex            # Flash image
│   └── main.lst            # Disassembly
└── lib_build/              # Library objects
```

### Source File Descriptions

| File | Lines | Purpose |
|------|-------|---------|
| `main.c` | ~160 | Core logic, USB callbacks, timing measurement |
| `Handlers.c` | ~136 | State machine handlers for each phase |
| `KeyboardHID.c` | ~86 | USB device initialization and events |
| `Descriptors.c` | ~221 | USB descriptor definitions |
| `ReportUtils.c` | ~24 | HID report construction helpers |
| `IndexToChar.c` | ~59 | Character set mapping |
| `CharCodeGerman.c` | ~115 | German keyboard scan codes |

### Test File Descriptions

| File | Lines | Purpose |
|------|-------|---------|
| `mock/main.c` | ~159 | Mock main with original logic for host testing |
| `mock/test_attack.c` | ~162 | Attack simulation with timing responses |
| `mock/test_logic.c` | ~95 | Logic verification of output sequences |
| `mock/KeyboardHID.h` | - | Mock USB HID interface stubs |
| `mock/USB.h` | - | Mock USB definitions |

---

## Build & Flash

### Build Commands

```bash
# Compile the project
make default

# Flash to device via J-Link
make program

# Debug with GDB
make debug

# Clean build artifacts
make clean

# Create submission package
make deliverable
```

### Build Output

```
arm-none-eabi-gcc -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mthumb ...
Building: main.c
Building: KeyboardHID.c
Building: Descriptors.c
Building: Handlers.c
Building: ReportUtils.c
Building: IndexToChar.c
Building: CharCodeGerman.c
Linking: build/main.elf
Creating: build/main.hex

   text    data     bss     dec     hex filename
  24576    1024    4096   29696    7400 build/main.elf
```

### Flash Process

```
$ make program
Connecting to J-Link...
Erasing flash...
Programming build/main.hex...
Verifying...
Done.
```

---

## Testing

The project includes a mock-based testing framework that allows running the attack logic on a host system without requiring actual XMC4500 hardware.

### Test Architecture

The `mock/` directory contains mock implementations that replace hardware-specific headers:
- **Mock headers** (`KeyboardHID.h`, `USB.h`) provide stubs for USB functions
- **Mock main.c** includes the application code with mocks enabled
- **Test harnesses** simulate the host system's timing responses

### Running Tests

```bash
# Compile and run the attack simulation test
cd mock
gcc -I. -o test_attack test_attack.c
./test_attack

# Compile and run the logic verification test
gcc -I. -o test_logic test_logic.c
./test_logic
```

### Test Descriptions

#### test_attack.c - Attack Simulation

Simulates a complete timing side-channel attack:
- Simulates a host password system with a known password (e.g., "secret")
- Feeds Num Lock LED toggles with variable delays based on character matches
- Validates whether the device successfully extracts the password character-by-character

```
Expected output:
Testing character 0: a (delay: 2ms)
Testing character 1: b (delay: 2ms)
...
Testing character 18: s (delay: 15ms) <- MATCH DETECTED
...
Password extracted: secret
TEST PASSED
```

#### test_logic.c - Logic Verification

Verifies the correctness of:
- HID keycode generation
- Character-to-index mapping
- German keyboard layout conversion
- State machine transitions

---

## Configuration

### student.mk

```makefile
# Project name
LD_NAME = Project

# Source files
SRCS = main.c KeyboardHID.c Descriptors.c Handlers.c \
       ReportUtils.c IndexToChar.c CharCodeGerman.c

# XMC library path
XMC_LIBDIR = /opt/XMClib/XMC_Peripheral_Library_v2.1.16

# Compiler flags
SCFLAGS = -std=gnu99 -O0
```

### Key Parameters (main.c)

| Parameter | Value | Description |
|-----------|-------|-------------|
| `CHARACTER_SET_SIZE` | 84 | Number of test characters |
| `MAX_PASSWORD_LENGTH` | 20 | Maximum password length |
| `SYSTICK_RATE` | 1 kHz | Timing resolution |

### USB Configuration (Descriptors.c)

| Parameter | Value |
|-----------|-------|
| Vendor ID | 0x03EB |
| Product ID | 0x2042 |
| Device Class | HID (Keyboard) |
| Endpoint | Interrupt IN |
| Report Size | 8 bytes |

---

## Technical Details

### USB HID Report Format

```
┌────────────────────────────────────────────────────────────────┐
│                    USB HID KEYBOARD REPORT                     │
├────────┬──────────┬─────────┬─────────┬─────────┬─────────────┤
│ Byte 0 │  Byte 1  │ Byte 2  │ Byte 3  │ Byte 4  │ Bytes 5-7   │
├────────┼──────────┼─────────┼─────────┼─────────┼─────────────┤
│Modifier│ Reserved │ Key 0   │ Key 1   │ Key 2   │ Keys 3-5    │
├────────┼──────────┼─────────┼─────────┼─────────┼─────────────┤
│ Shift  │   0x00   │ Scan    │ (up to 6 simultaneous keys)     │
│ Ctrl   │          │ Code    │                                 │
│ Alt    │          │         │                                 │
│ GUI    │          │         │                                 │
└────────┴──────────┴─────────┴─────────┴─────────┴─────────────┘

Modifier Bits:
  Bit 0: Left Ctrl      Bit 4: Right Ctrl
  Bit 1: Left Shift     Bit 5: Right Shift
  Bit 2: Left Alt       Bit 6: Right Alt (AltGr)
  Bit 3: Left GUI       Bit 7: Right GUI
```

### Timing Measurement

```c
// In SysTick_Handler (1ms interrupt)
void SysTick_Handler(void) {
    system_ticks++;
}

// In character send handler
uint32_t start_time = system_ticks;
SendCharacter(testingCharacterIndex, KeyboardReport);

// In response handler (Num Lock toggle received)
characterResponseTimes[testingCharacterIndex] = system_ticks - start_time;
```

### Character Analysis Algorithm

```c
uint8_t findTheChar(void) {
    uint32_t maxTime = 0;
    uint8_t maxIndex = 0;

    for (uint8_t i = 0; i < 84; i++) {
        if (characterResponseTimes[i] > maxTime) {
            maxTime = characterResponseTimes[i];
            maxIndex = i;
        }
    }

    return maxIndex;  // Character with longest response time
}
```

### Clock Configuration

```
Crystal Oscillator: 12 MHz
         │
         ▼
    ┌─────────┐
    │   PLL   │  N=80, P=2, K=4
    └────┬────┘
         │
         ▼
    System Clock: 120 MHz
         │
         ├──► CPU Clock: 120 MHz
         │
         └──► Peripheral Clock: 120 MHz

    USB PLL: 48 MHz (for USB full-speed)
```

---

## Memory Map

```
┌─────────────────────────────────────────┐  0x1000_0000
│                                         │
│              FLASH (1 MB)               │
│                                         │
│   .text   (code)                        │
│   .rodata (constants)                   │
│                                         │
├─────────────────────────────────────────┤  0x1FFF_0000
│                                         │
│              SRAM (160 KB)              │
│                                         │
│   .data   (initialized globals)         │
│   .bss    (uninitialized globals)       │
│                                         │
│   characterResponseTimes[84]   (336 B)  │
│   discoveredPasswordBuffer[20] (20 B)   │
│   nameString[]                 (42 B)   │
│                                         │
│   Stack                                 │
│                                         │
└─────────────────────────────────────────┘  0x2000_0000
```

---

## License

This project is provided as-is for research and security testing purposes.

---

## Author

**Mehmet Arslan**

---

## References

- [USB HID Specification](https://www.usb.org/hid)
- [XMC4500 Reference Manual](https://www.infineon.com/xmc4500)
- [LUFA Library Documentation](http://www.fourwalledcubicle.com/LUFA.php)
- [Timing Attacks on Implementations](https://www.rambus.com/timing-attacks/)
