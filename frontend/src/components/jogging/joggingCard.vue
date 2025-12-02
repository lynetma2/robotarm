<script setup lang="ts">
import { ref, watch } from 'vue'
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { ChevronRight, ChevronLeft, Gamepad2 } from 'lucide-vue-next'
import { useGamepad } from '@/composable/useGamepad'
import { useStomp } from '@/composable/useStomp'

// --- Gamepad Hook ---
const { isConnected, buttons } = useGamepad()

// --- STOMP Hook ---
const { publish } = useStomp()

// --- State ---
const joints = [
  { id: 1, name: 'Base (J1)' },
  { id: 2, name: 'Shoulder (J2)' },
  { id: 3, name: 'Elbow (J3)' },
  { id: 4, name: 'Wrist 1 (J4)' },
  { id: 5, name: 'Wrist 2 (J5)' },
  { id: 6, name: 'Wrist 3 (J6)' },
]

const selectedJointIndex = ref(0)
const isJoggingPos = ref(false)
const isJoggingNeg = ref(false)
const jogSpeed = ref(50) // 0-100%

// --- Controller Logic ---
// We watch the buttons array from the composable
watch(buttons, (newButtons) => {
  if (!isConnected.value || newButtons.length === 0) return

  // 1. Navigation (D-Pad Up/Down)
  // Index 12 = Dpad Up, 13 = Dpad Down
  if (newButtons[13]) handleSelectionChange(1)
  if (newButtons[12]) handleSelectionChange(-1)

  // 2. Jogging (Triggers)
  // Index 7 = R2 (Pos), 6 = L2 (Neg)
  // Using a threshold or boolean logic depending on browser/controller mapping
  isJoggingPos.value = newButtons[7]
  isJoggingNeg.value = newButtons[6]

  // Send jog commands via STOMP
  const jointId = joints[selectedJointIndex.value].id
  if (isJoggingPos.value) {
    publish('/app/jog', { motorId: jointId, direction: 1, speed: jogSpeed.value / 100 })
    console.log(`Jogging J${jointId} POS`)
  }
  if (isJoggingNeg.value) {
    publish('/app/jog', { motorId: jointId, direction: -1, speed: jogSpeed.value / 100 })
    console.log(`Jogging J${jointId} NEG`)
  }
})

// Debounce selection change so it doesn't fly through the list
let lastSelectionTime = 0
const handleSelectionChange = (direction: number) => {
  const now = Date.now()
  if (now - lastSelectionTime < 150) return // 150ms debounce

  let newIndex = selectedJointIndex.value + direction
  if (newIndex < 0) newIndex = joints.length - 1
  if (newIndex >= joints.length) newIndex = 0

  selectedJointIndex.value = newIndex
  lastSelectionTime = now
}
</script>

<template>
  <Card class="flex flex-col bg-card">
    <CardHeader class="pb-2 flex flex-row items-center justify-between">
      <CardTitle class="text-lg font-semibold">Manual Jogging</CardTitle>
      <div v-if="isConnected" class="flex items-center text-xs text-purple-500 font-medium animate-pulse">
        <Gamepad2 class="w-4 h-4 mr-1"/> PS5 Connected
      </div>
    </CardHeader>

    <CardContent class="grid gap-3">

      <div v-if="isConnected" class="text-xs bg-secondary/50 p-2 rounded text-muted-foreground mb-1">
        <span class="font-bold">D-Pad:</span> Select Joint &nbsp;|&nbsp;
        <span class="font-bold">L2/R2:</span> Jog - / +
      </div>

      <div class="flex flex-col gap-2">
        <div
          v-for="(joint, index) in joints"
          :key="joint.id"
          class="flex items-center justify-between p-2 rounded border transition-all"
          :class="{
            'border-primary bg-primary/5 ring-1 ring-primary/20': index === selectedJointIndex && isConnected,
            'border-border': index !== selectedJointIndex
          }"
          @click="selectedJointIndex = index"
        >
          <span class="text-sm font-medium w-24">{{ joint.name }}</span>

          <div class="flex items-center gap-2">
            <Button
              variant="outline"
              size="sm"
              class="h-8 w-12"
              :class="{ 'bg-red-100 border-red-500 text-red-700': index === selectedJointIndex && isJoggingNeg }"
              @mousedown="isJoggingNeg = true"
              @mouseup="isJoggingNeg = false"
              @mouseleave="isJoggingNeg = false"
            >
              <ChevronLeft class="w-4 h-4" />
            </Button>

            <Button
              variant="outline"
              size="sm"
              class="h-8 w-12"
              :class="{ 'bg-green-100 border-green-500 text-green-700': index === selectedJointIndex && isJoggingPos }"
              @mousedown="isJoggingPos = true"
              @mouseup="isJoggingPos = false"
              @mouseleave="isJoggingPos = false"
            >
              <ChevronRight class="w-4 h-4" />
            </Button>
          </div>
        </div>
      </div>

      <div class="mt-2 space-y-1">
        <div class="flex justify-between text-xs text-muted-foreground">
          <span>Jog Speed</span>
          <span>{{ jogSpeed }}%</span>
        </div>
        <input
          type="range"
          v-model="jogSpeed"
          class="w-full h-2 bg-secondary rounded-lg appearance-none cursor-pointer accent-primary"
        />
      </div>

    </CardContent>
  </Card>
</template>
