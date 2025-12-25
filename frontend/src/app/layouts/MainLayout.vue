<script setup lang="ts">
import { Header } from "@/widgets/Header"
import { Sidebar } from "@/widgets/Sidebar"
import { SidebarProvider, SidebarInset } from "@/shared/ui/sidebar"
</script>

<template>
  <SidebarProvider>
    <Sidebar />
    <SidebarInset>
      <Header />
      <main class="flex-1 p-6">
        <slot />
      </main>
    </SidebarInset>
  </SidebarProvider>
</template>
