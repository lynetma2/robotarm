<script setup lang="ts">
import { ScrollText } from 'lucide-vue-next'

// --- Shared UI ---
import { Button } from '@/shared/ui/button'
import { Card } from '@/shared/ui/card'
import {
  Sheet,
  SheetContent,
  SheetTrigger,
} from '@/shared/ui/sheet'
import "./style.css"

// --- FSD Widgets ---
// (Ensure your folders match these paths, or adjust to where you moved them)
import ConnectionStatus from "@/widgets/connectionStatus/connectionStatus.vue"
import WaypointManager from "@/widgets/waypointManager/waypointManager.vue"
import SimulationView from "@/widgets/simulationView/simulationView.vue"
import JoggingCard from "@/widgets/jogging/joggingCard.vue"
import SerialLogViewer from "@/widgets/serialLogViewer/serialLogViewer.vue"

// --- Entities ---
import { useSerialLogs } from '@/entities/log/model/useSerialLogs'
import { useMachineStore } from '@/entities/esp32/model/store'

// 1. Eager Initialization
// Start the log subscription immediately. It persists even if the Sheet is closed.
useSerialLogs()

// 2. Machine Store
// We use this to toggle serial connection from the Dev Controls
const machineStore = useMachineStore()
</script>

<template>
  <div class="min-h-screen w-full bg-background p-6">

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

      <ConnectionStatus />
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
            <Button
              size="sm"
              variant="outline"
              @click="machineStore.toggleSerial()"
            >
              Toggle Serial
            </Button>
          </div>
        </Card>
      </div>

    </div>
  </div>
</template>
