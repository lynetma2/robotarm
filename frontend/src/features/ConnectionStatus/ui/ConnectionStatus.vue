<script setup lang="ts">
// import { useStomp } from "@/shared/api/useStomp"

// const { isConnected } = useStomp()
const isConnected = true // Placeholder
</script>

<template>
  <div class="flex items-center gap-2">
    <div :class="['h-2 w-2 rounded-full', isConnected ? 'bg-green-500' : 'bg-red-500']" />
    <span class="text-sm font-medium">{{ isConnected ? 'Connected' : 'Disconnected' }}</span>
  </div>
</template>
