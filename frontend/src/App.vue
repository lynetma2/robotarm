<script setup lang="ts">
import { ref } from 'vue'
import { Button } from '@/components/ui/button'
import { Card } from '@/components/ui/card'
import "./style.css"

// Components
import MotorCard from "@/components/motorConfiguration/motorCard.vue";
import ConnectionStatus from "@/components/connectionStatus/connectionStatus.vue";
import WaypointManager from "@/components/waypointManager/waypointManager.vue";
import SimulationView from "@/components/simulationView/simulationView.vue";
import JoggingCard from "@/components/jogging/joggingCard.vue"; // Import new card
import SerialLogViewer from "@/components/serialLogViewer/serialLogViewer.vue";
import { ScrollText } from 'lucide-vue-next'
import {
  Sheet,
  SheetContent,
  SheetTrigger,
} from '@/components/ui/sheet'

// Composables
import { useGamepad } from '@/composable/useGamepad';
import { useStomp } from '@/composable/useStomp';

const serialConnected = ref(true)

// Use the gamepad hook here to pass status to the header
const { isConnected: isControllerConnected } = useGamepad()
const { isConnected: isWsConnected } = useStomp() // Use our new STOMP composable
</script>

<template>
  <div class="min-h-screen w-full bg-background p-6">

    <!-- Top-right icon buttons -->
    <div class="fixed top-4 right-4 z-[999] flex items-center gap-2">
      <Sheet>
        <SheetTrigger as-child>
          <Button variant="outline" size="icon" class="rounded-full">
            <ScrollText class="h-5 w-5" />
          </Button>
        </SheetTrigger>
        <SheetContent side="bottom" class="h-[40%] flex flex-col">
          <SerialLogViewer />
        </SheetContent>
      </Sheet>
      <ConnectionStatus
        :is-serial-connected="serialConnected"
        :is-controller-connected="isControllerConnected"
        :is-ws-connected="isWsConnected"
      />
    </div>

    <div class="grid grid-cols-1 lg:grid-cols-12 gap-6 h-[calc(100vh-140px)]">

      <div class="lg:col-span-8 h-full">
        <SimulationView />
      </div>

      <div class="lg:col-span-4 flex flex-col gap-6 h-full overflow-y-auto pr-1">

        <div class="flex-shrink-0">
          <JoggingCard />
        </div>

        <div class="flex-shrink-0">
          <WaypointManager />
        </div>

        <Card class="p-4 border-dashed bg-muted/30">
          <h3 class="font-semibold text-xs mb-3 text-muted-foreground uppercase">Dev Controls</h3>
          <div class="flex flex-wrap gap-2">
            <Button size="sm" variant="outline" @click="serialConnected = !serialConnected">
              Toggle Serial
            </Button>
          </div>
        </Card>
      </div>

    </div>
  </div>
</template>
