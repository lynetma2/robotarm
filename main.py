from tmc_driver.tmc_2209 import *
tmc = Tmc2209(TmcEnableControlPin(21), TmcMotionControlStepDir(16, 20), TmcComUart("/dev/serial0"))

tmc.set_direction_reg(False)
tmc.set_current(300)
tmc.set_interpolation(True)
tmc.set_spreadcycle(False)
tmc.set_microstepping_resolution(2)
tmc.set_internal_rsense(False)

tmc.acceleration_fullstep = 1000
tmc.max_speed_fullstep = 250

tmc.set_motor_enabled(True)

tmc.run_to_position_steps(400)
tmc.run_to_position_steps(0)

tmc.set_motor_enabled(False)