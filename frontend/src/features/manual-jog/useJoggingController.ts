import { ref, watch } from 'vue'
import { useGamepad } from '@/shared/lib/useGamepad'
import { useStomp } from '@/shared/api/useStomp'

// Import Types and Constants from Entity
import { ROBOT_JOINTS } from '@/entities/robot/constants'
import { type JogCommand, JogDirection } from '@/entities/robot/types'

export function useJoggingController() {
  const { isConnected, buttons } = useGamepad()
  const { publish } = useStomp()

  const selectedJointIndex = ref(0)
  const isJoggingPos = ref(false)
  const isJoggingNeg = ref(false)
  const jogSpeed = ref(50)

  // --- Helper to Send Command ---
  const sendJog = (direction: JogDirection) => {
    const joint = ROBOT_JOINTS[selectedJointIndex.value]

    const payload: JogCommand = {
      motorId: joint.id,
      direction: direction,
      speed: jogSpeed.value / 100
    }

    publish('/app/jog', payload)
  }

  // --- Watcher ---
  watch(buttons, (newButtons) => {
    if (!isConnected.value || newButtons.length === 0) return

    // Navigation (D-Pad)
    // Index 13 = Down, 12 = Up
    if (newButtons[13]) {
      let newIndex = selectedJointIndex.value + 1
      if (newIndex >= ROBOT_JOINTS.length) newIndex = 0
      selectedJointIndex.value = newIndex
    }
    if (newButtons[12]) {
      let newIndex = selectedJointIndex.value - 1
      if (newIndex < 0) newIndex = ROBOT_JOINTS.length - 1
      selectedJointIndex.value = newIndex
    }

    // State
    isJoggingPos.value = newButtons[7]
    isJoggingNeg.value = newButtons[6]

    // Execution
    if (isJoggingPos.value) sendJog(JogDirection.POSITIVE)
    if (isJoggingNeg.value) sendJog(JogDirection.NEGATIVE)
  })

  return {
    isControllerConnected: isConnected,
    selectedJointIndex,
    isJoggingPos,
    isJoggingNeg,
    jogSpeed,

    // Actions
    jogPositive: () => sendJog(JogDirection.POSITIVE),
    jogNegative: () => sendJog(JogDirection.NEGATIVE),
    selectJoint: (i: number) => selectedJointIndex.value = i,

    // 👇 THESE WERE MISSING:
    setJoggingPos: (val: boolean) => isJoggingPos.value = val,
    setJoggingNeg: (val: boolean) => isJoggingNeg.value = val,
  }
}
