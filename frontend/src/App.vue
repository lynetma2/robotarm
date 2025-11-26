<script setup lang="ts">
import { ref } from 'vue'
import { Button } from '@/components/ui/button'
import "./style.css"

// Components
import MotorCard from "@/components/motorConfiguration/motorCard.vue";
import ConnectionStatus from "@/components/connectionStatus/connectionStatus.vue";
import WaypointManager from "@/components/waypointManager/waypointManager.vue";
// Import the new biggest box component
import SimulationView from "@/components/simulationView/simulationView.vue";

const wsConnected = ref(true)
const serialConnected = ref(true)
</script>

<template>
  <div class="min-h-screen w-full bg-background p-6">

    <div class="mb-6">
      <ConnectionStatus
        :is-ws-connected="wsConnected"
        :is-serial-connected="serialConnected"
      />
    </div>

    <div class="grid grid-cols-1 lg:grid-cols-12 gap-6 h-[calc(100vh-140px)]">

      <div class="lg:col-span-8 h-full">
        <SimulationView />
      </div>

      <div class="lg:col-span-4 flex flex-col gap-6 h-full overflow-y-auto pr-1">
        <div class="flex-grow">
          <WaypointManager />
        </div>

        <div>
          <MotorCard />
        </div>

        <Card class="p-4 border-dashed">
          <h3 class="font-semibold text-sm mb-3">Dev Controls</h3>
          <div class="flex flex-wrap gap-2">
            <Button
              size="sm"
              variant="outline"
              @click="wsConnected = !wsConnected"
            >
              Toggle WS
            </Button>
            <Button
              size="sm"
              variant="outline"
              @click="serialConnected = !serialConnected"
            >
              Toggle Serial
            </Button>
          </div>
        </Card>
      </div>

    </div>
  </div>
</template>
