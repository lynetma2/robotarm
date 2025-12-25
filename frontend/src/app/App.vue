<script setup lang="ts">
import { useDark } from '@vueuse/core'
import "./style.css"
import MainLayout from "@/app/layouts/MainLayout.vue"

// --- Global Init ---
import { useSerialLogs } from '@/entities/log/model/useSerialLogs'

// Start the log subscription immediately. It persists across pages.
useSerialLogs()

// --- Auto Dark Mode ---
// This handles the 'dark' class on the html element automatically
useDark()
</script>

<template>
  <MainLayout>
    <RouterView v-slot="{ Component }">
      <!-- Try changing name to: "fade", "zoom", "blur", or "slide-left" -->
      <Transition name="blur" mode="out-in">
        <component :is="Component" />
      </Transition>
    </RouterView>
  </MainLayout>
</template>

<style>
/*
  1. Slide Fade (Original)
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

/*
  2. Simple Fade
*/
.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.3s ease;
}
.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}

/*
  3. Zoom / Scale
*/
.zoom-enter-active,
.zoom-leave-active {
  transition: all 0.3s ease;
}
.zoom-enter-from,
.zoom-leave-to {
  opacity: 0;
  transform: scale(0.95);
}

/*
  4. Blur Fade
*/
.blur-enter-active,
.blur-leave-active {
  transition: opacity 0.3s, filter 0.3s;
}
.blur-enter-from,
.blur-leave-to {
  opacity: 0;
  filter: blur(8px);
}
</style>
