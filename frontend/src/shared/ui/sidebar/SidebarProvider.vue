<script setup lang="ts">
import { ref, computed, provide, type HTMLAttributes, type Ref } from 'vue'
import { useVModel, useMediaQuery } from '@vueuse/core'
import { cn } from '@/shared/lib/utils'
import {
  SIDEBAR_COOKIE_NAME,
  SIDEBAR_WIDTH,
  SIDEBAR_WIDTH_ICON,
  SIDEBAR_KEY,
  type SidebarContext
} from './utils'

const props = withDefaults(defineProps<{
  defaultOpen?: boolean
  open?: boolean
  class?: HTMLAttributes['class']
}>(), {
  defaultOpen: true,
  open: undefined,
})

const emits = defineEmits<{
  'update:open': [open: boolean]
}>()

const isMobile = useMediaQuery('(max-width: 768px)')
const openMobile = ref(false)

// Internal state for desktop
const _open = ref(props.defaultOpen)
const open = useVModel(props, 'open', emits, {
  defaultValue: props.defaultOpen,
  passive: (props.open === undefined) as false,
}) as Ref<boolean>

const setOpen = (value: boolean) => {
  open.value = value
  // Optional: Save to cookie/localstorage here
  document.cookie = `${SIDEBAR_COOKIE_NAME}=${value}; path=/; max-age=${60 * 60 * 24 * 7}`
}

const setOpenMobile = (value: boolean) => {
  openMobile.value = value
}

const toggleSidebar = () => {
  return isMobile.value ? setOpenMobile(!openMobile.value) : setOpen(!open.value)
}

const state = computed(() => open.value ? 'expanded' : 'collapsed')

provide<SidebarContext>(SIDEBAR_KEY, {
  state,
  open,
  setOpen,
  isMobile,
  openMobile,
  setOpenMobile,
  toggleSidebar,
})
</script>

<template>
  <div
    :class="cn('group/sidebar-wrapper flex min-h-screen w-full has-[[data-variant=inset]]:bg-sidebar', props.class)"
    :style="{
      '--sidebar-width': SIDEBAR_WIDTH,
      '--sidebar-width-icon': SIDEBAR_WIDTH_ICON,
    }"
  >
    <slot />
  </div>
</template>