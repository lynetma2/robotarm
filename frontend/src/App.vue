<script setup lang="ts">
import { ref } from 'vue'
import { Button } from '@/components/ui/button'
import "./style.css"
import MotorCard from "@/components/motorConfiguration/motorCard.vue";
import ConnectionStatus from "@/components/connectionStatus/connectionStatus.vue";
import WaypointManager from "@/components/waypointManager/waypointManager.vue";

// --- In your real app, these values would come from ---
// --- your WebSocket and Serial connection logic. ---
const wsConnected = ref(true)
const serialConnected = ref(true)
</script>

<template>
  <div class="min-h-screen w-full p-10">
    <ConnectionStatus
        :is-ws-connected="wsConnected"
        :is-serial-connected="serialConnected"
    />

    <MotorCard />
    <WaypointManager />

    <div class="mt-8 space-x-2 border-t pt-4">
      <h3 class="font-semibold">Demo Controls</h3>
      <Button
          variant="outline"
          @click="wsConnected = !wsConnected"
      >
        Toggle WebSocket
      </Button>
      <Button
          variant="outline"
          @click="serialConnected = !serialConnected"
      >
        Toggle Serial
      </Button>
    </div>
  </div>
</template>
