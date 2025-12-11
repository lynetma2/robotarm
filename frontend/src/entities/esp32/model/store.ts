import { defineStore } from 'pinia'
import { ref } from 'vue'

export const useMachineStore = defineStore('machine', () => {
  // State
  const isSerialConnected = ref(true) // Default to true or false based on logic

  // Actions
  function toggleSerial() {
    isSerialConnected.value = !isSerialConnected.value
  }

  function setSerial(status: boolean) {
    isSerialConnected.value = status
  }

  return { isSerialConnected, toggleSerial, setSerial }
})
