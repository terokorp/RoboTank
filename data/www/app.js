
const joystick = document.getElementById("joystick");
const handle = document.getElementById("handle");

const speedElement = document.getElementById("speed");
const directionElement = document.getElementById("direction");

let dragging = false;


// --------------------------------------------------
// Update joystick
// --------------------------------------------------

function updateJoystick(clientX, clientY) {
  const rect = joystick.getBoundingClientRect();

  const centerX = rect.left + rect.width / 2;
  const centerY = rect.top + rect.height / 2;

  let x = clientX - centerX;
  let y = clientY - centerY;

  const radius = rect.width / 2;

  // Distance from center
  let distance = Math.sqrt(x * x + y * y);

  // Clamp to circle
  if (distance > radius) {
    x = x / distance * radius;
    y = y / distance * radius;

    distance = radius;
  }

  // Move handle
  handle.style.left =
    (50 + (x / radius) * 50) + "%";

  handle.style.top =
    (50 + (y / radius) * 50) + "%";


  // ----------------------------------------------
  // Speed
  // ----------------------------------------------

  const speed = Math.round(
    (distance / radius) * 100
  );


  // ----------------------------------------------
  // Direction
  //
  // Top    = 0°
  // Right  = +90°
  // Bottom = ±180°
  // Left   = -90°
  // ----------------------------------------------

  let direction =
    Math.atan2(x, -y) * 180 / Math.PI;

  // Normalize -180 ... +180
  if (direction > 180)
    direction -= 360;

  if (direction < -180)
    direction += 360;


  // Center has no meaningful direction
  if (speed === 0) {
    direction = 0;
  }


  servoChanged(speed, direction);
  speedElement.textContent = speed;
  directionElement.textContent = Math.round(direction);
}

function resetJoystick()
{
    handle.style.left = "50%";
    handle.style.top = "50%";

    servoChanged(0, 0);
    speedElement.textContent = "0";
    directionElement.textContent = "0";
}

function servoChanged(speed, direction) {
  fetch("/servo?speed=" + speed + "&direction=" + direction);
}

// --------------------------------------------------
// Pointer events
// --------------------------------------------------

joystick.addEventListener("pointerdown", function (event) {
  dragging = true;
  joystick.setPointerCapture(event.pointerId);

  updateJoystick(
    event.clientX,
    event.clientY
  );
});


joystick.addEventListener("pointermove", function (event) {
  if (!dragging)
    return;

  updateJoystick(
    event.clientX,
    event.clientY
  );
});


joystick.addEventListener("pointerup", function(event)
{
    dragging = false;
    joystick.releasePointerCapture(event.pointerId);
    resetJoystick();
});


joystick.addEventListener("pointercancel", function(event)
{
    dragging = false;
    resetJoystick();
});

