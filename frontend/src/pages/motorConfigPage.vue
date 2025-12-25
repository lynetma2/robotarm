<script setup lang="ts">
import { ref, computed } from 'vue'
import {
  Wrench,
  Save,
  Activity,
  Thermometer,
  Zap,
  AlertCircle,
  Cpu // Using CPU icon for the "Motor Cortex" vibe
} from 'lucide-vue-next'

// --- Types ---
type Microsteps = 8 | 16 | 32 | 64 | 128 | 256

interface MotorConfig {
  id: number
  name: string
  enabled: boolean
  runCurrent: number // mA
  holdCurrent: number // mA
  microsteps: Microsteps
  interpolation: boolean
  stealthChop: boolean // true = silent, false = spreadCycle
  stallGuardThreshold: number // 0-255
  reverseDirection: boolean
  status: 'ok' | 'warning' | 'error'
}

// --- State ---
const activeMotorId = ref<number>(0)

// Mock Data for 6-Axis Arm
const motors = ref<MotorConfig[]>([
  {
    id: 0,
    name: 'Axis 1 (Base)',
    enabled: true,
    runCurrent: 800,
    holdCurrent: 400,
    microsteps: 16,
    interpolation: true,
    stealthChop: true,
    stallGuardThreshold: 8,
    reverseDirection: false,
    status: 'ok'
  },
  {
    id: 1,
    name: 'Axis 2 (Shoulder)',
    enabled: true,
    runCurrent: 1200,
    holdCurrent: 600,
    microsteps: 16,
    interpolation: true,
    stealthChop: false,
    stallGuardThreshold: 12,
    reverseDirection: true,
    status: 'ok'
  },
  {
    id: 2,
    name: 'Axis 3 (Elbow)',
    enabled: true,
    runCurrent: 1000,
    holdCurrent: 500,
    microsteps: 16,
    interpolation: true,
    stealthChop: true,
    stallGuardThreshold: 10,
    reverseDirection: false,
    status: 'warning' // Example warning state
  },
  {
    id: 3,
    name: 'Axis 4 (Wrist 1)',
    enabled: true,
    runCurrent: 600,
    holdCurrent: 200,
    microsteps: 32,
    interpolation: true,
    stealthChop: true,
    stallGuardThreshold: 5,
    reverseDirection: false,
    status: 'ok'
  },
  {
    id: 4,
    name: 'Axis 5 (Wrist 2)',
    enabled: true,
    runCurrent: 600,
    holdCurrent: 200,
    microsteps: 32,
    interpolation: true,
    stealthChop: true,
    stallGuardThreshold: 5,
    reverseDirection: false,
    status: 'ok'
  },
  {
    id: 5,
    name: 'Axis 6 (Wrist 3)',
    enabled: false,
    runCurrent: 400,
    holdCurrent: 100,
    microsteps: 32,
    interpolation: true,
    stealthChop: true,
    stallGuardThreshold: 2,
    reverseDirection: false,
    status: 'ok'
  },
])

// --- Computed ---
const activeMotor = computed(() => {
  return motors.value.find(m => m.id === activeMotorId.value) || motors.value[0]
})

// --- Actions ---
const saveConfig = () => {
  console.log('Saving config to EEPROM/Backend:', JSON.stringify(motors.value))
  // Add your API call here
}

const emergencyStop = () => {
  console.error('EMERGENCY STOP TRIGGERED')
  motors.value.forEach(m => m.enabled = false)
}
</script>

