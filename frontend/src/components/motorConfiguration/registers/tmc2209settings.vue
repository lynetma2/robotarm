<script setup lang="ts">
import { ref, computed } from 'vue'
import { Button } from '@/components/ui/button'
import {
  Card,
  CardContent,
  CardDescription,
  CardFooter,
  CardHeader,
  CardTitle,
} from '@/components/ui/card'
import { Checkbox } from '@/components/ui/checkbox'
import { Label } from '@/components/ui/label'
import { Input } from '@/components/ui/input'
import { RadioGroup, RadioGroupItem } from '@/components/ui/radio-group'
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from '@/components/ui/select'
import { Separator } from '@/components/ui/separator'
import { Slider } from '@/components/ui/slider'
import { Switch } from '@/components/ui/switch'


// --- Reactive State Definitions ---

// Section 1: Currents
const runCurrent_mA = ref(800) // User input in Milliamps
const holdCurrent_mA = ref(500) // User input in Milliamps
const senseResistorOhms = ref(0.110) // CRITICAL: User must set this
const iholddelay = ref([8]) // 0-15 value

// Section 2: Driver Mode
const driverMode = ref('stealthchop')
const tpwmthrs = ref([100])

// Section 3: Microstepping
const mres = ref('0') // '0' corresponds to 1/256
const interpolate = ref(true)

// Section 4: Direction
const reverseDirection = ref(false)

// --- Calculation Logic ---

/**
 * Calculates the 5-bit register value (0-31) for IRUN or IHOLD.
 * This is based on the standard Klipper/Marlin formula for TMC2209.
 *
 * The formula for RMS current is:
 * Irms = ( (CS + 1) / 32 ) * ( Vfs / (Rsense + 0.02) ) * ( 1 / sqrt(2) )
 *
 * We solve for CS (Current Scale, 0-31), which is our register value:
 * CS = ( (Irms * sqrt(2) * (Rsense + 0.02)) / Vfs ) * 32 - 1
 *
 * Where:
 * - Vfs (full-scale voltage) is typically 0.325V
 * - (sqrt(2) * 32) / 0.325 is approx. 138.8
 *
 * @param current_mA The desired RMS current in milliamps.
 * @param r_sense The sense resistor value in Ohms.
 * @returns The calculated register value, clamped between 0 and 31.
 */
function calculateRegisterValue(current_mA: number, r_sense: number): number {
  if (r_sense <= 0) return 0 // Avoid division by zero

  const VFS_VOLTAGE = 0.325 // Standard full-scale reference voltage
  const INTERNAL_RESISTANCE = 0.02 // Driver internal resistance

  // This "magic number" is (Math.sqrt(2) * 32) / VFS_VOLTAGE
  const VFS_FACTOR = 138.8038

  const current_Amps = current_mA / 1000

  // Calculate the value
  let registerValue =
      (current_Amps * (r_sense + INTERNAL_RESISTANCE) * VFS_FACTOR) - 1

  // Round to the nearest integer and clamp between 0 and 31
  const roundedValue = Math.round(registerValue)
  return Math.max(0, Math.min(31, roundedValue))
}

// Computed properties to automatically update the register value
const computedRunValue = computed(() =>
    calculateRegisterValue(runCurrent_mA.value, senseResistorOhms.value)
)

const computedHoldValue = computed(() =>
    calculateRegisterValue(holdCurrent_mA.value, senseResistorOhms.value)
)
</script>

