package com.sundtrack.robotarm.config;

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
        // The message broker will carry messages back to the client on destinations prefixed with "/topic"
        config.enableSimpleBroker("/topic");
        // This defines the prefix for messages that are bound for @MessageMapping-annotated methods.
        config.setApplicationDestinationPrefixes("/app");
    }

    @Override
    public void registerStompEndpoints(StompEndpointRegistry registry) {
        // This is the endpoint that clients will connect to.
        // withSockJS() is a fallback for browsers that don't support WebSockets.
        registry.addEndpoint("/ws-robot-arm").setAllowedOriginPatterns("*").withSockJS();
        registry.addEndpoint("/serial-websocket").withSockJS();
    }
}