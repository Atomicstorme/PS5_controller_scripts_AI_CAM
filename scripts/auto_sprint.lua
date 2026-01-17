-- Auto-Sprint Script
-- Automatically presses L3 to sprint when pushing the left stick forward

script_info = {
    name = "Auto-Sprint",
    description = "Automatically sprints when pushing the left stick forward",
    author = "PS5 Controller Scripts",
    version = "1.0",
    parameters = {
        {
            key = "threshold",
            name = "Forward Threshold",
            description = "How far forward to push before auto-sprint activates",
            type = "float",
            default = 0.8,
            min = 0.5,
            max = 1.0,
            step = 0.05
        },
        {
            key = "hold_time",
            name = "Hold Time (sec)",
            description = "How long stick must be forward before sprinting",
            type = "float",
            default = 0.1,
            min = 0.0,
            max = 0.5,
            step = 0.05
        },
        {
            key = "cooldown",
            name = "Cooldown (sec)",
            description = "Cooldown between sprint activations",
            type = "float",
            default = 0.5,
            min = 0.1,
            max = 2.0,
            step = 0.1
        }
    }
}

-- State
local forward_time = 0
local last_sprint = 0
local total_time = 0

function init()
    print("Auto-Sprint script loaded")
end

function process(input)
    local output = input
    total_time = total_time + input.dt

    -- Get parameters
    local threshold = get_param("threshold", 0.8)
    local hold_time = get_param("hold_time", 0.1)
    local cooldown = get_param("cooldown", 0.5)

    -- Check if pushing forward (negative Y is forward on most games)
    local pushing_forward = input.left_y < -threshold

    if pushing_forward then
        forward_time = forward_time + input.dt

        -- Auto-sprint after holding forward
        if forward_time > hold_time and (total_time - last_sprint) > cooldown then
            output.l3 = true
            last_sprint = total_time
        end
    else
        forward_time = 0
    end

    return output
end
