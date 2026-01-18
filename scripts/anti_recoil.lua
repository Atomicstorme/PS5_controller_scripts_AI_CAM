-- Anti-Recoil Script
-- Automatically compensates for weapon recoil when firing (R2 pressed)
-- Supports per-weapon presets from the Weapon Presets panel

script_info = {
    name = "Anti-Recoil",
    description = "Automatically compensates for weapon recoil when firing. Use Weapon Presets for per-gun settings.",
    author = "PS5 Controller Scripts",
    version = "1.2",
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
            key = "horizontal_strength",
            name = "Horizontal",
            description = "Horizontal adjustment for weapons with sideways recoil",
            type = "float",
            default = 0.0,
            min = -1.0,
            max = 1.0,
            step = 0.01
        },
        {
            key = "smoothing",
            name = "Smoothing",
            description = "How smoothly to apply compensation (0 = instant, 1 = very smooth)",
            type = "float",
            default = 0.5,
            min = 0.0,
            max = 1.0,
            step = 0.05
        },
        {
            key = "fire_threshold",
            name = "Fire Threshold",
            description = "R2 pressure needed to activate (0.0-1.0)",
            type = "float",
            default = 0.3,
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
local current_compensation_y = 0
local current_compensation_x = 0

function init()
    print("Anti-Recoil script loaded")
end

function process(input)
    local output = input

    -- Get parameters (these may be overridden by weapon presets)
    local strength_ads = get_param("strength_ads", 0.15)
    local strength_hipfire = get_param("strength_hipfire", 0.10)
    local horizontal = get_param("horizontal_strength", 0.0)
    local smoothing = get_param("smoothing", 0.5)
    local fire_threshold = get_param("fire_threshold", 0.3)
    local ads_threshold = get_param("ads_threshold", 0.3)

    -- Check if firing (R2 pressed beyond threshold)
    local is_firing = input.right_trigger > fire_threshold
    -- Check if aiming down sights (L2 pressed)
    local is_ads = input.left_trigger > ads_threshold

    if is_firing then
        firing_time = firing_time + input.dt

        -- Use ADS or hip-fire strength based on L2
        local target_strength = is_ads and strength_ads or strength_hipfire
        local target_horizontal = horizontal

        -- Apply smoothing (lerp towards target)
        local smooth_factor = 1.0 - (smoothing * 0.95)  -- 0.05 to 1.0 range
        current_compensation_y = current_compensation_y + (target_strength - current_compensation_y) * smooth_factor
        current_compensation_x = current_compensation_x + (target_horizontal - current_compensation_x) * smooth_factor

        -- Apply compensation
        output.right_y = clamp(input.right_y + current_compensation_y, -1.0, 1.0)

        if current_compensation_x ~= 0 then
            output.right_x = clamp(input.right_x + current_compensation_x, -1.0, 1.0)
        end
    else
        -- Smooth release when not firing
        local smooth_factor = 1.0 - (smoothing * 0.95)
        current_compensation_y = current_compensation_y * (1.0 - smooth_factor)
        current_compensation_x = current_compensation_x * (1.0 - smooth_factor)

        -- Reset after compensation decays
        if math.abs(current_compensation_y) < 0.001 then
            current_compensation_y = 0
            current_compensation_x = 0
            firing_time = 0
        end
    end

    return output
end

function cleanup()
    print("Anti-Recoil script unloaded")
end
