<script setup lang="ts">
import { ScrollText, Box, Settings2, Terminal, PenTool } from 'lucide-vue-next'
import { useRouter } from 'vue-router'

// --- Shared UI ---
import { Button } from '@/shared/ui/button'
import { Card } from '@/shared/ui/card'
import { Sheet, SheetContent, SheetTrigger } from '@/shared/ui/sheet'

// --- Widgets ---
import ConnectionStatus from "@/widgets/connectionStatus/connectionStatus.vue"
import WaypointManager from "@/widgets/waypointManager/waypointManager.vue"
import SimulationView from "@/widgets/simulationView/simulationView.vue"
import JoggingCard from "@/widgets/jogging/joggingCard.vue"
import SerialLogViewer from "@/widgets/serialLogViewer/serialLogViewer.vue"

// --- Entities ---
import { useMachineStore } from '@/entities/esp32/model/store'

const machineStore = useMachineStore()
const router = useRouter()
</script>

<template>
  <div class="flex flex-col h-screen bg-background">
    <!-- Header Bar -->
    <header class="h-16 border-b bg-background/80 backdrop-blur-md px-4 lg:px-6 flex items-center justify-between sticky top-0 z-50 shadow-sm">
      <div class="flex items-center gap-3">
        <div class="h-9 w-9 bg-blue-600 rounded-lg shadow-sm flex items-center justify-center text-white">
          <Box class="h-5 w-5" />
        </div>
        <div class="flex flex-col">
          <h1 class="font-bold text-lg leading-none tracking-tight">Robot Arm</h1>
          <span class="text-[10px] font-semibold text-muted-foreground uppercase tracking-wider">Control Station</span>
        </div>
      </div>

      <div class="flex items-center gap-2">
        <!-- Edit Mode Button -->
        <Button variant="default" size="sm" class="gap-2 bg-zinc-900 text-white hover:bg-zinc-800 flex-shrink-0 shadow-sm active:scale-95 transition-transform" @click="router.push('/editor')">
          <PenTool class="w-4 h-4" />
          Edit Mode
        </Button>

        <div class="h-6 w-px bg-border mx-1"></div>
        <ConnectionStatus />
        <div class="h-6 w-px bg-border mx-1"></div>

        <Sheet>
          <SheetTrigger as-child>
            <Button variant="ghost" size="icon" class="text-muted-foreground hover:text-foreground transition-colors">
              <ScrollText class="h-5 w-5" />
            </Button>
          </SheetTrigger>
          <SheetContent side="bottom" class="h-[50vh] flex flex-col border-t-4 border-blue-500/20">
            <SerialLogViewer />
          </SheetContent>
        </Sheet>
      </div>
    </header>

    <!-- Main Workspace -->
    <main class="flex-1 p-4 lg:p-6 overflow-hidden h-[calc(100vh-64px)]">
      <div class="grid grid-cols-1 lg:grid-cols-12 gap-6 h-full">
        <!-- Left Panel: Simulation -->
        <div class="lg:col-span-8 h-full flex flex-col">
          <div class="flex-1 rounded-xl border bg-card shadow-sm overflow-hidden relative group ring-1 ring-black/5">
            <SimulationView />
            <div class="absolute top-4 left-4 bg-background/90 backdrop-blur px-3 py-1.5 rounded-md border shadow-sm flex items-center gap-2 pointer-events-none">
              <span class="relative flex h-2 w-2">
                <span class="animate-ping absolute inline-flex h-full w-full rounded-full bg-green-400 opacity-75"></span>
                <span class="relative inline-flex rounded-full h-2 w-2 bg-green-500"></span>
              </span>
              <span class="text-xs font-medium text-muted-foreground">Live View</span>
            </div>
          </div>
        </div>

        <!-- Right Panel: Controls -->
        <div class="lg:col-span-4 flex flex-col gap-5 h-full overflow-y-auto pr-1 pb-6">
          <div class="flex-shrink-0">
            <JoggingCard />
          </div>

          <!-- Minimized Waypoint View (Optional, or remove if moving fully to Editor) -->
          <div class="flex-shrink-0 opacity-80 hover:opacity-100 transition-opacity">
            <WaypointManager />
          </div>

          <Card class="p-4 border-dashed bg-muted/30 hover:bg-muted/50 transition-colors shadow-none">
            <div class="flex items-center gap-2 mb-3">
              <Settings2 class="w-4 h-4 text-muted-foreground" />
              <h3 class="font-semibold text-xs text-muted-foreground uppercase tracking-wider">System</h3>
            </div>
            <div class="grid grid-cols-1 gap-2">
              <Button
                size="sm"
                variant="outline"
                class="w-full justify-start bg-background hover:bg-accent text-muted-foreground hover:text-foreground"
                @click="machineStore.toggleSerial()"
              >
                <Terminal class="w-3.5 h-3.5 mr-2" />
                Toggle Serial Connection
              </Button>
            </div>
          </Card>
        </div>
      </div>
    </main>
  </div>
</template>