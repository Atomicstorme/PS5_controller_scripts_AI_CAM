-- Aim Assist / Sticky Aim Script
-- Slows down aim movement when ADS (L2) is pressed for more precise aiming

script_info = {
    name = "Aim Assist",
    description = "Slows down aim movement when aiming down sights for precision",
    author = "PS5 Controller Scripts",
    version = "1.0",
    parameters = {
        {
            key = "slowdown",
            name = "Slowdown",
            description = "Aim slowdown factor when ADS (lower = slower)",
            type = "float",
            default = 0.6,
            min = 0.1,
            max = 1.0,
            step = 0.05
        },
        {
            key = "ads_threshold",
            name = "ADS Threshold",
            description = "How much L2 needs to be pressed to activate",
            type = "float",
            default = 0.3,
            min = 0.1,
            max = 0.9,
            step = 0.1
        },
        {
            key = "precision_zone",
            name = "Precision Zone",
            description = "Inner stick zone for extra slowdown",
            type = "float",
            default = 0.3,
            min = 0.0,
            max = 0.5,
            step = 0.05
        },
        {
            key = "precision_slowdown",
            name = "Precision Slowdown",
            description = "Extra slowdown in precision zone",
            type = "float",
            default = 0.4,
            min = 0.1,
            max = 1.0,
            step = 0.05
        }
    }
}

function init()
    print("Aim Assist script loaded")
end

function process(input)
    local output = input

    -- Get parameters
    local slowdown = get_param("slowdown", 0.6)
    local ads_threshold = get_param("ads_threshold", 0.3)
    local precision_zone = get_param("precision_zone", 0.3)
    local precision_slowdown = get_param("precision_slowdown", 0.4)

    -- Check if aiming down sights
    local is_ads = input.left_trigger > ads_threshold

    if is_ads then
        -- Calculate stick magnitude
        local magnitude = math.sqrt(input.right_x * input.right_x + input.right_y * input.right_y)

        -- Determine slowdown factor based on stick position
        local factor = slowdown
        if magnitude < precision_zone then
            -- Extra slowdown in precision zone
            factor = precision_slowdown
        end

        -- Apply slowdown to right stick
        output.right_x = input.right_x * factor
        output.right_y = input.right_y * factor
    end

    return output
end
