class Debug {

    //Draws the bounds/hitbox of an entity
    static drawBounds(item) {
        ctx.fillStyle = "#c86464";
        ctx.globalAlpha = 0.35;
        ctx.beginPath();
        ctx.moveTo(item.bounds.points[item.bounds.points.length - 1].x, item.bounds.points[item.bounds.points.length - 1].y);
        for (let i = 0; i < item.bounds.points.length; i++) {
            ctx.lineTo(item.bounds.points[i].x, item.bounds.points[i].y);
        }
        ctx.fill();
        ctx.globalAlpha = 1.0;
    }
    
    //Draws the position of an entity
    static drawPosition(item) {
        ctx.fillStyle = "#7dfa7d";
        ctx.globalAlpha = 0.75;
        ctx.beginPath();
        ctx.arc(item.position.x, item.position.y, 2.5, 0, 2 * Math.PI);
        ctx.fill();
        ctx.globalAlpha = 1.0;
    }
    
    //Draws the velocity of an entity
    static drawVelocity(item) {
        const angle = item.velocity.angle();
        ctx.translate(item.position.x, item.position.y);
        ctx.rotate(angle);
        ctx.translate(-item.position.x, -item.position.y);
        const scale_velocity = item.velocity.mag() * 10;
        ctx.strokeStyle = "#fafa64";
        ctx.lineWidth = 1.5;
        ctx.globalAlpha = 0.75;
        ctx.beginPath();
        ctx.moveTo(item.position.x, item.position.y);
        ctx.lineTo(item.position.x + scale_velocity, item.position.y);
        ctx.lineTo(item.position.x + scale_velocity - 5, item.position.y - 5);
        ctx.moveTo(item.position.x + scale_velocity, item.position.y);
        ctx.lineTo(item.position.x + scale_velocity - 5, item.position.y + 5);
        ctx.stroke();
        ctx.resetTransform();
        ctx.globalAlpha = 1.0;
    }
    
    //Draws the velocity of an entity
    static drawAcceleration(item) {
        if (!item.accelerating) {
            return;
        }
        ctx.translate(item.position.x, item.position.y);
        ctx.rotate(-item.angle);
        ctx.translate(-item.position.x, -item.position.y);
        const scale_acceleration = item.acceleration * 250;
        ctx.strokeStyle = "#a7b8fc";
        ctx.lineWidth = 1.5;
        ctx.globalAlpha = 0.5;
        ctx.beginPath();
        ctx.moveTo(item.position.x, item.position.y);
        ctx.lineTo(item.position.x + scale_acceleration, item.position.y);
        ctx.lineTo(item.position.x + scale_acceleration - 5, item.position.y - 5);
        ctx.moveTo(item.position.x + scale_acceleration, item.position.y);
        ctx.lineTo(item.position.x + scale_acceleration - 5, item.position.y + 5);
        ctx.stroke();
        ctx.resetTransform();
        ctx.globalAlpha = 1.0;
    }

    //Draws the game data on the overlay
    static drawGameData(wave, saucers_length, asteroid_lengths, time) {
        ctx.font = "400 15px Roboto Mono";
        ctx.fillStyle = "#f2564b";
        const wave_text = "Wave: " + wave;
        const wave_size = ctx.measureText(wave_text);
        ctx.fillText(wave_text, canvas_bounds.width - wave_size.width - 10, 20);
        const saucer_text = "Saucer Count: " + saucers_length;
        const saucer_size = ctx.measureText(saucer_text);
        ctx.fillText(saucer_text, canvas_bounds.width - saucer_size.width - 10, 40);
        let asteroid_text = "Large Asteroid Count: " + asteroid_lengths[2];
        let asteroid_size = ctx.measureText(asteroid_text);
        ctx.fillText(asteroid_text, canvas_bounds.width - asteroid_size.width - 10, 60);
        asteroid_text = "Medium Asteroid Count: " + asteroid_lengths[1];
        asteroid_size = ctx.measureText(asteroid_text);
        ctx.fillText(asteroid_text, canvas_bounds.width - asteroid_size.width - 10, 80);
        asteroid_text = "Small Asteroid Count: " + asteroid_lengths[0];
        asteroid_size = ctx.measureText(asteroid_text);
        ctx.fillText(asteroid_text, canvas_bounds.width - asteroid_size.width - 10, 100);
        const fps_text = "FPS: " + fps.toFixed(2);
        const fps_size = ctx.measureText(fps_text);
        ctx.fillText(fps_text, canvas_bounds.width - fps_size.width - 10, 120);
        const time_text = "Time Elapsed: " + time.toFixed(2);
        const time_size = ctx.measureText(time_text);
        ctx.fillText(time_text, canvas_bounds.width - time_size.width - 10, 140);
    }

}

