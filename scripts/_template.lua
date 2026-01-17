--[[
    Script Template
    Copy this file and rename it to create your own script

    Available input fields:
    - left_x, left_y: Left stick (-1.0 to 1.0)
    - right_x, right_y: Right stick (-1.0 to 1.0)
    - left_trigger, right_trigger: Triggers (0.0 to 1.0)
    - cross, circle, square, triangle: Face buttons (true/false)
    - l1, r1, l2_button, r2_button: Shoulder buttons (true/false)
    - l3, r3: Stick buttons (true/false)
    - share, options, ps, touchpad, mute: Other buttons (true/false)
    - dpad: D-pad direction (0-7 for directions, 8 = released)
           0=up, 1=up-right, 2=right, 3=down-right, 4=down, 5=down-left, 6=left, 7=up-left
    - gyro_x, gyro_y, gyro_z: Gyroscope (normalized)
    - dt: Delta time since last update (seconds)

    Available helper functions:
    - clamp(value, min, max): Clamp a value between min and max
    - lerp(a, b, t): Linear interpolation between a and b
    - deadzone(value, zone): Apply deadzone to a value
    - get_param(name, default): Get a parameter value
    - set_param(name, value): Set a parameter value
    - print(...): Print to console (for debugging)
]]

-- Script metadata (shown in UI)
script_info = {
    name = "My Script",           -- Display name in the UI
    description = "Description of what this script does",
    author = "Your Name",
    version = "1.0",
    parameters = {
        -- Example float parameter (slider)
        {
            key = "my_float",           -- Internal name for get_param()
            name = "My Float Value",    -- Display name in UI
            description = "Tooltip description",
            type = "float",
            default = 0.5,
            min = 0.0,
            max = 1.0,
            step = 0.01
        },
        -- Example int parameter (slider with whole numbers)
        {
            key = "my_int",
            name = "My Integer",
            description = "An integer value",
            type = "int",
            default = 5,
            min = 1,
            max = 10,
            step = 1
        },
        -- Example bool parameter (checkbox)
        {
            key = "my_bool",
            name = "Enable Feature",
            description = "Toggle this feature on/off",
            type = "bool",
            default = true
        },
        -- Example choice parameter (dropdown)
        -- {
        --     key = "my_choice",
        --     name = "Mode",
        --     description = "Select a mode",
        --     type = "choice",
        --     default = 0,
        --     choices = { "Mode A", "Mode B", "Mode C" }
        -- }
    }
}

-- Called once when script is loaded
function init()
    print("My script loaded!")
end

-- Called every frame with controller input
-- Return the modified input
function process(input)
    local output = input  -- Start with a copy of input

    -- Get parameter values
    local my_float = get_param("my_float", 0.5)
    local my_int = get_param("my_int", 5)
    local my_bool = get_param("my_bool", 1) > 0.5

    -- YOUR CODE HERE
    -- Example: invert Y axis if feature enabled
    -- if my_bool then
    --     output.left_y = -input.left_y
    -- end

    return output
end

-- Called when script is unloaded (optional)
function cleanup()
    print("My script unloaded")
end
