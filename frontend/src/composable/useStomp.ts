import { ref, onUnmounted, readonly } from 'vue'
import { Client, type IFrame, type IMessage } from '@stomp/stompjs'

// --- Configuration ---
const WEBSOCKET_URL = 'ws://localhost:8080/ws' // Replace with your backend WebSocket endpoint

// --- Module-level state (Singleton Pattern) ---
// By creating the client and state outside the composable function, we ensure
// that only one instance is shared across the entire application.
const isConnected = ref(false)

const client = new Client({
  brokerURL: WEBSOCKET_URL,
  reconnectDelay: 5000, // Automatically reconnect every 5 seconds
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

  // Activate the client on first use
  if (!client.active) {
    console.log('STOMP: Activating client...')
    client.activate()
  }

  /**
   * Subscribes to a STOMP topic and executes a callback on message.
   * @param topic The destination topic (e.g., '/topic/positions').
   * @param callback The function to call with the message body.
   * @returns A function to unsubscribe.
   */
  const subscribe = (topic: string, callback: (payload: any) => void) => {
    const subscription = client.subscribe(topic, (message: IMessage) => {
      try {
        const parsedBody = JSON.parse(message.body)
        callback(parsedBody)
      } catch (e) {
        console.error(`Could not parse JSON from topic [${topic}]:`, message.body)
        callback(message.body) // Fallback to raw body
      }
    })
    return () => subscription.unsubscribe()
  }

  /**
   * Sends a message to a STOMP destination.
   * @param destination The destination (e.g., '/app/jog').
   * @param body The message payload, which will be stringified.
   */
  const publish = (destination: string, body: object) => {
    if (!isConnected.value) {
      console.warn('STOMP: Cannot publish. Client not connected.')
      return
    }
    client.publish({ destination, body: JSON.stringify(body) })
  }

  // Ensure client is deactivated when the app is completely torn down
  onUnmounted(() => {
    client.deactivate()
  })

  return {
    isConnected: readonly(isConnected),
    publish,
    subscribe,
  }
}