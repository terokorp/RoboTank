function servoChanged(channel, value) {
  document.getElementById("value" + channel).innerText = value;
  fetch("/servo?channel=" + channel + "&angle=" + value);
}
