<script setup lang="ts">
import type { Coordinate } from '../types'

defineProps<{
  coordinates: Coordinate[]
}>()
</script>

<template>
  <div class="absolute bottom-6 left-1/2 -translate-x-1/2 bg-white/90 dark:bg-zinc-950/90 backdrop-blur border rounded-lg p-4 shadow-xl w-[90%] max-w-3xl pointer-events-none select-none">
    <h4 class="text-xs font-bold mb-3 text-muted-foreground uppercase tracking-wider flex items-center gap-2">
      <span class="w-2 h-2 rounded-full bg-green-500 animate-pulse"></span>
      Current coordinates
    </h4>
    <div class="grid grid-cols-6 gap-2 pointer-events-auto">
      <div
        v-for="(axis, index) in coordinates"
        :key="index"
        class="flex flex-col items-center justify-center p-2 bg-background/50 border rounded-md hover:bg-background hover:border-primary/50 transition-all shadow-sm"
      >
        <span class="text-[10px] uppercase text-muted-foreground font-semibold">{{ axis.label }}</span>
        <span class="font-mono font-medium text-lg tracking-tight">{{ axis.value }}</span>
      </div>
    </div>
  </div>
</template>
