import { ref, readonly } from 'vue'
import { useStomp } from '@/composable/useStomp'

// --- Type Definition for a Log Message ---
export interface LogMessage {
  timestamp: string
  level: 'INFO' | 'WARN' | 'ERROR' | 'DEBUG' | 'RAW'
  message: string
}

// --- Configuration ---
const MAX_LOGS = 500 // Keep a maximum of 500 log lines in memory
const LOG_TOPIC = '/topic/serial/logs'

// --- Module-level state (Singleton Pattern) ---
const logs = ref<LogMessage[]>([])
let isSubscribed = false

// --- Composable ---
export function useSerialLogs() {
  // Ensure we only subscribe once for the whole application
  if (!isSubscribed) {
    const { subscribe } = useStomp()

    subscribe(LOG_TOPIC, (log: LogMessage) => {
      // By replacing the array instead of mutating it, we ensure Vue's
      // reactivity system reliably detects the change when a component re-mounts.
      logs.value = [...logs.value, log]
      console.log("log recieved: ", log)


      // If we've exceeded the max number of logs, remove the oldest one
      if (logs.value.length > MAX_LOGS) {
        logs.value.shift()
      }
    })

    isSubscribed = true
  }

  const clearLogs = () => {
    logs.value = []
  }

  // Expose the logs as a readonly ref to prevent components from modifying it directly
  return { logs: readonly(logs), clearLogs }
}
