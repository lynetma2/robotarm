const stopBtn = document.getElementById('stop-btn');
const positionSpan = document.getElementById('position');

const socket = new WebSocket(`ws://${window.location.host}/ws`);

socket.onopen = () => console.log('WebSocket connected');
socket.onclose = () => console.log('WebSocket disconnected');

// Listen for diagnostics from the server
socket.onmessage = (event) => {
    const data = JSON.parse(event.data);
    if (data.position !== undefined) {
        positionSpan.innerText = data.position;
    }
};

// Send a command to the server
stopBtn.addEventListener('click', () => {
    const command = { action: 'stop' };
    socket.send(JSON.stringify(command));
});