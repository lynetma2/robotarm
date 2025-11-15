<script setup lang="ts">
import { computed } from 'vue'
import {
  Popover,
  PopoverContent,
  PopoverTrigger,
} from '@/components/ui/popover'
import { Button } from '@/components/ui/button'
import { Wifi, Usb, CheckCircle2, XCircle } from 'lucide-vue-next'

// These props will receive the live status from your app
const props = defineProps<{
  isWsConnected: boolean
  isSerialConnected: boolean
}>()

// A computed property to quickly know if everything is OK
const isAllGood = computed(() => props.isWsConnected && props.isSerialConnected)
</script>

<template>
  <div class="fixed top-4 right-4 z-50">
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
              <CheckCircle2
                v-if="isWsConnected"
                class="h-5 w-5 text-green-500"
              />
              <XCircle v-else class="h-5 w-5 text-red-500" />
            </div>

            <div class="flex items-center justify-between">
              <div class="flex items-center gap-2">
                <Usb class="h-4 w-4 text-muted-foreground" />
                <span>Serial</span>
              </div>
              <CheckCircle2
                v-if="isSerialConnected"
                class="h-5 w-5 text-green-500"
              />
              <XCircle v-else class="h-5 w-5 text-red-500" />
            </div>
          </div>
        </div>
      </PopoverContent>
    </Popover>
  </div>
</template>
