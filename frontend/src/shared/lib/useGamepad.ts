// src/composables/useGamepad.ts
import { ref, onMounted, onUnmounted } from 'vue'

export function useGamepad() {
  const isConnected = ref(false)
  const gamepadIndex = ref<number | null>(null)

  // State for buttons and axes.
  // Standard mapping: 0=A/Cross, 1=B/Circle, 2=X/Square, 3=Y/Triangle
  // 12=DpadUp, 13=DpadDown, 14=DpadLeft, 15=DpadRight
  // 6=L2, 7=R2 (Triggers)
  const buttons = ref<boolean[]>([])
  const axes = ref<number[]>([])

  let reqId: number

  const updateLoop = () => {
    if (gamepadIndex.value !== null) {
      const gp = navigator.getGamepads()[gamepadIndex.value]
      if (gp) {
        // Map buttons to simple booleans
        buttons.value = gp.buttons.map(b => b.pressed)
        // Map axes
        axes.value = [...gp.axes]
      }
    }
    reqId = requestAnimationFrame(updateLoop)
  }

  const onConnected = (e: GamepadEvent) => {
    console.log("Gamepad connected:", e.gamepad.id)
    gamepadIndex.value = e.gamepad.index
    isConnected.value = true
    updateLoop()
  }

  const onDisconnected = (e: GamepadEvent) => {
    if (e.gamepad.index === gamepadIndex.value) {
      console.log("Gamepad disconnected")
      isConnected.value = false
      gamepadIndex.value = null
      cancelAnimationFrame(reqId)
    }
  }

  onMounted(() => {
    window.addEventListener("gamepadconnected", onConnected)
    window.addEventListener("gamepaddisconnected", onDisconnected)
  })

  onUnmounted(() => {
    window.removeEventListener("gamepadconnected", onConnected)
    window.removeEventListener("gamepaddisconnected", onDisconnected)
    cancelAnimationFrame(reqId)
  })

  return {
    isConnected,
    buttons,
    axes
  }
}
