<script setup lang="ts">
import { ref, watch, nextTick } from 'vue'
import { Trash2, Server } from 'lucide-vue-next'

// Shared UI
import { Card, CardContent, CardHeader, CardTitle } from '@/shared/ui/card'
import { Button } from '@/shared/ui/button'

// Entity Logic & UI
import { useSerialLogs } from '@/entities/log/model/useSerialLogs' // Assuming path based on context
import LogLine from '@/entities/log/ui/LogLine.vue'

// --- Logic ---
const { logs, clearLogs } = useSerialLogs()
const logContainer = ref<HTMLDivElement | null>(null)

// Auto-scroll Logic
// We watch the 'logs' array for changes.
watch(logs, async () => {
  if (!logContainer.value) return

  // 1. Check if user was already at the bottom (before the new log arrived)
  // We use a 20px threshold to be forgiving.
  const { scrollHeight, clientHeight, scrollTop } = logContainer.value
  const isAtBottom = scrollHeight - clientHeight <= scrollTop + 50

  // 2. Wait for DOM update
  await nextTick()

  // 3. Scroll to bottom if they were already there
  if (isAtBottom) {
    logContainer.value.scrollTop = logContainer.value.scrollHeight
  }
}, { deep: true })
</script>

<template>
  <Card class="flex flex-col bg-card h-full">
    <CardHeader class="flex flex-row items-center justify-between pb-2 border-b">
      <CardTitle class="text-lg font-semibold flex items-center gap-2">
        <Server class="w-5 h-5" />
        Serial Logs
      </CardTitle>

      <Button variant="ghost" size="sm" @click="clearLogs">
        <Trash2 class="w-4 h-4 mr-2" />
        Clear
      </Button>
    </CardHeader>

    <CardContent class="flex-grow overflow-hidden p-0 relative">
      <div
        ref="logContainer"
        class="h-full overflow-y-auto p-4 scroll-smooth"
      >
        <div v-if="logs.length === 0" class="flex h-full items-center justify-center text-muted-foreground italic">
          Waiting for serial logs...
        </div>

        <LogLine
          v-for="(log, index) in logs"
          :key="index"
          :log="log"
        />
      </div>
    </CardContent>
  </Card>
</template>
