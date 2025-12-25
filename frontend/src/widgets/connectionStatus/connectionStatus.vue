<script setup lang="ts">
import { computed } from 'vue'
import { Wifi, Usb, CheckCircle2, XCircle, Gamepad2 } from 'lucide-vue-next'

// --- FSD Layer Imports ---
// 1. Shared UI (Visuals)
import { Popover, PopoverContent, PopoverTrigger } from '@/shared/ui/popover'
import { Button } from '@/shared/ui/button'

// 2. Shared API/Lib (Data Sources)
// We consume the composables directly here.
import { useStomp } from '@/shared/api/useStomp'
import { useGamepad } from '@/shared/lib/useGamepad'

// 3. Entities (Global State)
// Assuming you create a store for the machine state (see below)
// import { useMachineStore } from '@/entities/machine/model/store'

const { isConnected: isWsConnected } = useStomp()
const { isConnected: isControllerConnected } = useGamepad()

// TODO: Replace this ref with: const { isSerialConnected } = useMachineStore()
// For now, we defaults to false or you can temporarily keep it as a prop if strictly needed,
// but for "perfect" FSD, this state belongs in a Store, not a prop.
import { ref } from 'vue'
const isSerialConnected = ref(true)

// A computed property to quickly know if everything is OK
const isAllGood = computed(() =>
  isWsConnected.value && isSerialConnected.value
)
</script>

<template>
  <Popover>
    <PopoverTrigger as-child>
      <Button
        variant="outline"
        size="icon"
        class="relative rounded-full"
      >
        <Wifi class="h-5 w-5" />

        <span class="absolute -top-1 -right-1 flex h-3 w-3">
          <span
            v-if="!isAllGood"
            class="animate-ping absolute inline-flex h-full w-full rounded-full bg-red-400 opacity-75"
          ></span>
          <span
            class="relative inline-flex rounded-full h-3 w-3"
            :class="isAllGood ? 'bg-green-500' : 'bg-red-500'"
          ></span>
        </span>
      </Button>
    </PopoverTrigger>

    <PopoverContent class="w-64">
      <div class="space-y-4">
        <h4 class="font-medium leading-none">Connection Status</h4>
        <div class="space-y-2">

          <div class="flex items-center justify-between">
            <div class="flex items-center gap-2">
              <Wifi class="h-4 w-4 text-muted-foreground" />
              <span>WebSocket</span>
            </div>
            <CheckCircle2 v-if="isWsConnected" class="h-5 w-5 text-green-500" />
            <XCircle v-else class="h-5 w-5 text-red-500" />
          </div>

          <div class="flex items-center justify-between">
            <div class="flex items-center gap-2">
              <Usb class="h-4 w-4 text-muted-foreground" />
              <span>Serial</span>
            </div>
            <CheckCircle2 v-if="isSerialConnected" class="h-5 w-5 text-green-500" />
            <XCircle v-else class="h-5 w-5 text-red-500" />
          </div>

          <div class="flex items-center justify-between">
            <div class="flex items-center gap-2">
              <Gamepad2 class="h-4 w-4 text-muted-foreground" />
              <span>Controller</span>
            </div>
            <CheckCircle2 v-if="isControllerConnected" class="h-5 w-5 text-green-500" />
            <XCircle v-else class="h-5 w-5 text-red-500" />
          </div>

        </div>
      </div>
    </PopoverContent>
  </Popover>
</template>
