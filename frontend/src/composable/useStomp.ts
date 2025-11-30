import { ref, watch, onUnmounted, getCurrentInstance, readonly } from 'vue'
import { Client, type IFrame, type IMessage } from '@stomp/stompjs'

// --- Configuration ---
const WEBSOCKET_URL = 'ws://localhost:8080/ws-robot-arm'

// --- Module-level state (Singleton Pattern) ---
const isConnected = ref(false)

const client = new Client({
  brokerURL: WEBSOCKET_URL,
  reconnectDelay: 5000,
  heartbeatIncoming: 4000,
  heartbeatOutgoing: 4000,
  onConnect: () => {
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
   * - Waits for connection before subscribing (prevents crashes).
   * - Automatically cleans up when the component unmounts.
   */
  const subscribe = (topic: string, callback: (payload: any) => void) => {
    let stompSubscription: any = null

    // 1. Define the actual subscription logic
    const doSubscribe = () => {
      // Only subscribe if connected and not already subscribed
      if (isConnected.value && !stompSubscription) {
        try {
          stompSubscription = client.subscribe(topic, (message: IMessage) => {
            try {
              const parsedBody = JSON.parse(message.body)
              callback(parsedBody)
            } catch (e) {
              console.error(`Could not parse JSON from topic [${topic}]:`, message.body)
              callback(message.body)
            }
          })
        } catch (err) {
          console.error(`Failed to subscribe to ${topic}`, err)
        }
      }
    }

    // 2. Watch connection state
    // 'immediate: true' ensures we try immediately if already connected
    const stopWatcher = watch(isConnected, (connected) => {
      if (connected) {
        doSubscribe()
      } else {
        // Connection lost: StompJS handles internal cleanup,
        // but we clear our reference so we can resubscribe on reconnect
        stompSubscription = null
      }
    }, { immediate: true })

    // 3. Define Cleanup Function
    const cleanup = () => {
      stopWatcher() // Stop watching isConnected
      if (stompSubscription) {
        try {
          stompSubscription.unsubscribe()
        } catch (e) { /* ignore */ }
        stompSubscription = null
      }
    }

    // 4. AUTO-MAGIC: If called inside a component, auto-register cleanup
    if (getCurrentInstance()) {
      onUnmounted(cleanup)
    }

    // Return cleanup in case manual unsub is needed
    return cleanup
  }

  /**
   * Safe Publish Wrapper
   * - Sends immediately if connected.
   * - Queues the message if disconnected and sends it once connected.
   */
  const publish = (destination: string, body: object) => {
    const doSend = () => {
      try {
        client.publish({ destination, body: JSON.stringify(body) })
      } catch (err) {
        console.error(`STOMP: Failed to publish to ${destination}`, err)
      }
    }

    if (isConnected.value) {
      doSend()
    } else {
      // If not connected, wait for connection
      // This creates a one-time watcher that executes the publish and then destroys itself
      const stopWatcher = watch(isConnected, (connected) => {
        if (connected) {
          doSend()
          stopWatcher() // Unwatch immediately after sending to prevent duplicate sends
        }
      })
    }
  }

  return {
    isConnected: readonly(isConnected),
    publish,
    subscribe,
  }
}
