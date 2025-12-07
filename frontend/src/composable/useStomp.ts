import { ref, onUnmounted, getCurrentInstance, readonly } from 'vue'
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
   * - The STOMP client automatically waits for connection before subscribing.
   * - Automatically cleans up when the component unmounts.
   * @param topic The topic to subscribe to.
   * @param callback The function to execute with the message payload.
   * @returns A function to manually unsubscribe if needed.
   */
  const subscribe = (topic: string, callback: (payload: any) => void) => {
    // The client will queue this subscription until it is connected.
    const stompSubscription = client.subscribe(topic, (message: IMessage) => {
      try {
        const parsedBody = JSON.parse(message.body)
        callback(parsedBody)
      } catch (e) {
        console.error(`Could not parse JSON from topic [${topic}]:`, message.body)
        // Fallback to raw body if JSON parsing fails
        callback(message.body)
      }
    })

    // If called inside a component's setup, automatically unsubscribe on unmount.
    if (getCurrentInstance()) {
      onUnmounted(() => {
        stompSubscription.unsubscribe()
      })
    }

    // Return the unsubscribe function for manual cleanup if needed.
    return () => stompSubscription.unsubscribe()
  }

  /**
   * Safe Publish Wrapper
   * - The STOMP client automatically queues messages if not connected
   *   and sends them upon connection.
   */
  const publish = (destination: string, body: object) => {
    // This will be queued by the library if the client is not yet connected.
    client.publish({ destination, body: JSON.stringify(body) })
  }

  return {
    isConnected: readonly(isConnected),
    publish,
    subscribe,
  }
}