class AIDebug {

    //Draws the danger of an entity
    static drawDangerLevel(danger) {
        ctx.font = "bold 11px Roboto Mono";
        ctx.fillStyle = "#d28cf0";
        let max_danger = 0;
        for (let i = 0; i < 9; i++) {
            max_danger = Math.max(max_danger, danger.danger_levels[i]);
        }
        const text = max_danger.toFixed(2);
        const size = ctx.measureText(text);
        ctx.fillText(text, danger.position.x - size.width / 2, danger.position.y - 10);
    }

    //Draws the danger radius of an entity
    static drawDangerRadius(danger) {
        ctx.strokeStyle = "#d28cf0";
        ctx.lineWidth = 1.5;
        ctx.globalAlpha = 0.5;
        ctx.beginPath();
        ctx.arc(danger.position.x, danger.position.y, danger.size, 0, 2 * Math.PI);
        ctx.stroke();
        ctx.globalAlpha = 1.0;
    }

    //Draws the flee values (the importance of how much we have to move in a certain direction for the AI)
    static drawFleeValues(ship) {
        if (game.getTitleScreen() || game.isShipDead()) {
            return;
        }
        ctx.strokeStyle = "#d28cf0";
        ctx.lineWidth = 1.5;
        ctx.globalAlpha = 0.5;
        ctx.translate(ship.position.x, ship.position.y);
        //Draw arrows
        let scale_flee = Math.min(ai.flee_values[2] * 75, 75);
        ctx.rotate(-ship.angle);
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(scale_flee, 0);
        ctx.lineTo(scale_flee - 5, 0 - 5);
        ctx.moveTo(scale_flee, 0);
        ctx.lineTo(scale_flee - 5, 0 + 5);
        ctx.stroke();
        scale_flee = Math.min(ai.flee_values[0] * 75, 75);
        ctx.rotate(-Math.PI / 2);
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(scale_flee, 0);
        ctx.lineTo(scale_flee - 5, 0 - 5);
        ctx.moveTo(scale_flee, 0);
        ctx.lineTo(scale_flee - 5, 0 + 5);
        ctx.stroke();
        scale_flee = Math.min(ai.flee_values[3] * 75, 75);
        ctx.rotate(-Math.PI / 2);
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(scale_flee, 0);
        ctx.lineTo(scale_flee - 5, 0 - 5);
        ctx.moveTo(scale_flee, 0);
        ctx.lineTo(scale_flee - 5, 0 + 5);
        ctx.stroke();
        scale_flee = Math.min(ai.flee_values[1] * 75, 75);
        ctx.rotate(-Math.PI / 2);
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(scale_flee, 0);
        ctx.lineTo(scale_flee - 5, 0 - 5);
        ctx.moveTo(scale_flee, 0);
        ctx.lineTo(scale_flee - 5, 0 + 5);
        ctx.stroke();
        ctx.rotate(ship.angle + 3 * Math.PI / 2);
        //Draw numbers
        ctx.fillStyle = "#d28cf0";
        ctx.font = "10px Roboto Mono";
        const text_position = new Vector(Math.cos(ai.ship.angle), -Math.sin(ai.ship.angle));
        text_position.mul(27);
        text_position.rotate(Math.PI / 180 * -25, new Vector());
        let text = ai.flee_values[2].toFixed(1);
        let text_size = ctx.measureText(text);
        ctx.fillText(text, text_position.x - text_size.width / 2, text_position.y);
        text_position.rotate(Math.PI / 2, new Vector());
        text = ai.flee_values[0].toFixed(1);
        text_size = ctx.measureText(text);
        ctx.fillText(text, text_position.x - text_size.width / 2, text_position.y);
        text_position.rotate(Math.PI / 2, new Vector());
        text = ai.flee_values[3].toFixed(1);
        text_size = ctx.measureText(text);
        ctx.fillText(text, text_position.x - text_size.width / 2, text_position.y);
        text_position.rotate(Math.PI / 2, new Vector());
        text = ai.flee_values[1].toFixed(1);
        text_size = ctx.measureText(text);
        ctx.fillText(text, text_position.x - text_size.width / 2, text_position.y);
        ctx.translate(-ship.position.x, -ship.position.y);
        ctx.globalAlpha = 1.0;
    }

