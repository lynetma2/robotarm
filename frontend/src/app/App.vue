<script setup lang="ts">
import { onMounted, onUnmounted } from 'vue'
import "./style.css"
import MainLayout from "@/app/layouts/MainLayout.vue"

// --- Global Init ---
import { useSerialLogs } from '@/entities/log/model/useSerialLogs'

// Start the log subscription immediately. It persists across pages.
useSerialLogs()

// --- Auto Dark Mode ---
// Sync 'dark' class with system preference
const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)')

const updateTheme = (e: MediaQueryListEvent | MediaQueryList) => {
  document.documentElement.classList.toggle('dark', e.matches)
}

onMounted(() => {
  updateTheme(mediaQuery)
  mediaQuery.addEventListener('change', updateTheme)
})

onUnmounted(() => {
  mediaQuery.removeEventListener('change', updateTheme)
})
</script>

<template>
  <MainLayout>
    <RouterView v-slot="{ Component }">
      <Transition name="slide-fade" mode="out-in">
        <component :is="Component" />
      </Transition>
    </RouterView>
  </MainLayout>
</template>

<style>
/*
  Nice Page Transition
  Slides slightly up and fades in.
*/
.slide-fade-enter-active,
.slide-fade-leave-active {
  transition: all 0.25s ease-out;
}

.slide-fade-enter-from {
  opacity: 0;
  transform: translateY(10px) scale(0.98);
}

.slide-fade-leave-to {
  opacity: 0;
  transform: translateY(-10px) scale(0.98);
}
</style>
