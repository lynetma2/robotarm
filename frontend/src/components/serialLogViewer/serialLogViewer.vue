<script setup lang="ts">
import { ref, watch } from 'vue'
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Trash2, Server } from 'lucide-vue-next'
import { useSerialLogs, type LogMessage } from '@/composable/useSerialLogs'

// --- Composables ---
const { logs, clearLogs } = useSerialLogs()

// --- Refs ---
const logContainer = ref<HTMLDivElement | null>(null)

// --- Auto-scrolling Logic ---
watch(
  logs,
  () => {
    // Use nextTick to wait for the DOM to update before scrolling
    if (logContainer.value) {
      // A small threshold to prevent auto-scrolling if the user has scrolled up
      const isScrolledToBottom =
        logContainer.value.scrollHeight - logContainer.value.clientHeight <= logContainer.value.scrollTop + 20

      if (isScrolledToBottom) {
        logContainer.value.scrollTop = logContainer.value.scrollHeight
      }
    }
  },
  { deep: true },
)

// --- Helpers ---
const getLevelClass = (level: LogMessage['level']) => {
  switch (level) {
    case 'ERROR':
      return 'text-red-400'
    case 'WARN':
      return 'text-yellow-400'
    case 'INFO':
      return 'text-blue-400'
    case 'DEBUG':
      return 'text-purple-400'
    default:
      return 'text-gray-400'
  }
}
</script>

<template>
  <Card class="flex flex-col bg-card h-full">
    <CardHeader class="flex flex-row items-center justify-between pb-2">
      <CardTitle class="text-lg font-semibold flex items-center gap-2">
        <Server class="w-5 h-5" />
        Serial Logs
      </CardTitle>
      <Button variant="ghost" size="sm" @click="clearLogs">
        <Trash2 class="w-4 h-4 mr-2" />
        Clear
      </Button>
    </CardHeader>

    <CardContent class="flex-grow overflow-hidden p-0">
      <div ref="logContainer" class="h-full overflow-y-auto p-4 font-mono text-xs">
        <div v-for="(log, index) in logs" :key="index" class="flex gap-3">
          <span class="text-gray-500">{{ log.timestamp }}</span>
          <span class="font-bold" :class="getLevelClass(log.level)">[{{ log.level }}]</span>
          <span class="whitespace-pre-wrap break-all">{{ log.message }}</span>
        </div>
        <div v-if="logs.length === 0" class="text-center text-muted-foreground italic mt-4">
          Waiting for serial logs...
        </div>
      </div>
    </CardContent>
  </Card>
</template>