import { ref, computed } from 'vue'
import type { MotorConfig } from './types'

const API_BASE = 'http://localhost:8080/api/v1/config/tmc2209'

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
const isLoading = ref(false)
const error = ref<string | null>(null)

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

  const fetchConfigs = async () => {
    isLoading.value = true
    error.value = null
    try {
      // Fetch all 6 motors in parallel
      const promises = Array.from({ length: 6 }, (_, i) =>
        fetch(`${API_BASE}/${i}`).then(async (res) => {
          if (!res.ok) throw new Error(`Failed to fetch motor ${i}`)
          return { id: i, data: await res.json() }
        })
      )

      const results = await Promise.all(promises)

      // Update state
      results.forEach(({ id, data }) => {
        const index = motorConfigs.value.findIndex(m => m.id === id)
        if (index !== -1) {
          // Merge backend data with existing (preserves name if backend doesn't send it)
          motorConfigs.value[index] = { ...motorConfigs.value[index], ...data }
        }
      })

      markAsSaved()
    } catch (e) {
      console.error("Failed to fetch motor configs", e)
      error.value = "Failed to load configuration from robot."
    } finally {
      isLoading.value = false
    }
  }

  const saveConfigs = async () => {
    isLoading.value = true
    error.value = null
    try {
      const promises = motorConfigs.value.map(config =>
        fetch(`${API_BASE}/${config.id}`, {
          method: 'PUT',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(config)
        }).then(async res => {
          if (!res.ok) throw new Error(`Failed to save motor ${config.id}`)
          return { id: config.id, data: await res.json() }
        })
      )

      const results = await Promise.all(promises)

      results.forEach(({ id, data }) => {
        const index = motorConfigs.value.findIndex(m => m.id === id)
        if (index !== -1) {
          motorConfigs.value[index] = { ...motorConfigs.value[index], ...data }
        }
      })

      markAsSaved()
    } catch (e) {
      console.error("Failed to save motor configs", e)
      error.value = "Failed to save configuration."
    } finally {
      isLoading.value = false
    }
  }

  return {
    motorConfigs,
    resetMotor,
    hasChanges,
    markAsSaved,
    fetchConfigs,
    saveConfigs,
    isLoading,
    error
  }
}
