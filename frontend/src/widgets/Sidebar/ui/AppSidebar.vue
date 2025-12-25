<script setup lang="ts">
import { LayoutDashboard, Gamepad2, FileCode, Activity, Settings } from "lucide-vue-next"
import {
  Sidebar,
  SidebarContent,
  SidebarGroup,
  SidebarGroupContent,
  SidebarGroupLabel,
  SidebarMenu,
  SidebarMenuButton,
  SidebarMenuItem,
  SidebarHeader
} from "@/shared/ui/sidebar"

const links = [
  { name: 'Dashboard', icon: LayoutDashboard, href: '/' },
  { name: 'Manual Control', icon: Gamepad2, href: '/manual' },
  { name: 'Editor', icon: FileCode, href: '/editor' },
  { name: 'Diagnostics', icon: Activity, href: '/diagnostics' },
  { name: 'Motor Config', icon: Settings, href: '/config' },
]
</script>

<template>
  <Sidebar>
    <SidebarHeader />
    <SidebarContent>
      <SidebarGroup>
        <SidebarGroupLabel>Menu</SidebarGroupLabel>
        <SidebarGroupContent>
          <SidebarMenu>
            <SidebarMenuItem v-for="link in links" :key="link.name">
              <SidebarMenuButton as-child>
                <RouterLink :to="link.href">
                  <component :is="link.icon" />
                  <span>{{ link.name }}</span>
                </RouterLink>
              </SidebarMenuButton>
            </SidebarMenuItem>
          </SidebarMenu>
        </SidebarGroupContent>
      </SidebarGroup>
    </SidebarContent>
  </Sidebar>
</template>
