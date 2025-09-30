import time
from threading import Thread
from queue import Queue
from tmc_driver.tmc_2209 import *

#Outside the class to be static.
def _motor_init():
    tmc = Tmc2209(TmcEnableControlPin(21), TmcMotionControlStepDir(16, 20), TmcComUart("/dev/ttyAMA0"),
                  loglevel=Loglevel.DEBUG)
    tmc.tmc_logger.loglevel = Loglevel.DEBUG
    tmc.movement_abs_rel = MovementAbsRel.ABSOLUTE
    tmc.set_direction_reg(False)
    tmc.set_current(300)
    tmc.set_interpolation(True)
    tmc.set_spreadcycle(False)
    tmc.set_microstepping_resolution(2)
    tmc.set_internal_rsense(False)
    tmc.acceleration_fullstep = 1000
    tmc.max_speed_fullstep = 250
    return tmc

class MotorManager(Thread):

    def __init__(self):
        super().__init__()
        self.daemon = True  # Allows main program to exit even if this thread is running
        self.app = None
        self.socketio = None
        self.command_queue = Queue()
        self.tmc = _motor_init()

    def set_app_context(self, app, socketio):
        self.app = app
        self.socketio = socketio

    def run(self):
        """The main loop for the motor control thread."""
        print("Motor thread started.")
        while not self.app:
            print("MotorManager waiting for app context")
            time.sleep(1)

        with self.app.app_context():
            self.app.logger.info("MotorManager thread started.")
            while True:
                # 1. Process commands from the queue
                if not self.command_queue.empty():
                    command = self.command_queue.get()
                    self.app.logger.info(f"Executing command: {command}")
                    self.command_handler(command)

                # 2. Get and publish diagnostics (state)
                current_state = {'microstep_counter_in_steps': self.tmc.get_microstep_counter_in_steps(),'current_pos': self.tmc.current_pos}
                self.socketio.emit('diagnostics_update', current_state)

                time.sleep(0.05)  # Loop delay

    def vactual(self, speed):
        self.tmc.set_vactual(speed)

    def stop(self):
        self.tmc.set_vactual(0)
        self.tmc.set_motor_enabled(False)

    def start(self):
        self.tmc.set_motor_enabled(True)

    def command_handler(self, command):
        try:
            action = command['action']

            if action == 'stop':
                self.stop()
            elif action == 'set_vactual':
                self.vactual(command['velocity'])
            elif action == 'start':
                self.start()
            else:
                self.app.logger.warning(f"Unknown command action: {action}")

        except Exception as e:
            self.app.logger.error(f"Error processing command {command}: {e}")