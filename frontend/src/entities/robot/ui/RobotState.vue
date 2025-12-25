<script setup lang="ts">
// type RobotState = 'IDLE' | 'RUNNING' | 'ERROR'
const state = 'IDLE' // Placeholder

const stateColors = {
  IDLE: 'bg-secondary text-secondary-foreground hover:bg-secondary/80',
  RUNNING: 'bg-primary text-primary-foreground hover:bg-primary/80',
  ERROR: 'bg-destructive text-destructive-foreground hover:bg-destructive/80'
} as const
</script>

<template>
  <div :class="['inline-flex items-center rounded-full border px-2.5 py-0.5 text-xs font-semibold transition-colors focus:outline-none focus:ring-2 focus:ring-ring focus:ring-offset-2', stateColors[state]]">
    {{ state }}
  </div>
</template>
