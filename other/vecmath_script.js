
// Javascript is a huge disaster

var grid_unit_size = 160;
var center_x = 209;
var center_y = 393 - 189;
var point_radius = 6;
var grab_distance = 16;

// In number space!
var mouse_x = 0;
var mouse_y = 0;

var px, py, qx, qy, wx, wy;
px = 0.4; py = 0.4;
qx = -0.4; qy = 0.8;

var grabbed = null;

var canvas = document.getElementById("figure1");
var ctx = canvas.getContext("2d");

var grid_image = new Image();
grid_image.onload = refresh;
grid_image.src = "../figures/vecmath_figure_1_fast.svg";

function refresh() {
    // Move points
    if (grabbed == "p") {
        px = mouse_x;
        py = mouse_y;
    } else if (grabbed == "q") {
        qx = mouse_x;
        qy = mouse_y;
    }

    // Set w to p*q
    wx = px*qx - py*qy;
    wy = px*qy + py*qx;

    // Redraw
    ctx.clearRect(0, 0, canvas.width, canvas.height)

    var x = canvas.width/2 - grid_image.width/2
    var y = canvas.height/2 - grid_image.height/2
    ctx.drawImage(grid_image, x, y);

    // Please don't fuck up text rendering:
    ctx.textBaseline = "top";
    ctx.font = "16px serif";

    function draw_point(x, y, color, label) {
        var x = center_x + x*grid_unit_size;
        var y = center_y - y*grid_unit_size;

        ctx.fillStyle = color;
        ctx.beginPath();
        ctx.arc(x, y, point_radius, 0,2*Math.PI);
        ctx.fill();

        ctx.fillStyle = "#000";
        ctx.fillText(label, x + point_radius/2, y + point_radius/2);
    }

    draw_point(px, py, "#f00", "p")
    draw_point(qx, qy, "#0d3", "q")
    draw_point(wx, wy, "#0ad", "w")
}

canvas.addEventListener('mousemove', function(e) {
    var r = canvas.getBoundingClientRect();
    mouse_x = (e.clientX - r.left - center_x)/grid_unit_size;
    mouse_y = -(e.clientY - r.top - center_y)/grid_unit_size;

    if (mouse_x > 1.0)  { mouse_x = 1.0; }
    if (mouse_y > 1.0)  { mouse_y = 1.0; }
    if (mouse_x < -1.0) { mouse_x = -1.0; }
    if (mouse_y < -1.0) { mouse_y = -1.0; }

    if (grabbed != null) {
        refresh();
    }
}, false);

canvas.addEventListener('mousedown', function() {
    var distance_to_p = (mouse_x - px)*(mouse_x - px) + (mouse_y - py)*(mouse_y - py);
    var distance_to_q = (mouse_x - qx)*(mouse_x - qx) + (mouse_y - qy)*(mouse_y - qy);

    grabbed = null;
    if (Math.min(distance_to_p, distance_to_q)*grid_unit_size < grab_distance) {
        if (distance_to_p < distance_to_q) { 
            grabbed = "p"; 
        } else {
            grabbed = "q"; 
        }
    }

    if (grabbed != null) {
        refresh();
    }
});

document.addEventListener('mouseup', function() {
    grabbed = null;
});

document.body.onload = function() {
    document.getElementById("example_button_1").onclick = function() {
        px = 0.5; py = 0;
        refresh();
    };
    document.getElementById("example_button_2").onclick = function() {
        px = 0; py = -0.75;
        refresh();
    };
    document.getElementById("example_button_3").onclick = function() {
        px = -0.5; py = 1;
        refresh();
    };
};
