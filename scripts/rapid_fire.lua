-- Rapid Fire Script
-- Rapidly presses R2 when held for semi-automatic weapons

script_info = {
    name = "Rapid Fire",
    description = "Rapidly toggles R2 trigger for semi-automatic weapons",
    author = "PS5 Controller Scripts",
    version = "1.0",
    parameters = {
        {
            key = "fire_rate",
            name = "Fire Rate",
            description = "Fires per second",
            type = "int",
            default = 10,
            min = 1,
            max = 30,
            step = 1
        },
        {
            key = "trigger_threshold",
            name = "Trigger Threshold",
            description = "How much R2 needs to be pressed to activate",
            type = "float",
            default = 0.8,
            min = 0.3,
            max = 1.0,
            step = 0.1
        }
    }
}

-- State
local timer = 0
local is_on = false

function init()
    print("Rapid Fire script loaded")
end

function process(input)
    local output = input

    -- Get parameters
    local fire_rate = get_param("fire_rate", 10)
    local trigger_threshold = get_param("trigger_threshold", 0.8)

    -- Check if trigger is held
    if input.right_trigger > trigger_threshold then
        timer = timer + input.dt
        local interval = 1.0 / fire_rate

        -- Toggle trigger on/off rapidly
        if timer >= interval then
            timer = timer - interval
            is_on = not is_on
        end

        -- Set trigger based on rapid fire state
        if is_on then
            output.right_trigger = 1.0
        else
            output.right_trigger = 0.0
        end
    else
        -- Reset when not holding trigger
        timer = 0
        is_on = false
    end

    return output
end
