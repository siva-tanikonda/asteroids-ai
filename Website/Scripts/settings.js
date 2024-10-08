//Settings, including debug settings, ai settings, and game speed settings
const settings = {
    game_speed: 1,
    game_lives: 3,
    remove_particles: false,
    debug: {
        show_hitboxes: false,
        show_positions: false,
        show_velocity: false,
        show_acceleration: false,
        show_game_data: false
    },
    ai: false,
    ai_settings: {
        show_strategy: false
    }
};

//Checks if we enabled all debug settings in the previous iteration
let previous_enable_all_debug = false;
let previous_ai_enabled = false;

//Updates the settings based on what boxes are checked and what values the user enters
function updateSettings() {
    
    //Update game speed setting
    let game_speed = document.getElementById("game-speed-input").value;
    if (!isNaN(game_speed) && game_speed != 0) {
        settings.game_speed = game_speed;
    }
    
    //Update game lives setting
    let game_lives = document.getElementById("game-lives-input").value;
    if (!isNaN(game_lives) && game_lives != 0) {
        settings.game_lives = game_lives;
    }
    
    //check if we enabled all debug settings and apply necessary actions
    let enable_all_debug = document.getElementById("game-enable-all-debug-input").checked;
    document.getElementById("game-enable-all-debug-input").blur();
    if (enable_all_debug && !previous_enable_all_debug) {
        let elements = document.getElementById("debug-settings-container").children;
        for (let i = 0; i < elements.length; i++) {
            let items = elements[i].children;
            for (let j = 0; j < items.length; j++) {
                items[j].disabled = true;
                items[j].style.opacity = "0.5";
            }
        }
    } else if (!enable_all_debug && previous_enable_all_debug) {
        let elements = document.getElementById("debug-settings-container").children;
        for (let i = 0; i < elements.length; i++) {
            let items = elements[i].children;
            for (let j = 0; j < items.length; j++) {
                items[j].disabled = false;
                items[j].style.opacity = "1";
            }
        }
    }
    previous_enable_all_debug = enable_all_debug;

    //Manage debug settings if not all are enabled
    if (!enable_all_debug) {
        settings.debug.show_hitboxes = document.getElementById("game-hitbox-input").checked;
        settings.debug.show_positions = document.getElementById("game-position-input").checked;
        settings.debug.show_velocity = document.getElementById("game-velocity-input").checked;
        settings.debug.show_acceleration = document.getElementById("game-acceleration-input").checked;
        settings.debug.show_game_data = document.getElementById("game-data-input").checked;
        document.getElementById("game-hitbox-input").blur();
        document.getElementById("game-position-input").blur();
        document.getElementById("game-velocity-input").blur();
        document.getElementById("game-acceleration-input").blur();
        document.getElementById("game-data-input").blur();
    } else {
        for (let i in settings.debug) {
            settings.debug[i] = true;
        }
    }

    //Check if we have particles on or off
    settings.remove_particles = document.getElementById("game-particles-input").checked;
    document.getElementById("game-particles-input").blur();

    //Manage AI toggling
    settings.ai = document.getElementById("game-ai-input").checked;
    document.getElementById("game-ai-input").blur();
    if (settings.ai && !previous_ai_enabled) {
        document.getElementById("ai-settings-container").hidden = false;
    } else if (!settings.ai && previous_ai_enabled) {
        document.getElementById("ai-settings-container").hidden = true;
    }
    previous_ai_enabled = settings.ai;

    //Manage AI debug activation
    settings.ai_settings.show_strategy = document.getElementById("game-ai-strategy-input").checked;
    document.getElementById("game-ai-strategy-input").blur();
    if (!settings.ai) {
        settings.ai_settings.show_strategy = false;
    }

}

//Toggles the information box on or off
function toggleInfoBox() {
    let box = document.getElementById("info-box");
    let box_toggle = document.getElementById("info-button");
    box.hidden = !box.hidden;
    box_toggle.hidden = !box_toggle.hidden;
}