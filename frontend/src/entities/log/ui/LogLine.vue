<script setup lang="ts">
import type { LogMessage } from '../types'

const props = defineProps<{
  log: LogMessage
}>()

// Presentation Logic: Maps log levels to Tailwind classes
const levelColor = (level: LogMessage['level']) => {
  switch (level) {
    case 'ERROR': return 'text-red-400'
    case 'WARN':  return 'text-yellow-400'
    case 'INFO':  return 'text-blue-400'
    case 'DEBUG': return 'text-purple-400'
    default:      return 'text-gray-400'
  }
}
</script>

<template>
  <div class="flex gap-3 font-mono text-xs hover:bg-white/5 p-0.5 rounded">
    <span class="text-gray-500 shrink-0">{{ log.timestamp }}</span>
    <span class="font-bold shrink-0 w-[50px]" :class="levelColor(log.level)">
      [{{ log.level }}]
    </span>
    <span class="whitespace-pre-wrap break-all">{{ log.message }}</span>
  </div>
</template>