    //Draws the nudge values (the nudges added by different directions onto certain directions)
    static drawNudgeValues(ship) {
        if (game.getTitleScreen() || game.isShipDead()) {
            return;
        }
        ctx.strokeStyle = "#74f3f7";
        ctx.lineWidth = 1.5;
        ctx.globalAlpha = 0.5;
        ctx.translate(ship.position.x, ship.position.y);
        //Draw arrows
        let scale_flee = Math.min(ai.nudge_values[2] * 75, 75);
        ctx.rotate(-ship.angle);
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(scale_flee, 0);
        ctx.lineTo(scale_flee - 5, 0 - 5);
        ctx.moveTo(scale_flee, 0);
        ctx.lineTo(scale_flee - 5, 0 + 5);
        ctx.stroke();
        scale_flee = Math.min(ai.nudge_values[0] * 75, 75);
        ctx.rotate(-Math.PI / 2);
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(scale_flee, 0);
        ctx.lineTo(scale_flee - 5, 0 - 5);
        ctx.moveTo(scale_flee, 0);
        ctx.lineTo(scale_flee - 5, 0 + 5);
        ctx.stroke();
        ctx.rotate(-Math.PI / 2);
        scale_flee = Math.min(ai.nudge_values[1] * 75, 75);
        ctx.rotate(-Math.PI / 2);
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(scale_flee, 0);
        ctx.lineTo(scale_flee - 5, 0 - 5);
        ctx.moveTo(scale_flee, 0);
        ctx.lineTo(scale_flee - 5, 0 + 5);
        ctx.stroke();
        ctx.rotate(ship.angle + 3 * Math.PI / 2);
        //Draw numbers
        ctx.fillStyle = "#74f3f7";
        ctx.font = "10px Roboto Mono";
        const text_position = new Vector(Math.cos(ai.ship.angle), -Math.sin(ai.ship.angle));
        text_position.mul(27);
        text_position.rotate(Math.PI / 180 * 25, new Vector());
        let text = ai.nudge_values[2].toFixed(1);
        let text_size = ctx.measureText(text);
        ctx.fillText(text, text_position.x - text_size.width / 2, text_position.y);
        text_position.rotate(Math.PI / 2, new Vector());
        text = ai.nudge_values[0].toFixed(1);
        text_size = ctx.measureText(text);
        ctx.fillText(text, text_position.x - text_size.width / 2, text_position.y);
        text_position.rotate(Math.PI, new Vector());
        text = ai.nudge_values[1].toFixed(1);
        text_size = ctx.measureText(text);
        ctx.fillText(text, text_position.x - text_size.width / 2, text_position.y);
        ctx.translate(-ship.position.x, -ship.position.y);
        ctx.globalAlpha = 1.0;
    }

    //Draws the target radius of an entity
    static drawTargetRadius(target) {
        ctx.strokeStyle = "#f59445";
        ctx.lineWidth = 1.5;
        ctx.globalAlpha = 0.5;
        ctx.beginPath();
        ctx.arc(target.position.x, target.position.y, target.size, 0, 2 * Math.PI);
        ctx.stroke();
        ctx.globalAlpha = 1.0;
    }

    //Draws the minimum fire range of the ship
    static drawTargetMinDistance(ship) {
        if (game.getTitleScreen() || game.isShipDead()) {
            return;
        }
        ctx.fillStyle = "#f59445";
        ctx.globalAlpha = 0.1;
        ctx.beginPath();
        ctx.arc(ship.position.x, ship.position.y, ai.c[26], 0, 2 * Math.PI);
        ctx.fill();
        ctx.globalAlpha = 1.0;
    }

    //Draws a marker
    static drawMarker(marker) {
        ctx.font = "bold 16px Roboto Mono";
        ctx.fillStyle = "#f59445";
        const size = ctx.measureText("X");
        ctx.fillText("X", marker.position.x - size.width / 2, marker.position.y + 8);
    }

    //Draws the crosshair of the AI
    static drawCrosshair(crosshair) {
        let wrap_position = crosshair.position.copy();
        wrap(wrap_position);
        ctx.font = "20px Roboto Mono";
        ctx.fillStyle = "#f59445";
        const size = ctx.measureText("@");
        ctx.fillText("@", wrap_position.x - size.width / 2, wrap_position.y + 8);
    }

}