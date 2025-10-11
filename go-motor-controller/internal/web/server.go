package web

import (
	"context"
	"log"
	"net/http"
	"robotarm/internal/comms"
	"sync"
	"time"

	"github.com/coder/websocket"
	"github.com/coder/websocket/wsjson"
)

// Hub manages all active WebSocket connections.
type Hub struct {
	clients  map[*websocket.Conn]bool
	mu       sync.Mutex
	channels comms.Channels
}

func newHub(channels comms.Channels) *Hub {
	return &Hub{
		clients:  make(map[*websocket.Conn]bool),
		channels: channels,
	}
}

// StartServer starts the web server and WebSocket hub.
func StartServer(addr string, channels comms.Channels) {
	hub := newHub(channels)
	go hub.runDiagnosticsBroadcaster()

	http.Handle("/", http.FileServer(http.Dir("./internal/web/static")))
	http.HandleFunc("/ws", hub.handleWebSocket)

	log.Printf("Starting web server on %s", addr)
	if err := http.ListenAndServe(addr, nil); err != nil {
		log.Fatalf("FATAL: Web server failed: %v", err)
	}
}

// handleWebSocket upgrades connections and listens for incoming commands.
func (h *Hub) handleWebSocket(w http.ResponseWriter, r *http.Request) {
	// websocket.Accept handles the upgrade.
	c, err := websocket.Accept(w, r, nil)
	if err != nil {
		log.Printf("Error upgrading connection: %v", err)
		return
	}
	// The final argument is the status message on close.
	defer c.Close(websocket.StatusInternalError, "the sky is falling")

	h.addClient(c)
	defer h.removeClient(c)

	log.Println("Client connected")

	// The request's context is used to cancel the read loop if the client disconnects.
	ctx := r.Context()
	for {
		var cmd comms.Command
		// Use wsjson for convenience, or c.Read for raw bytes.
		// This read is context-aware. If the client disconnects, the context is
		// cancelled and this function returns an error, breaking the loop.
		if err := wsjson.Read(ctx, c, &cmd); err != nil {
			log.Printf("Client disconnected: %v", err)
			break
		}
		// Send the command to the motor manager's channel
		h.channels.CommandChan <- cmd
	}

	c.Close(websocket.StatusNormalClosure, "")
}

// runDiagnosticsBroadcaster listens for diagnostics and sends them to all clients.
func (h *Hub) runDiagnosticsBroadcaster() {
	for diag := range h.channels.DiagnosticsChan {
		h.mu.Lock()
		for client := range h.clients {
			// Create a context with a timeout for writing.
			ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)

			if err := wsjson.Write(ctx, client, diag); err != nil {
				log.Printf("Error writing to client: %v", err)
			}
			cancel() // Always call cancel to free context resources.
		}
		h.mu.Unlock()
	}
}

func (h *Hub) addClient(conn *websocket.Conn) {
	h.mu.Lock()
	h.clients[conn] = true
	h.mu.Unlock()
}

func (h *Hub) removeClient(conn *websocket.Conn) {
	h.mu.Lock()
	delete(h.clients, conn)
	h.mu.Unlock()
}
