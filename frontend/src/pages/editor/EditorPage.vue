<script setup lang="ts">
import { ArrowLeft, Save, Layers } from 'lucide-vue-next'
import { useRouter } from 'vue-router'

import { Button } from '@/shared/ui/button'
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/shared/ui/tabs'

// Widgets
import WaypointManager from "@/widgets/waypointManager/waypointManager.vue"
import MotorCurrent from "@/widgets/motorConfiguration/motorCurrent.vue"
import SimulationView from "@/widgets/simulationView/simulationView.vue"

const router = useRouter()
</script>

<template>
  <div class="min-h-screen bg-background flex flex-col">
    
    <!-- Editor Header -->
    <header class="h-16 bg-white dark:bg-zinc-900 border-b px-6 flex items-center justify-between sticky top-0 z-40">
      <div class="flex items-center gap-4">
        <Button variant="ghost" size="icon" @click="router.back()">
          <ArrowLeft class="w-5 h-5" />
        </Button>
        <div>
          <h1 class="text-lg font-bold flex items-center gap-2">
            <Layers class="w-5 h-5 text-blue-600" />
            Editor Mode
          </h1>
          <p class="text-xs text-muted-foreground">Configuration & Sequence Management</p>
        </div>
      </div>
      
      <div class="flex items-center gap-2">
        <Button variant="outline" size="sm">Discard</Button>
        <Button size="sm" class="bg-blue-600 hover:bg-blue-700 text-white">
          <Save class="w-4 h-4 mr-2" />
          Save Changes
        </Button>
      </div>
    </header>

    <!-- Editor Content -->
    <main class="flex-1 p-6 overflow-hidden">
      <div class="grid grid-cols-1 lg:grid-cols-12 gap-6 h-[calc(100vh-120px)]">
        
        <!-- Left: Main Editor Area -->
        <div class="lg:col-span-8 h-full flex flex-col gap-4">
          <Tabs default-value="waypoints" class="h-full flex flex-col">
            <TabsList class="w-full justify-start border-b rounded-none bg-transparent p-0 mb-4">
              <TabsTrigger value="waypoints" class="data-[state=active]:border-b-2 data-[state=active]:border-blue-600 rounded-none px-4 pb-2">Waypoints</TabsTrigger>
              <TabsTrigger value="motors" class="data-[state=active]:border-b-2 data-[state=active]:border-blue-600 rounded-none px-4 pb-2">Motor Config</TabsTrigger>
            </TabsList>
            
            <TabsContent value="waypoints" class="flex-1 overflow-y-auto pr-2">
               <WaypointManager class="h-full" />
            </TabsContent>
            <TabsContent value="motors" class="flex-1 overflow-y-auto pr-2">
               <MotorCurrent />
            </TabsContent>
          </Tabs>
        </div>

        <!-- Right: Reference View -->
        <div class="lg:col-span-4 h-full flex flex-col gap-4">
          <div class="font-semibold text-sm text-muted-foreground uppercase tracking-wider">Visual Reference</div>
          <div class="flex-1 rounded-xl border bg-card overflow-hidden shadow-sm">
            <SimulationView />
          </div>
        </div>

      </div>
    </main>
  </div>
</template>