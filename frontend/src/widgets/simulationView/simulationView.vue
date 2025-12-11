<script setup lang="ts">
import { ref } from 'vue'
import { Card, CardContent, CardHeader, CardTitle } from '@/shared/ui/card'

// Import Logic & Data
import { useThreeScene } from './model/useThreeScene'
import { useRobotCoordinates } from '@/entities/robot/useRobotCoordinates'
import CoordinatesOverlay from '@/entities/robot/ui/CoordinatesOverlay.vue'

// 1. Setup Data
const { coordinates } = useRobotCoordinates()

// 2. Setup Scene
const containerRef = ref<HTMLDivElement | null>(null)
useThreeScene(containerRef)
</script>

<template>
  <Card class="flex flex-col h-full bg-card text-card-foreground shadow-sm overflow-hidden">
    <CardHeader class="pb-2 border-b">
      <CardTitle class="text-xl font-semibold flex justify-between items-center">
        <span>3D View & Simulation</span>
        <span class="text-xs font-normal text-muted-foreground bg-secondary px-2 py-1 rounded">
          Interactive
        </span>
      </CardTitle>
    </CardHeader>

    <CardContent class="flex-grow relative p-0 overflow-hidden">
      <div ref="containerRef" class="w-full h-full cursor-move outline-none" />

      <CoordinatesOverlay :coordinates="coordinates" />
    </CardContent>
  </Card>
</template>
