<script setup lang="ts">
import { ref, computed } from 'vue'

// --- shadcn-vue Component Imports ---
import { Button } from '@/shared/ui/button'
import { Input } from '@/shared/ui/input'
import { Label } from '@/shared/ui/label'
import { Slider } from '@/shared/ui/slider'
import { CardFooter } from '@/shared/ui/card' // Added for the footer

// --- Reactive State Definitions ---
const runCurrent_mA = ref(800) // User input in Milliamps
const holdCurrent_mA = ref(500) // User input in Milliamps
const senseResistorOhms = ref(0.110) // CRITICAL: User must set this
const iholddelay = ref([8]) // 0-15 value

// --- Calculation Logic ---
/**
 * Calculates the 5-bit register value (0-31) for IRUN or IHOLD.
 */
function calculateRegisterValue(current_mA: number, r_sense: number): number {
  if (r_sense <= 0) return 0 // Avoid division by zero

  const VFS_VOLTAGE = 0.325
  const INTERNAL_RESISTANCE = 0.02
  const VFS_FACTOR = 138.8038 // (Math.sqrt(2) * 32) / VFS_VOLTAGE

  const current_Amps = current_mA / 1000
  let registerValue =
    (current_Amps * (r_sense + INTERNAL_RESISTANCE) * VFS_FACTOR) - 1

  const roundedValue = Math.round(registerValue)
  const test = "test"
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
  <div class="space-y-8">
    <div class="space-y-2 p-4 bg-secondary/50 border rounded-lg">
      <Label for="rsense" class="text-base font-semibold">
        Sense Resistor (R-Sense)
      </Label>
      <Input
        id="rsense"
        type="number"
        v-model.number="senseResistorOhms"
        step="0.01"
      />

      <p class="text-sm text-muted-foreground">
        <span class="font-semibold text-foreground">
          This is critical for correct current calculation.
        </span>
        Find this value on your driver's datasheet. Common values:
        <span class="font-semibold text-foreground">0.110</span> (BTT),
        <span class="font-semibold text-foreground">0.150</span> (Watterott).
      </p>
    </div>

    <div class="space-y-2">
      <Label for="irun-ma" class="text-base">Run Current (mA)</Label>
      <Input
        id="irun-ma"
        type="number"
        v-model.number="runCurrent_mA"
        step="50"
        placeholder="e.g. 800"
      />
      <p class="text-sm text-muted-foreground">
        Enter your motor's
        <span class="font-semibold text-foreground">RMS current</span>
        (in mA). Usually ~70-85% of the motor's peak rating. This is the
        current used when
        <span class="font-semibold text-foreground">moving</span>.
      </p>
      <div class="p-2 bg-muted rounded">
        <span class="font-mono text-sm">
          Calculated IRUN Register: {{ computedRunValue }}/31
        </span>
      </div>
    </div>


    <div class="space-y-2">
      <Label for="ihold-ma" class="text-base">Hold Current (mA)</Label>
      <Input
        id="ihold-ma"
        type="number"
        v-model.number="holdCurrent_mA"
        step="50"
        placeholder="e.g. 500"
      />
      <p class="text-sm text-muted-foreground">
        Enter the desired
        <span class="font-semibold text-foreground">stationary</span>
        current (in mA). Usually 50-70% of Run Current. Lower values save
        power and reduce heat.
      </p>
      <div class="p-2 bg-muted rounded">
        <span class="font-mono text-sm">
          Calculated IHOLD Register: {{ computedHoldValue }}/31
        </span>
      </div>
    </div>

    <div class="space-y-2">
      <Label for="iholddelay" class="text-base">
        Hold Delay (IHOLDDELAY: {{ iholddelay[0] }}/15)
      </Label>
      <Slider id="iholddelay" v-model="iholddelay" :max="15" :step="1" />
      <p class="text-sm text-muted-foreground">
        The "wait time" before the driver reduces current from `IRUN` to
        `IHOLD` after the motor stops. A value of 4-8 is typical.
      </p>
    </div>

    <CardFooter class="p-0 pt-4">
      <Button class="w-full text-base">
        Save Currents (IRUN={{ computedRunValue }}, IHOLD={{
          computedHoldValue
        }})
      </Button>
    </CardFooter>
  </div>
</template>
