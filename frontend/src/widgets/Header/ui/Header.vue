<script setup lang="ts">
import { EmergencyStop } from "@/features/EmergencyStop"
import { ConnectionStatus } from "@/features/ConnectionStatus"
import { ModeSwitch } from "@/features/ModeSwitch"
import { RobotState } from "@/entities/Robot"
import { SidebarTrigger } from "@/shared/ui/sidebar"
import { Separator } from "@/shared/ui/separator"
</script>

<template>
  <header class="flex h-16 shrink-0 items-center gap-2 border-b px-4 transition-[width,height] ease-linear group-has-data-[collapsible=icon]/sidebar-wrapper:h-12 bg-background">
    <div class="flex items-center gap-2 px-4">
      <SidebarTrigger class="-ml-1" />
      <Separator orientation="vertical" class="mr-2 h-4" />
      <!-- You can add Breadcrumbs here later -->
      <h1 class="text-sm font-semibold">Robot Arm Control</h1>
    </div>

    <div class="ml-auto flex items-center gap-4 px-4">
      <RobotState />
      <ModeSwitch />
      <ConnectionStatus />
      <EmergencyStop />
    </div>
  </header>
</template>
