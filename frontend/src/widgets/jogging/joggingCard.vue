<script setup lang="ts">
import { ChevronRight, ChevronLeft, Gamepad2 } from 'lucide-vue-next'
// Shared UI (Visuals)
import { Card, CardContent, CardHeader, CardTitle } from '@/shared/ui/card'
import { Button } from '@/shared/ui/button'

// Entities (Data Constants)
import { ROBOT_JOINTS } from '@/entities/robot/constants'

// Feature (Logic & Behavior)
import { useJoggingController } from '@/features/manual-jog/useJoggingController'

// Initialize the Feature Hook
const {
  isControllerConnected,
  selectedJointIndex,
  isJoggingPos,
  isJoggingNeg,
  jogSpeed,
  selectJoint,
  jogPositive,
  jogNegative,
  setJoggingPos,
  setJoggingNeg
} = useJoggingController()
</script>

<template>
  <Card class="flex flex-col bg-card">
    <CardHeader class="pb-2 flex flex-row items-center justify-between">
      <CardTitle class="text-lg font-semibold">Manual Jogging</CardTitle>

      <div
        v-if="isControllerConnected"
        class="flex items-center text-xs text-purple-500 font-medium animate-pulse"
      >
        <Gamepad2 class="w-4 h-4 mr-1"/> PS5 Connected
      </div>
    </CardHeader>

    <CardContent class="grid gap-3">
      <div
        v-if="isControllerConnected"
        class="text-xs bg-secondary/50 p-2 rounded text-muted-foreground mb-1"
      >
        <span class="font-bold">D-Pad:</span> Select Joint &nbsp;|&nbsp;
        <span class="font-bold">L2/R2:</span> Jog - / +
      </div>

      <div class="flex flex-col gap-2">
        <div
          v-for="(joint, index) in ROBOT_JOINTS"
          :key="joint.id"
          class="flex items-center justify-between p-2 rounded border transition-all cursor-pointer select-none"
          :class="{
            'border-primary bg-primary/5 ring-1 ring-primary/20': index === selectedJointIndex && isControllerConnected,
            'border-border hover:bg-muted/50': index !== selectedJointIndex
          }"
          @click="selectJoint(index)"
        >
          <span class="text-sm font-medium w-24">{{ joint.name }}</span>

          <div class="flex items-center gap-2">
            <Button
              variant="outline"
              size="sm"
              class="h-8 w-12"
              :class="{ 'bg-red-100 border-red-500 text-red-700': index === selectedJointIndex && isJoggingNeg }"
              @mousedown="jogNegative(); setJoggingNeg(true)"
              @mouseup="setJoggingNeg(false)"
              @mouseleave="setJoggingNeg(false)"
            >
              <ChevronLeft class="w-4 h-4" />
            </Button>

            <Button
              variant="outline"
              size="sm"
              class="h-8 w-12"
              :class="{ 'bg-green-100 border-green-500 text-green-700': index === selectedJointIndex && isJoggingPos }"
              @mousedown="jogPositive(); setJoggingPos(true)"
              @mouseup="setJoggingPos(false)"
              @mouseleave="setJoggingPos(false)"
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
          min="0"
          max="100"
          class="w-full h-2 bg-secondary rounded-lg appearance-none cursor-pointer accent-primary"
        />
      </div>

    </CardContent>
  </Card>
</template>
