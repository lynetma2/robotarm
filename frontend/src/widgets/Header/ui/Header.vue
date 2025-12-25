<script setup lang="ts">
import { EmergencyStop } from "@/features/EmergencyStop"
import { ConnectionStatus } from "@/features/ConnectionStatus"
import { ModeSwitch } from "@/features/ModeSwitch"
import { RobotState } from "@/entities/Robot"
</script>

<template>
  <header class="flex items-center justify-between border-b px-6 py-3 bg-background">
    <div class="flex items-center gap-4">
      <h1 class="text-xl font-bold">Robot Arm</h1>
    </div>

    <div class="flex items-center gap-4">
      <RobotState />
      <ModeSwitch />
      <ConnectionStatus />
      <EmergencyStop />
    </div>
  </header>
</template>
