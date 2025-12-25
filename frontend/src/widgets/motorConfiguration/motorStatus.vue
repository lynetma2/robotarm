<script setup lang="ts">
import { ref } from 'vue'

// Define props to receive data from the parent
defineProps<{
  motorId: number
}>()

// Placeholder for your real-time data
const position = ref(120.5)
const speed = ref(50.0)
const temp = ref(42.5)
const load = ref(35)
const emf = ref(35)
</script>

<template>
  <div class="space-y-4 p-4 rounded-lg border">
    <h3 class="font-semibold">Live Data (Motor {{ motorId }})</h3>

    <div class="flex justify-between">
      <span class="text-muted-foreground">Current Position:</span>
      <span class="font-mono">{{ position }} mm</span>
    </div>

    <div class="flex justify-between">
      <span class="text-muted-foreground">Current Speed:</span>
      <span class="font-mono">{{ speed }} mm/s</span>
    </div>

    <div class="flex justify-between">
      <span class="text-muted-foreground">Driver Temp:</span>
      <span class="font-mono">{{ temp }}°C</span>
    </div>

    <div class="flex justify-between">
      <span class="text-muted-foreground">Current Load:</span>
      <span class="font-mono">{{ load }}</span>
    </div>

    <div class="flex justify-between">
      <span class="text-muted-foreground">Current Back-EMF:</span>
      <span class="font-mono">{{ emf }}</span>
    </div>
  </div>
</template>
