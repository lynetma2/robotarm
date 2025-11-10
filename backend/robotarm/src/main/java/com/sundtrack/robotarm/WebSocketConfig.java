package com.sundtrack.robotarm;

import org.springframework.context.annotation.Configuration;
import org.springframework.messaging.simp.config.MessageBrokerRegistry;
import org.springframework.web.socket.config.annotation.EnableWebSocketMessageBroker;
import org.springframework.web.socket.config.annotation.StompEndpointRegistry;
import org.springframework.web.socket.config.annotation.WebSocketMessageBrokerConfigurer;

@Configuration
@EnableWebSocketMessageBroker
public class WebSocketConfig implements WebSocketMessageBrokerConfigurer {

    @Override
    public void configureMessageBroker(MessageBrokerRegistry config) {
        // Enable a simple in-memory message broker
        // Messages sent to destinations prefixed with "/topic" will be
        // broadcast to all connected clients.
        config.enableSimpleBroker("/topic");

        // We don't have messages coming *from* clients to the server
        // in this example, but if we did, they would be prefixed with "/app".
        config.setApplicationDestinationPrefixes("/app");
    }

    @Override
    public void registerStompEndpoints(StompEndpointRegistry registry) {
        // Register the "/serial-websocket" endpoint.
        // This is the URL clients will connect to.
        // withSockJS() provides a fallback for browsers that don't support WebSocket.
        registry.addEndpoint("/serial-websocket").withSockJS();
    }
}
