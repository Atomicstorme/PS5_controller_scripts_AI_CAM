-- Anti-Recoil Script
-- Automatically compensates for weapon recoil when firing (R2 pressed)

script_info = {
    name = "Anti-Recoil",
    description = "Automatically compensates for weapon recoil when firing",
    author = "PS5 Controller Scripts",
    version = "1.1",
    parameters = {
        {
            key = "strength_ads",
            name = "ADS Strength",
            description = "Recoil compensation when aiming down sights (L2 + R2)",
            type = "float",
            default = 0.15,
            min = 0.0,
            max = 1.0,
            step = 0.01
        },
        {
            key = "strength_hipfire",
            name = "Hip-Fire Strength",
            description = "Recoil compensation when firing without ADS",
            type = "float",
            default = 0.10,
            min = 0.0,
            max = 1.0,
            step = 0.01
        },
        {
            key = "horizontal",
            name = "Horizontal",
            description = "Horizontal adjustment for weapons with sideways recoil",
            type = "float",
            default = 0.0,
            min = -1.0,
            max = 1.0,
            step = 0.01
        },
        {
            key = "delay",
            name = "Delay (sec)",
            description = "Time before compensation starts after firing",
            type = "float",
            default = 0.05,
            min = 0.0,
            max = 0.5,
            step = 0.01
        },
        {
            key = "fire_threshold",
            name = "Fire Threshold",
            description = "R2 pressure needed to activate (0.0-1.0)",
            type = "float",
            default = 0.5,
            min = 0.1,
            max = 1.0,
            step = 0.1
        },
        {
            key = "ads_threshold",
            name = "ADS Threshold",
            description = "L2 pressure to count as aiming down sights",
            type = "float",
            default = 0.3,
            min = 0.1,
            max = 1.0,
            step = 0.1
        }
    }
}

-- State
local firing_time = 0

function init()
    print("Anti-Recoil script loaded")
end

function process(input)
    local output = input

    -- Get parameters
    local strength_ads = get_param("strength_ads", 0.15)
    local strength_hipfire = get_param("strength_hipfire", 0.10)
    local horizontal = get_param("horizontal", 0.0)
    local delay = get_param("delay", 0.05)
    local fire_threshold = get_param("fire_threshold", 0.5)
    local ads_threshold = get_param("ads_threshold", 0.3)

    -- Check if firing (R2 pressed beyond threshold)
    local is_firing = input.right_trigger > fire_threshold
    -- Check if aiming down sights (L2 pressed)
    local is_ads = input.left_trigger > ads_threshold

    if is_firing then
        firing_time = firing_time + input.dt

        -- Apply compensation after delay
        if firing_time > delay then
            -- Use ADS or hip-fire strength based on L2
            local strength = is_ads and strength_ads or strength_hipfire

            -- Pull the right stick down to compensate for recoil
            output.right_y = clamp(input.right_y + strength, -1.0, 1.0)

            -- Optional horizontal compensation
            if horizontal ~= 0 then
                output.right_x = clamp(input.right_x + horizontal, -1.0, 1.0)
            end
        end
    else
        -- Reset when not firing
        firing_time = 0
    end

    return output
end

function cleanup()
    print("Anti-Recoil script unloaded")
end
