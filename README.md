# PS5 Controller Scripts

A Windows application for modifying PS5 DualSense controller input using Lua scripts. Create custom controller behaviors like anti-recoil, auto-sprint, aim assist, and more.

**For offline/single-player games only.**

![License](https://img.shields.io/badge/license-OSL%203.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)

## Features

- **DualSense Support** - Full PS5 controller input reading via USB or Bluetooth
- **Virtual Controller** - Outputs to a virtual Xbox 360 controller that games recognize
- **Lua Scripting** - Write custom scripts to modify controller input in real-time
- **Live Preview** - See input and output values side-by-side in the GUI
- **Hot Reload** - Refresh scripts without restarting the application
- **Per-Game Profiles** - Save different script configurations for each game
- **Weapon Presets** - Per-gun recoil patterns with customizable settings
- **Global Hotkeys** - Toggle scripts and switch weapons with keyboard shortcuts
- **On-Screen Overlay** - See active scripts and weapon info while gaming

## Requirements

- Windows 10/11 (64-bit)
- PS5 DualSense controller
- [ViGEmBus Driver](https://github.com/ViGEm/ViGEmBus/releases) (required for virtual controller)
- Visual Studio 2022 with C++ workload (for building)

## Building

### 1. Clone the repository

```bash
git clone https://github.com/Atomicstorme/controller_scripts.git
cd controller_scripts
```

### 2. Download dependencies

Run `setup_dependencies.bat` to create the folder structure, then download:

| Dependency | Source | Destination |
|------------|--------|-------------|
| Dear ImGui | [GitHub](https://github.com/ocornut/imgui/releases) | `libs/imgui/` |
| HIDAPI | [GitHub](https://github.com/libusb/hidapi/releases) | `libs/hidapi/` |
| ViGEmClient | [GitHub](https://github.com/nefarius/ViGEmClient) | `libs/ViGEmClient/` |
| Lua 5.4 | [lua.org](https://www.lua.org/ftp/) | `libs/lua-src/` |

See `SETUP.txt` for detailed instructions.

### 3. Build with Visual Studio

1. Open the folder in Visual Studio 2022
2. Select a kit (e.g., "Visual Studio Community 2022 Release - amd64")
3. Build -> Build All

Or via command line:
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Usage

1. Install the [ViGEmBus driver](https://github.com/ViGEm/ViGEmBus/releases) (one-time)
2. Connect your PS5 DualSense controller
3. Run `PS5ControllerScripts.exe`
4. Enable scripts by checking the checkbox next to them
5. Play your game - it will see the virtual Xbox controller with modified input

## Profiles

Create different profiles for each game with unique script settings:

1. Click the **+** button next to the profile dropdown
2. Enter a profile name (e.g., "Call of Duty", "Apex Legends")
3. Configure scripts and weapon presets for that profile
4. Switch between profiles using the dropdown

Settings are automatically saved and restored when you switch profiles.

## Weapon Presets

Create per-gun recoil patterns within each profile:

1. Expand the **Weapon Presets** section
2. Click **+** to add a new weapon (e.g., "AK-47", "M4A1")
3. Adjust settings for that weapon:
   - **ADS Strength** - Anti-recoil when aiming down sights
   - **Hip-Fire Strength** - Anti-recoil when hip-firing
   - **Horizontal** - Horizontal recoil compensation
   - **Smoothing** - Movement smoothing (0 = instant, 1 = smooth)
4. Set a **Hotkey** to quickly switch to that weapon in-game

## Hotkeys

### Script Hotkeys
Click the hotkey button next to any script (shows "..." or current key) to assign a toggle hotkey.

### Weapon Hotkeys
In the Weapon Presets section, click the hotkey button to assign a key for instant weapon switching.

### Overlay Toggle
Press **F12** to show/hide the on-screen overlay.

### Setting Hotkeys
1. Click the hotkey button
2. Press your desired key (with optional Ctrl/Alt/Shift modifiers)
3. Press **Escape** to cancel, **Delete** to clear

## On-Screen Overlay

The overlay displays real-time status while gaming:
- Controller connection status
- Active weapon and settings
- Enabled scripts
- Configured hotkeys

Configure in **File > Settings > Overlay**:
- **Show/Hide** - Toggle overlay visibility
- **Position** - Choose corner (Top-Left, Top-Right, Bottom-Left, Bottom-Right)
- **Opacity** - Adjust transparency

The overlay works over borderless/windowed games. Press **F12** to toggle.

## Included Scripts

| Script | Description |
|--------|-------------|
| `anti_recoil.lua` | Automatically compensates for weapon recoil when firing |
| `auto_sprint.lua` | Auto-sprint when pushing the left stick forward |
| `aim_assist.lua` | Slows aim movement when ADS for precision |
| `rapid_fire.lua` | Rapid fire for semi-automatic weapons |
| `deadzone.lua` | Adjustable stick deadzones |

## Writing Scripts

Create a `.lua` file in the `scripts/` folder. Scripts must have a `process` function:

```lua
function process(input)
    local output = input

    -- Example: Invert Y axis
    output.left_y = -input.left_y

    return output
end
```

### Available Input Fields

| Field | Type | Range | Description |
|-------|------|-------|-------------|
| `left_x`, `left_y` | float | -1.0 to 1.0 | Left stick |
| `right_x`, `right_y` | float | -1.0 to 1.0 | Right stick |
| `left_trigger`, `right_trigger` | float | 0.0 to 1.0 | L2/R2 triggers |
| `cross`, `circle`, `square`, `triangle` | bool | | Face buttons |
| `l1`, `r1`, `l3`, `r3` | bool | | Shoulder/stick buttons |
| `dpad` | int | 0-8 | D-pad (0=up, 2=right, 4=down, 6=left, 8=released) |
| `dt` | float | | Delta time in seconds |

### Helper Functions

```lua
clamp(value, min, max)    -- Clamp value between min and max
lerp(a, b, t)             -- Linear interpolation
deadzone(value, zone)     -- Apply deadzone to stick value
get_param(name, default)  -- Get script parameter
print(...)                -- Debug output
```

See `scripts/_template.lua` for a complete reference.

## Distribution

To distribute the application, package these files:
- `PS5ControllerScripts.exe`
- `hidapi.dll`
- `scripts/` folder

Users only need to install the ViGEmBus driver.

## License

This project is licensed under the [Open Software License 3.0](LICENSE.txt).

## Acknowledgments

- [Dear ImGui](https://github.com/ocornut/imgui) - Immediate mode GUI
- [HIDAPI](https://github.com/libusb/hidapi) - HID device communication
- [ViGEmBus](https://github.com/ViGEm/ViGEmBus) - Virtual gamepad emulation
- [Lua](https://www.lua.org/) - Scripting language

## Disclaimer

This software is intended for use with offline/single-player games only. Using input modification in online multiplayer games may violate terms of service and result in bans. Use responsibly.
