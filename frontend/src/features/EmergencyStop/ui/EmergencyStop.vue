<script setup lang="ts">
import { Button } from "@/shared/ui/button"
import { AlertTriangle } from "lucide-vue-next"

const triggerEmergencyStop = () => {
  console.error("EMERGENCY STOP TRIGGERED")
  // Add actual logic here
}
</script>

<template>
  <Button variant="destructive" @click="triggerEmergencyStop">
    <AlertTriangle class="mr-2 h-4 w-4" />
    E-STOP
  </Button>
</template>