<template>
  <Card class="w-full max-w-2xl">
    <CardHeader>
      <CardTitle>TMC2209 Motor Configuration</CardTitle>
      <CardDescription>
        Configure all common driver settings in one place. Be careful,
        incorrect settings can cause overheating.
      </CardDescription>
    </CardHeader>
    <CardContent class="space-y-6 pt-2">
      <div>
        <h3 class="text-lg font-medium">⚡ Motor Current</h3>
        <p class="text-sm text-muted-foreground">
          Controls the power sent to the motor during movement and while
          stationary.
        </p>
      </div>

      <div class="space-y-2 p-3 bg-secondary/50 border rounded-lg">
        <Label for="rsense" class="font-bold">Sense Resistor (Ohms)</Label>
        <Input
            id="rsense"
            type="number"
            v-model.number="senseResistorOhms"
            step="0.01"
        />
        <p class="text-sm text-muted-foreground">
          **This is critical for correct current calculation.** Find this
          value on your driver's datasheet.
          Common values: **0.110** (BTT), **0.150** (Watterott).
        </p>
      </div>


      <div class="space-y-6 pl-2 border-l-2 border-dashed">
        <div class="space-y-2">
          <Label for="irun-ma">Run Current (mA)</Label>
          <Input
              id="irun-ma"
              type="number"
              v-model.number="runCurrent_mA"
              step="50"
          />
          <p class="text-sm text-muted-foreground">
            Enter your motor's **RMS current** (in mA). Usually ~70-85% of
            the motor's peak rating. This is the current used when
            **moving**.
          </p>
          <div class="p-2 bg-muted rounded">
            <span class="font-mono text-sm">
              Calculated IRUN: {{ computedRunValue }}/31
            </span>
          </div>
        </div>

        <div class="space-y-2">
          <Label for="ihold-ma">Hold Current (mA)</Label>
          <Input
              id="ihold-ma"
              type="number"
              v-model.number="holdCurrent_mA"
              step="50"
          />
          <p class="text-sm text-muted-foreground">
            Enter the desired **stationary** current (in mA). Usually 50-70%
            of Run Current. Lower values save power and reduce heat.
          </p>
          <div class="p-2 bg-muted rounded">
            <span class="font-mono text-sm">
              Calculated IHOLD: {{ computedHoldValue }}/31
            </span>
          </div>
        </div>

        <div class="space-y-2">
          <Label for="iholddelay">
            Hold Delay (IHOLDDELAY: {{ iholddelay[0] }}/15)
          </Label>
          <Slider id="iholddelay" v-model="iholddelay" :max="15" :step="1" />
          <p class="text-sm text-muted-foreground">
            The "wait time" before the driver reduces current from `IRUN` to
            `IHOLD` after the motor stops. A value of 4-8 is typical.
          </p>
        </div>
      </div>

      <Separator class="my-6" />

      <div>
        <h3 class="text-lg font-medium">🎧 Driver Mode</h3>
        <p class="text-sm text-muted-foreground">
          This is the main trade-off between quietness and high-speed torque.
        </p>
      </div>

      <RadioGroup v-model="driverMode" default-value="stealthchop" class="space-y-2">
        <div class="flex items-center space-x-2">
          <RadioGroupItem id="r-stealth" value="stealthchop" />
          <Label for="r-stealth">StealthChop (Quiet Mode)</Label>
        </div>
        <p class="pl-6 text-sm text-muted-foreground">
          **Extremely quiet** and smooth, ideal for 3D printers and
          low-to-medium speed applications.
        </p>

        <div class="flex items-center space-x-2">
          <RadioGroupItem id="r-spread" value="spreadcycle" />
          <Label for="r-spread">SpreadCycle (Torque Mode)</Label>
        </div>
        <p class="pl-6 text-sm text-muted-foreground">
          **High-torque** mode. It's louder, but provides better performance at
          high velocities and under heavy load.
        </p>

        <div class="flex items-center space-x-2">
          <RadioGroupItem id="r-hybrid" value="hybrid" />
          <Label for="r-hybrid">Hybrid Mode (Automatic)</Label>
        </div>
        <p class="pl-6 text-sm text-muted-foreground">
          The **best of both worlds**. Runs in quiet StealthChop and
          automatically switches to SpreadCycle when the speed exceeds the
          threshold you set below.
        </p>
      </RadioGroup>

      <div v-if="driverMode === 'hybrid'" class="space-y-2 pt-4 border-t mt-4">
        <Label for="tpwmthrs">Hybrid Speed Threshold (TPWMTHRS)</Label>
        <Slider id="tpwmthrs" v-model="tpwmthrs" :max="1048575" :step="100" />
        <p class="text-sm text-muted-foreground">
          Sets the speed at which the driver switches from StealthChop to
          SpreadCycle. A value of 0 forces StealthChop.
        </p>
      </div>

      <Separator class="my-6" />

      <div>
        <h3 class="text-lg font-medium">👟 Microstep Resolution</h3>
        <p class="text-sm text-muted-foreground">
          Controls the smoothness of the motor by dividing each full step into
          smaller "microsteps".
        </p>
      </div>

      <div class="space-y-2">
        <Label for="mres">Microsteps per Full Step</Label>
        <Select v-model="mres">
          <SelectTrigger id="mres">
            <SelectValue placeholder="Select resolution" />
          </SelectTrigger>
          <SelectContent>
            <SelectItem value="0">1/256 (Native, Smoothest)</SelectItem>
            <SelectItem value="1">1/128</SelectItem>
            <SelectItem value="2">1/64</SelectItem>
            <SelectItem value="3">1/32</SelectItem>
            <SelectItem value="4">1/16 (Common Default)</SelectItem>
            <SelectItem value="5">1/8</SelectItem>
            <SelectItem value="6">1/4</SelectItem>
            <SelectItem value="7">1/2 (Half Step)</SelectItem>
            <SelectItem value="8">1 (Full Step, Max Torque)</SelectItem>
          </SelectContent>
        </Select>
        <p class="text-sm text-muted-foreground">
          **1/256** is the native setting for StealthChop and provides the
          smoothest motion. **1/16** is a very common and reliable setting.
        </p>
      </div>

      <div class="flex items-center space-x-2 pt-2">
        <Checkbox id="interpolate" v-model="interpolate" />
        <Label for="interpolate" class="font-normal">
          Enable Interpolation
        </Label>
      </div>
      <p class="pl-6 text-sm text-muted-foreground">
        **Highly Recommended.** If checked, the driver will automatically
        interpolate any setting (like 1/16) up to 1/256 steps.
      </p>

      <Separator class="my-6" />

      <div>
        <h3 class="text-lg font-medium">🔄 Motor Direction</h3>
        <p class="text-sm text-muted-foreground">
          A quick fix if the motor is spinning in the reverse direction.
        </p>
      </div>

      <div class="flex items-center space-x-2">
        <Switch id="direction-toggle" v-model="reverseDirection" />
        <Label for="direction-toggle">Reverse Motor Direction</Label>
      </div>
      <p class="pt-2 text-sm text-muted-foreground">
        Toggles the `REFR_DIR` bit. This is much easier than
        rewiring the motor connector.
      </p>
    </CardContent>
    <CardFooter>
      <Button class="w-full">
        Save All Settings (IRUN={{ computedRunValue }}, IHOLD={{
          computedHoldValue
        }})
      </Button>
    </CardFooter>
  </Card>
</template>
