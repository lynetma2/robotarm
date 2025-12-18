<script setup lang="ts">
import { ref, computed } from 'vue'
import { Card, CardContent, CardHeader, CardTitle } from '@/shared/ui/card'
import { PlayCircle, StopCircle } from 'lucide-vue-next'
import { Button } from '@/shared/ui/button'

// Import Logic & Data
import { useThreeScene } from './model/useThreeScene'
import { useRobotCoordinates } from '@/entities/robot/useRobotCoordinates'
import { useSimulationStore } from '@/entities/simulation/model/store'
import CoordinatesOverlay from '@/entities/robot/ui/CoordinatesOverlay.vue'

// 1. Setup Data
const { coordinates: liveCoordinates } = useRobotCoordinates()
const simulationStore = useSimulationStore()

const displayCoordinates = computed(() => {
  if (simulationStore.isSimulating) {
    // Transform Pose object to Coordinate[] format for the overlay
    const p = simulationStore.currentPose
    return [
      { label: 'X', value: p.x.toFixed(2) },
      { label: 'Y', value: p.y.toFixed(2) },
      { label: 'Z', value: p.z.toFixed(2) },
      { label: 'R', value: p.roll.toFixed(2) },
      { label: 'P', value: p.pitch.toFixed(2) },
      { label: 'Y', value: p.yaw.toFixed(2) },
    ]
  }
  return liveCoordinates.value
})

// 2. Setup Scene
const containerRef = ref<HTMLDivElement | null>(null)
useThreeScene(containerRef)
</script>

<template>
  <Card class="flex flex-col h-full bg-card text-card-foreground shadow-sm overflow-hidden">
    <CardHeader class="pb-2 border-b">
      <CardTitle class="text-xl font-semibold flex justify-between items-center">
        <span>3D View & Simulation</span>
        
        <div class="flex items-center gap-2">
          <span v-if="simulationStore.isSimulating" class="text-xs font-bold text-blue-500 animate-pulse flex items-center gap-1">
            <PlayCircle class="w-3 h-3" /> SIMULATION
          </span>
          <span v-else class="text-xs font-normal text-muted-foreground bg-secondary px-2 py-1 rounded">
            Interactive
          </span>
        </div>
      </CardTitle>
    </CardHeader>

    <CardContent class="flex-grow relative p-0 overflow-hidden">
      <div ref="containerRef" class="w-full h-full cursor-move outline-none" />

      <!-- Simulation Overlay Controls -->
      <div v-if="simulationStore.isSimulating" class="absolute bottom-4 left-1/2 -translate-x-1/2 flex flex-col items-center gap-2 z-10 w-3/4 max-w-sm">
        <div class="w-full bg-black/20 h-1.5 rounded-full overflow-hidden backdrop-blur-sm">
          <div class="h-full bg-blue-500 transition-all duration-75 ease-linear" :style="{ width: `${simulationStore.progress}%` }"></div>
        </div>
        <Button size="sm" variant="destructive" class="h-7 text-xs shadow-lg" @click="simulationStore.stopSimulation()">
          <StopCircle class="w-3 h-3 mr-1" /> Stop Simulation
        </Button>
      </div>

      <!-- Pass the computed coordinates (Live OR Sim) -->
      <CoordinatesOverlay :coordinates="displayCoordinates" />
    </CardContent>
  </Card>
</template>
