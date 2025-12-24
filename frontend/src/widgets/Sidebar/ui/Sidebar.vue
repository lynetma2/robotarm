<script setup lang="ts">
import { cn } from "@/lib/utils"
import { Button } from "@/shared/ui/button"
import { LayoutDashboard, Gamepad2, FileCode, Activity, Settings } from "lucide-vue-next"

const links = [
  { name: 'Dashboard', icon: LayoutDashboard, href: '/' },
  { name: 'Manual Control', icon: Gamepad2, href: '/manual' },
  { name: 'Editor', icon: FileCode, href: '/editor' },
  { name: 'Diagnostics', icon: Activity, href: '/diagnostics' },
  { name: 'Motor Config', icon: Settings, href: '/config' },
]
</script>

<template>
  <div class="pb-12 w-64 border-r min-h-screen">
    <div class="space-y-4 py-4">
      <div class="px-3 py-2">
        <h2 class="mb-2 px-4 text-lg font-semibold tracking-tight">
          Menu
        </h2>
        <div class="space-y-1">
          <Button
            v-for="link in links"
            :key="link.name"
            variant="ghost"
            class="w-full justify-start"
            as-child
          >
            <a :href="link.href">
              <component :is="link.icon" class="mr-2 h-4 w-4" />
              {{ link.name }}
            </a>
          </Button>
        </div>
      </div>
    </div>
  </div>
</template>
