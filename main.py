from tmc_driver.tmc_2209 import *
from pydualsense import pydualsense
import time
tmc = Tmc2209(TmcEnableControlPin(21), TmcMotionControlStepDir(16, 20), TmcComUart("/dev/ttyAMA0"))
ds = pydualsense() # open controller
ds.init() # initialize controller

tmc.set_direction_reg(False)
tmc.set_current(300)
tmc.set_interpolation(True)
tmc.set_spreadcycle(False)
tmc.set_microstepping_resolution(2)
tmc.set_internal_rsense(False)

tmc.acceleration_fullstep = 1000
tmc.max_speed_fullstep = 250

tmc.set_motor_enabled(True)

# --- Control Logic ---
try:
    print("Controller and stepper motor script running. Press Ctrl+C to exit.")
    while True:
        # Read the left joystick's Y-axis value (up and down)
        joystick_y = ds.state.RY
        print("Joystick y:", joystick_y)

        # Map the joystick value to a motor speed
        # The joystick value ranges from -128 (up) to 127 (down)
        # Let's map this to a speed range, for example, -500 to 500
        motor_speed = int((joystick_y / 127.0) * 500)
        print("Motor speed:", motor_speed)
        tmc.set_vactual(motor_speed)

        # Add a small delay to prevent overwhelming the system
        time.sleep(0.05)

except KeyboardInterrupt:
    print("Exiting...")
    # Clean up
    ds.close()
    tmc.set_vactual(0)
    tmc.set_motor_enabled(False)