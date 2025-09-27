from pydualsense import pydualsense
import time

ds = pydualsense() # open controller
ds.init() # initialize controller

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


        # Add a small delay to prevent overwhelming the system
        time.sleep(0.05)

except KeyboardInterrupt:
    print("Exiting...")
    # Clean up
    ds.close()
    # stepper.disable()