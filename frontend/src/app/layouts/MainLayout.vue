<script setup lang="ts">
import { Header } from "@/widgets/Header"
import { Sidebar } from "@/widgets/Sidebar"
</script>

<template>
  <div class="min-h-screen flex flex-col">
    <Header />
    <div class="flex flex-1">
      <Sidebar />
      <main class="flex-1 p-6">
        <slot />
      </main>
    </div>
  </div>
</template>
