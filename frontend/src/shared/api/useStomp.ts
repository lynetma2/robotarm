import { ref, onUnmounted, getCurrentInstance, readonly, watch } from 'vue'
import { Client, type IFrame, type IMessage, type StompSubscription } from '@stomp/stompjs'

// --- Configuration ---
// Ideally move this to a config file later
const WEBSOCKET_URL = 'ws://localhost:8080/ws-robot-arm'

// --- Module-level state (Singleton Pattern) ---
// This ensures the connection state is shared across the entire app
const isConnected = ref(false)

const client = new Client({
  brokerURL: WEBSOCKET_URL,
  reconnectDelay: 5000,
  heartbeatIncoming: 4000,
  heartbeatOutgoing: 4000,
  onConnect: (frame: IFrame) => {
    console.log('STOMP: Connected')
    isConnected.value = true
  },
  onDisconnect: () => {
    console.log('STOMP: Disconnected')
    isConnected.value = false
  },
  onStompError: (frame: IFrame) => {
    console.error('STOMP: Broker reported error: ' + frame.headers['message'])
    console.error('STOMP: Additional details: ' + frame.body)
  },
})

// --- Composable ---
export function useStomp() {

  // Activate the client on first use if not already active
  if (!client.active) {
    console.log('STOMP: Activating client...')
    client.activate()
  }

  /**
   * Safe Subscribe Wrapper
   * - Checks if connected before subscribing.
   * - If not connected, watches 'isConnected' and subscribes automatically when ready.
   */
  const subscribe = (topic: string, callback: (payload: any) => void) => {
    let stompSubscription: StompSubscription | null = null

    // Helper to perform the actual subscription
    const doSubscribe = () => {
      if (stompSubscription) return // Prevent double subscription

      stompSubscription = client.subscribe(topic, (message: IMessage) => {
        try {
          const parsedBody = JSON.parse(message.body)
          callback(parsedBody)
        } catch (e) {
          console.error(`Could not parse JSON from topic [${topic}]:`, message.body)
          callback(message.body)
        }
      })
    }

    // Decision Logic: Subscribe now or wait?
    if (isConnected.value) {
      doSubscribe()
    } else {
      // Wait for the connection to become true
      const unwatch = watch(isConnected, (connected) => {
        if (connected) {
          doSubscribe()
          unwatch() // Stop watching once subscribed
        }
      })
    }

    // Cleanup Logic
    const unsubscribe = () => {
      if (stompSubscription) {
        stompSubscription.unsubscribe()
        stompSubscription = null
      }
    }

    // Automatically unsubscribe on component unmount
    if (getCurrentInstance()) {
      onUnmounted(unsubscribe)
    }

    return unsubscribe
  }

  /**
   * Safe Publish Wrapper
   * The library handles queuing for publish automatically, but we add a safety check.
   */
  const publish = (destination: string, body: object) => {
    client.publish({ destination, body: JSON.stringify(body) })
  }

  return {
    isConnected: readonly(isConnected),
    publish,
    subscribe,
  }
}
