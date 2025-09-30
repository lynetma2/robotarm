# app/controller_input/dualsense_manager.py
import time
from threading import Thread
from pydualsense import pydualsense
from queue import Queue


class DualSenseManager(Thread):
    def __init__(self, command_queue: Queue):
        super().__init__()
        self.daemon = True
        self.command_queue = command_queue
        self.ds = None
        self.app = None
        self.socketio = None
        self.motor_enabled = False

    def set_app_context(self, app, socketio):
        """Injects the Flask app and SocketIO instance for web communication."""
        self.app = app
        self.socketio = socketio

    def _emit_status(self, status: str):
        """Helper to emit status updates over WebSocket."""
        if self.socketio and self.app:
            with self.app.app_context():
                self.socketio.emit('controller_status', {'status': status})
        self.app.logger.info(f"Controller Status: {status}")

    def init_controller(self):
        try:
            self.ds = pydualsense()
            self.ds.init()
            self.ds.right_joystick_changed += self.on_right_stick
            self.ds.cross_pressed += self.on_cross_down
            # TODO register other callbacks
            self.ds.light.setColorI(0, 255, 0)
            self._emit_status('Connected')
        except Exception as e:
            self._emit_status(f'Disconnected: {e}')
            self.ds = None

    def run(self):
        self.init_controller()
        while True:
            if self.ds is None:
                self.init_controller()
            time.sleep(5)

    def on_right_stick(self, stateX, stateY):
        # ... (joystick logic to put commands on the queue)
        deadzone = 10
        velocity = 0 if abs(stateY) < deadzone else int((stateY / 127.0) * 500)
        self.command_queue.put({'action': 'set_vactual', 'velocity': velocity})

        # TODO design command scheme.

    def on_cross_down(self, state):
        if self.motor_enabled:
            self.motor_enabled = False
            self.command_queue.put({'action': 'stop'})
        else:
            self.motor_enabled = True
            self.command_queue.put({'action': 'start'})