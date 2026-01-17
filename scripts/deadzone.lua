-- Deadzone Adjustment Script
-- Adjusts stick deadzones for more or less sensitive input

script_info = {
    name = "Deadzone",
    description = "Adjusts stick deadzones for more or less sensitive input",
    author = "PS5 Controller Scripts",
    version = "1.0",
    parameters = {
        {
            key = "left_deadzone",
            name = "Left Stick Deadzone",
            description = "Deadzone for left stick (movement)",
            type = "float",
            default = 0.1,
            min = 0.0,
            max = 0.5,
            step = 0.01
        },
        {
            key = "right_deadzone",
            name = "Right Stick Deadzone",
            description = "Deadzone for right stick (aiming)",
            type = "float",
            default = 0.05,
            min = 0.0,
            max = 0.5,
            step = 0.01
        }
    }
}

function init()
    print("Deadzone script loaded")
end

function process(input)
    local output = input

    -- Get parameters
    local left_dz = get_param("left_deadzone", 0.1)
    local right_dz = get_param("right_deadzone", 0.05)

    -- Apply deadzone to left stick
    output.left_x = deadzone(input.left_x, left_dz)
    output.left_y = deadzone(input.left_y, left_dz)

    -- Apply deadzone to right stick
    output.right_x = deadzone(input.right_x, right_dz)
    output.right_y = deadzone(input.right_y, right_dz)

    return output
end
