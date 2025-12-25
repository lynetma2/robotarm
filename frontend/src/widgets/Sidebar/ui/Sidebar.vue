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
  SidebarHeader,
  SidebarProvider
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
  <!-- 
    Ideally, SidebarProvider should wrap your entire MainLayout. 
    If you can't move it, this will work for the sidebar itself, 
    but the main content won't automatically adjust its width.
  -->
  <SidebarProvider>
    <Sidebar>
      <SidebarHeader />
      <SidebarContent>
        <SidebarGroup>
          <SidebarGroupLabel>Menu</SidebarGroupLabel>
          <SidebarGroupContent>
            <SidebarMenu>
              <SidebarMenuItem v-for="link in links" :key="link.name">
                <SidebarMenuButton as-child>
                  <a :href="link.href">
                    <component :is="link.icon" />
                    <span>{{ link.name }}</span>
                  </a>
                </SidebarMenuButton>
              </SidebarMenuItem>
            </SidebarMenu>
          </SidebarGroupContent>
        </SidebarGroup>
      </SidebarContent>
    </Sidebar>
  </SidebarProvider>
</template>