<template>
  <div class="min-h-screen bg-neutral-50 text-neutral-900 flex flex-col font-sans">

    <header class="h-16 border-b bg-white px-6 flex items-center justify-between shrink-0 z-10">
      <div class="flex items-center gap-3">
        <div class="p-2 bg-neutral-900 text-white rounded-lg">
          <Cpu class="w-5 h-5" />
        </div>
        <div>
          <h1 class="font-bold text-lg leading-tight">Motor Cortex</h1>
          <p class="text-xs text-neutral-500">TMC2209 Configuration</p>
        </div>
      </div>

      <div class="flex items-center gap-3">
        <button
          @click="emergencyStop"
          class="bg-red-600 hover:bg-red-700 text-white px-4 py-2 rounded-md text-sm font-semibold shadow-sm transition-colors"
        >
          E-STOP
        </button>
        <button
          @click="saveConfig"
          class="bg-neutral-900 hover:bg-neutral-800 text-white px-4 py-2 rounded-md text-sm font-medium flex items-center gap-2 shadow-sm transition-colors"
        >
          <Save class="w-4 h-4" /> Save All
        </button>
      </div>
    </header>

    <div class="flex flex-1 overflow-hidden">

      <aside class="w-72 bg-white border-r flex flex-col">
        <div class="p-4 space-y-1 overflow-y-auto flex-1">
          <div class="text-xs font-bold text-neutral-400 uppercase tracking-wider mb-4 px-2">Joints</div>

          <button
            v-for="motor in motors"
            :key="motor.id"
            @click="activeMotorId = motor.id"
            class="w-full text-left px-3 py-3 rounded-lg text-sm font-medium transition-all flex items-center justify-between group relative"
            :class="activeMotorId === motor.id ? 'bg-neutral-100 text-neutral-900' : 'text-neutral-500 hover:bg-neutral-50'"
          >
            <div v-if="activeMotorId === motor.id" class="absolute left-0 top-2 bottom-2 w-1 bg-neutral-900 rounded-r-full"></div>

            <div class="pl-2">
              <span class="block">{{ motor.name }}</span>
              <span class="text-[10px] font-mono opacity-70">UART: 0{{ motor.id }}</span>
            </div>

            <div class="flex items-center gap-2">
              <span
                class="w-2 h-2 rounded-full ring-2 ring-white"
                :class="{
                  'bg-emerald-500': motor.status === 'ok',
                  'bg-amber-500': motor.status === 'warning',
                  'bg-red-500': motor.status === 'error'
                }"
              ></span>
            </div>
          </button>
        </div>
      </aside>

      <main class="flex-1 overflow-y-auto bg-neutral-50/50 p-8">
        <div class="max-w-5xl mx-auto space-y-6">

          <div class="flex items-center justify-between mb-6">
            <div>
              <h2 class="text-3xl font-bold text-neutral-900">{{ activeMotor.name }}</h2>
              <div class="flex items-center gap-2 text-neutral-500 text-sm mt-1">
                <span class="font-mono bg-neutral-200 px-1.5 rounded">ID: {{ activeMotor.id }}</span>
                <span>•</span>
                <span>Driver Status: Ready</span>
              </div>
            </div>

            <div
              @click="activeMotor.enabled = !activeMotor.enabled"
              class="cursor-pointer flex items-center gap-3 px-4 py-2 bg-white border rounded-full shadow-sm hover:bg-neutral-50 transition-colors select-none"
            >
              <span class="text-sm font-semibold" :class="activeMotor.enabled ? 'text-emerald-600' : 'text-neutral-400'">
                {{ activeMotor.enabled ? 'Motor Enabled' : 'Motor Disabled' }}
              </span>
              <div
                class="w-10 h-6 rounded-full relative transition-colors duration-200"
                :class="activeMotor.enabled ? 'bg-emerald-500' : 'bg-neutral-200'"
              >
                <div
                  class="absolute top-1 w-4 h-4 bg-white rounded-full shadow transition-transform duration-200"
                  :class="activeMotor.enabled ? 'translate-x-5' : 'translate-x-1'"
                ></div>
              </div>
            </div>
          </div>

          <div class="grid grid-cols-1 md:grid-cols-2 gap-6">

            <div class="bg-white rounded-xl border p-6 shadow-sm space-y-6">
              <div class="flex items-center gap-2 pb-4 border-b">
                <Zap class="w-5 h-5 text-amber-500" />
                <h3 class="font-bold text-neutral-900">Current Control</h3>
              </div>

              <div class="space-y-4">
                <div class="flex justify-between items-end">
                  <label class="text-sm font-medium text-neutral-700">Run Current (I_RMS)</label>
                  <div class="px-2 py-0.5 bg-neutral-100 rounded text-sm font-mono font-bold">{{ activeMotor.runCurrent }} mA</div>
                </div>
                <input
                  type="range" min="100" max="2000" step="50"
                  v-model.number="activeMotor.runCurrent"
                  class="range-slider w-full"
                />
              </div>

              <div class="space-y-4">
                <div class="flex justify-between items-end">
                  <label class="text-sm font-medium text-neutral-700">Hold Current</label>
                  <div class="px-2 py-0.5 bg-neutral-100 rounded text-sm font-mono font-bold">{{ activeMotor.holdCurrent }} mA</div>
                </div>
                <input
                  type="range" min="0" max="1000" step="50"
                  v-model.number="activeMotor.holdCurrent"
                  class="range-slider w-full"
                />
                <p class="text-xs text-neutral-400">Lower hold current reduces heat when idle.</p>
              </div>
            </div>

            <div class="bg-white rounded-xl border p-6 shadow-sm space-y-6">
              <div class="flex items-center gap-2 pb-4 border-b">
                <Activity class="w-5 h-5 text-blue-500" />
                <h3 class="font-bold text-neutral-900">Motion Steps</h3>
              </div>

              <div class="grid grid-cols-2 gap-4">
                <div class="col-span-2">
                  <label class="block text-sm font-medium text-neutral-700 mb-2">Microsteps</label>
                  <select
                    v-model.number="activeMotor.microsteps"
                    class="w-full h-10 px-3 rounded-md border border-neutral-300 bg-white text-sm focus:outline-none focus:ring-2 focus:ring-neutral-900"
                  >
                    <option :value="8">1/8 Step</option>
                    <option :value="16">1/16 Step</option>
                    <option :value="32">1/32 Step</option>
                    <option :value="64">1/64 Step</option>
                    <option :value="128">1/128 Step</option>
                    <option :value="256">1/256 Step</option>
                  </select>
                </div>

                <div class="col-span-2 flex items-center justify-between py-2">
                  <div>
                    <div class="text-sm font-medium">Interpolation</div>
                    <div class="text-xs text-neutral-400">Smooth 256 steps</div>
                  </div>
                  <button
                    @click="activeMotor.interpolation = !activeMotor.interpolation"
                    class="w-10 h-6 rounded-full relative transition-colors duration-200"
                    :class="activeMotor.interpolation ? 'bg-blue-600' : 'bg-neutral-200'"
                  >
                    <span
                      class="absolute top-1 w-4 h-4 bg-white rounded-full shadow transition-transform duration-200"
                      :class="activeMotor.interpolation ? 'translate-x-5' : 'translate-x-1'"
                    ></span>
                  </button>
                </div>

                <div class="col-span-2 flex items-center justify-between py-2 border-t border-dashed">
                  <div>
                    <div class="text-sm font-medium">Invert Direction</div>
                  </div>
                  <button
                    @click="activeMotor.reverseDirection = !activeMotor.reverseDirection"
                    class="w-10 h-6 rounded-full relative transition-colors duration-200"
                    :class="activeMotor.reverseDirection ? 'bg-neutral-900' : 'bg-neutral-200'"
                  >
                    <span
                      class="absolute top-1 w-4 h-4 bg-white rounded-full shadow transition-transform duration-200"
                      :class="activeMotor.reverseDirection ? 'translate-x-5' : 'translate-x-1'"
                    ></span>
                  </button>
                </div>
              </div>
            </div>

            <div class="bg-white rounded-xl border p-6 shadow-sm md:col-span-2 space-y-6">
              <div class="flex items-center gap-2 pb-4 border-b">
                <Thermometer class="w-5 h-5 text-rose-500" />
                <h3 class="font-bold text-neutral-900">Advanced TMC2209 Tuning</h3>
              </div>

              <div class="grid grid-cols-1 md:grid-cols-2 gap-12">

                <div>
                  <div class="flex items-center justify-between mb-4">
                    <div>
                      <div class="text-sm font-medium">StealthChop™</div>
                      <div class="text-xs text-neutral-400">Silent operation mode</div>
                    </div>
                    <button
                      @click="activeMotor.stealthChop = !activeMotor.stealthChop"
                      class="w-10 h-6 rounded-full relative transition-colors duration-200"
                      :class="activeMotor.stealthChop ? 'bg-emerald-500' : 'bg-neutral-200'"
                    >
                      <span
                        class="absolute top-1 w-4 h-4 bg-white rounded-full shadow transition-transform duration-200"
                        :class="activeMotor.stealthChop ? 'translate-x-5' : 'translate-x-1'"
                      ></span>
                    </button>
                  </div>

                  <div v-if="!activeMotor.stealthChop" class="bg-amber-50 border border-amber-200 rounded-md p-3 flex gap-3 text-amber-800">
                    <AlertCircle class="w-5 h-5 shrink-0" />
                    <p class="text-xs leading-relaxed">
                      <strong>SpreadCycle Active:</strong> Motors will be louder but provide higher torque at high speeds. Recommended for the base axis.
                    </p>
                  </div>
                </div>

                <div class="space-y-4">
                  <div class="flex justify-between items-end">
                    <div>
                      <label class="text-sm font-medium text-neutral-700">StallGuard Threshold</label>
                      <p class="text-[10px] text-neutral-400">Sensorless homing sensitivity (SGTHRS)</p>
                    </div>
                    <div class="px-2 py-0.5 bg-neutral-100 rounded text-sm font-mono font-bold">{{ activeMotor.stallGuardThreshold }}</div>
                  </div>
                  <input
                    type="range" min="0" max="255"
                    v-model.number="activeMotor.stallGuardThreshold"
                    class="range-slider w-full"
                  />

                  <div class="pt-2">
                    <div class="flex justify-between text-[10px] uppercase text-neutral-400 font-bold mb-1">
                      <span>Low Sensitivity</span>
                      <span>High Sensitivity</span>
                    </div>
                    <div class="h-1.5 w-full bg-neutral-100 rounded-full overflow-hidden flex">
                      <div class="h-full bg-neutral-300 flex-1 border-r border-white"></div>
                      <div class="h-full bg-neutral-300 flex-1 border-r border-white"></div>
                      <div class="h-full bg-neutral-300 flex-1"></div>
                    </div>
                  </div>
                </div>

              </div>
            </div>

          </div>
        </div>
      </main>
    </div>
  </div>
</template>

<style scoped>
/* Custom Range Slider Styling to match Shadcn/Tailwind aesthetic */
.range-slider {
  -webkit-appearance: none;
  height: 6px;
  background: #e5e5e5; /* neutral-200 */
  border-radius: 9999px;
  outline: none;
}

/* Chrome/Safari/Edge Thumb */
.range-slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 18px;
  height: 18px;
  background: #171717; /* neutral-900 */
  border-radius: 50%;
  cursor: pointer;
  border: 2px solid white;
  box-shadow: 0 1px 3px rgba(0,0,0,0.1);
  transition: transform 0.1s ease;
}

.range-slider::-webkit-slider-thumb:hover {
  transform: scale(1.1);
}

/* Firefox Thumb */
.range-slider::-moz-range-thumb {
  width: 18px;
  height: 18px;
  background: #171717;
  border-radius: 50%;
  cursor: pointer;
  border: 2px solid white;
  box-shadow: 0 1px 3px rgba(0,0,0,0.1);
}
</style>
