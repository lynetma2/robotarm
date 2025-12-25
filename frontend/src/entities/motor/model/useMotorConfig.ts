import { ref, computed } from 'vue'
import type { MotorConfig } from './types'

// --- Helpers ---
const generateDefaults = (i: number): MotorConfig => ({
  id: i,
  name: `Motor ${i + 1}`,
  enabled: true,
  runCurrent: 800,
  holdCurrent: 400,
  microsteps: 16,
  stallThreshold: 10,
  coolStep: true,
  spreadCycle: false,
  interpolation: true,
  pwmAutoScale: true,
})

// --- State (Singleton) ---
const motorConfigs = ref<MotorConfig[]>(
  Array.from({ length: 6 }, (_, i) => generateDefaults(i))
)

const lastSavedState = ref(JSON.stringify(motorConfigs.value))

// --- Composable ---
export function useMotorConfig() {
  const resetMotor = (motorId: number) => {
    const defaults = generateDefaults(motorId)
    const index = motorConfigs.value.findIndex(m => m.id === motorId)
    if (index !== -1) {
      motorConfigs.value[index] = { ...motorConfigs.value[index], ...defaults }
    }
  }

  const hasChanges = computed(() => JSON.stringify(motorConfigs.value) !== lastSavedState.value)

  const markAsSaved = () => {
    lastSavedState.value = JSON.stringify(motorConfigs.value)
  }

  return { motorConfigs, resetMotor, hasChanges, markAsSaved }
}