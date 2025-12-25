import type { InjectionKey, Ref } from 'vue'
import { inject } from 'vue'

export const SIDEBAR_COOKIE_NAME = 'sidebar:state'
export const SIDEBAR_COOKIE_MAX_AGE = 60 * 60 * 24 * 7
export const SIDEBAR_WIDTH = '16rem'
export const SIDEBAR_WIDTH_MOBILE = '18rem'
export const SIDEBAR_WIDTH_ICON = '3rem'
export const SIDEBAR_KEYBOARD_SHORTCUT = 'b'

export type SidebarContext = {
  state: Ref<'expanded' | 'collapsed'>
  open: Ref<boolean>
  setOpen: (value: boolean) => void
  isMobile: Ref<boolean>
  openMobile: Ref<boolean>
  setOpenMobile: (value: boolean) => void
  toggleSidebar: () => void
}

export const SIDEBAR_KEY: InjectionKey<SidebarContext> = Symbol('SIDEBAR_KEY')

export function useSidebar() {
  return inject(SIDEBAR_KEY)!
}